#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <tuple>
#include <stdexcept>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <TFile.h>
#include <TTree.h>
#include <TLeaf.h>
#include <TMath.h>
#include <TString.h>

#include <directory.hpp>

using date = std::tuple<int, int, int>;

std::string NextDay(std::string const& currentday);
bool IsInRange(std::string const& currentday, std::string const& lastday);
bool CfrString(const char* str1, const char* str2);

class Variable
{
  union {
    int ivar;
    float fvar;
  } m_storage;

  std::string m_name;

 public:

  enum type_t { INT, FLOAT };

  Variable(std::string const& name, std::string const& type)
  : m_name(name)
  , m_type(make_type(type))
  {}

  void* storage()
  {
    return static_cast<void*>(&m_storage);
  }

  std::string name() const
  {
    return m_name;
  }

  std::string stype() const
  {
    return is_integer() ? "I" : "F";
  }

  type_t type() const
  {
    return m_type;
  }

  bool is_integer() const
  {
    return m_type == INT;
  }

  std::string rootname() const
  {
    return name() + '/' + stype();
  }

  int i() const
  {
    return m_storage.ivar;
  }

  float f() const
  {
    return m_storage.fvar;
  }

  int i(int val)
  {
    return m_storage.ivar = val;
  }

  float f(float val)
  {
    return m_storage.fvar = val;
  }

  static
  type_t make_type(std::string const& t)
  {
    if (t == "I") {
      return INT;
    } else if (t == "F") {
      return FLOAT;
    } else {
      throw std::runtime_error("Unrecognised type " + t);
    }
  }

 private:

  type_t m_type;
};

std::vector<std::string> fileset(
    std::string const& beg_date
  , std::string const& end_date
  , std::string const& telescope
  , bool MC
  , std::size_t max_days = 30
) {
  std::string const pathToRecon = MC ? "/MC" : "/recon2";
  std::string const schoolInFile = MC ? std::string("MONT-01") : telescope;

  std::vector<std::string> ret;

  std::string currentday = beg_date;
  std::size_t ndays = 0;
  while (IsInRange(currentday, end_date) && ndays < max_days) {
    auto const folder_name = pathToRecon + '/'
      + telescope + '/'
      + currentday + '/';

    try {
      auto dir = open_dir(folder_name);
      auto list = matching_items(dir, schoolInFile + ".*dst\\.root");

      std::for_each(
          std::begin(list)
        , std::end(list)
        , [&folder_name, &ret](std::string const& item) {
            ret.emplace_back(folder_name + item);
          }
      );

      ++ndays;
    } catch (...) {}

    currentday = NextDay(currentday);
  }

  return ret;
}

void rootout(
    std::vector<std::string> const& fileset
  , std::vector<Variable>& vars
  , std::string const& outname
  , TString const& cut
  , std::size_t maxentries
) {
  TTree outputTree("eee", "eee");
  for (auto& var : vars) {
    outputTree.Branch(var.name().c_str(), var.storage(), var.rootname().c_str());
  }

  std::size_t total = 0;
  for (auto const& fname : fileset) {
    TFile file(fname.c_str());

    // Extract the pressure
    auto weather = dynamic_cast<TTree*>(file.Get("Weather"));
    weather->GetEvent(0);
    auto const pressure = weather->GetLeaf("Pressure")->GetValue();

    // Extract the events
    auto tree = dynamic_cast<TTree*>(file.Get("Events"));
    auto workingtree = tree->CopyTree(cut.Data());

    auto const nentries = workingtree->GetEntriesFast();
    for (int i = 0; i < nentries && total < maxentries; ++i, ++total) {
      workingtree->GetEvent(i);

      Float_t const xd = workingtree->GetLeaf("XDir")->GetValue();
      Float_t const yd = workingtree->GetLeaf("YDir")->GetValue();
      Float_t const zd = workingtree->GetLeaf("ZDir")->GetValue();

      for (auto& var : vars) {
        if (var.name() == "Pressure") {
          var.f(pressure);
        } else if (var.name() == "Theta") {
          var.f(TMath::ACos(zd) * TMath::RadToDeg());
        } else if (var.name() == "Phi") {
          var.f(TMath::ATan2(yd, xd) * TMath::RadToDeg());
        } else {
          if (var.is_integer()) {
            var.i(workingtree->GetLeaf(var.name().c_str())->GetValue());
          } else {
            var.f(workingtree->GetLeaf(var.name().c_str())->GetValue());
          }
        }
      }
      outputTree.Fill();
    }
  }

  TFile foutRoot(outname.c_str(), "RECREATE");
  outputTree.Write();
  foutRoot.Close();
}

void csvout(
    std::vector<std::string> const& fileset
  , std::vector<Variable>& vars
  , std::string const& outname
  , TString const& cut
  , std::size_t maxentries
) {
  std::ofstream outCSV(outname.c_str());
  outCSV << std::fixed;

  auto const nvar = vars.size();

  for (std::size_t i = 0; i != nvar - 1; ++i) {
    outCSV << vars[i].name() << ',';
  }

  outCSV << vars[nvar - 1].name() << '\n';

  std::size_t total = 0;
  for (auto const& fname : fileset) {
    TFile file(fname.c_str());

    // Extract the pressure
    auto weather = dynamic_cast<TTree*>(file.Get("Weather"));
    weather->GetEvent(0);
    auto const pressure = weather->GetLeaf("Pressure")->GetValue();

    // Extract the events
    auto tree = dynamic_cast<TTree*>(file.Get("Events"));
    auto workingtree = tree->CopyTree(cut.Data());

    auto const nentries = workingtree->GetEntriesFast();
    for (int i = 0; i < nentries && total < maxentries; ++i, ++total) {
      workingtree->GetEvent(i);

      Float_t const xd = workingtree->GetLeaf("XDir")->GetValue();
      Float_t const yd = workingtree->GetLeaf("YDir")->GetValue();
      Float_t const zd = workingtree->GetLeaf("ZDir")->GetValue();

      for (std::size_t j = 0; j != nvar; ++j) {
        auto const sep = j != nvar - 1 ? ',' : '\n';

        if (vars[j].name() == "Pressure") {
          outCSV << pressure << sep;
        } else if (vars[j].name() == "Theta") {
          outCSV << TMath::ACos(zd) * TMath::RadToDeg() << sep;
        } else if (vars[j].name() == "Phi") {
          outCSV << TMath::ATan2(yd, xd) * TMath::RadToDeg() << sep;
        } else {
          if (vars[j].is_integer()) {
            outCSV << static_cast<int64_t>(workingtree->GetLeaf(vars[j].name().c_str())->GetValue()) << sep;
          } else {
            outCSV << workingtree->GetLeaf(vars[j].name().c_str())->GetValue() << sep;
          }
        }
      }
    }
  }
}

int main(int argc, char** argv)
{
  gErrorIgnoreLevel = kFatal;

  if (argc < 9) {
    std::cout <<
        "Error: Something is missing (please check your submission)!\n";
    return 10;  // at least 8 arguments needed
  }

  bool isRoot = true;
  if (CfrString(argv[1], "ROOT")) {
    isRoot = true;
  } else if (CfrString(argv[1], "CSV")) {
    isRoot = false;
  } else {
    std::cout << "Error: CSV or ROOT should be set!\n";
    return 11;  // first argument should be CSV or ROOT
  }

  const char* school = argv[2];

  const char* dateIn = argv[3];
  const char* dateOut = argv[4];

  TString cutMy(CfrString(argv[5], "") ? "(1)" : argv[5]);


  bool const is_mc = CfrString(argv[6], "1");

  if (is_mc) {
    // temporary: data set to 1st October 2017
    dateIn = "2017-10-01";
    dateOut = "2017-10-01";
  }

  std::vector<Variable> variables;
  for (int k = 8; k < argc; k += 2) {
    variables.emplace_back(Variable(argv[k], argv[k - 1]));
  }

  if (variables.empty()) {
    std::cout << "Error: At least one variable is needed!\n";
    return 12;
  }

  auto const fs = fileset(dateIn, dateOut, school, is_mc);

  if (fs.empty()) {
    std::cout << "Error: No data available in the requested period!\n";
    return 1;
  }

  if (cutMy.Contains("Theta")) {
    cutMy.ReplaceAll("Theta", "TMath::ACos(ZDir[0])*TMath::RadToDeg()");
  }
  if (cutMy.Contains("Phi")) {
    cutMy.ReplaceAll(
        "Phi", "TMath::ATan2(YDir[0],XDir[0])*TMath::RadToDeg()");
  }
  if (cutMy.Contains("ChiSquare")) {
    cutMy.ReplaceAll("ChiSquare", "ChiSquare[0]");
  }
  if (cutMy.Contains("TimeOfFlight")) {
    cutMy.ReplaceAll("TimeOfFlight", "TimeOfFlight[0]");
  }
  if (cutMy.Contains("TrackLength")) {
    cutMy.ReplaceAll("TrackLength", "TrackLength[0]");
  }

  TString const cutBase = TString("(StatusCode==0)&&") + cutMy;

  std::ostringstream oss;
  oss << "/tmp/" << school << "from" << dateIn << "to" << dateOut << (isRoot ? ".root" : ".csv");

  std::string const outname = oss.str();

  std::size_t const nmaxentr = 12500000 / variables.size();

  if (isRoot) {
    rootout(fs, variables, outname, cutBase, nmaxentr);
  } else {
    csvout(fs, variables, outname, cutBase, nmaxentr);
  }

  std::cout << outname << '\n';
  std::cout.flush();
  _exit(0);
}

date parse_date(std::string const& string)
{
  char const* str = string.c_str();
  int y = 0, m = 0, d = 0;
  sscanf(str, "%4i", &y);
  if (str[5] != '0')
    sscanf(&(str[5]), "%i", &m);
  else
    sscanf(&(str[6]), "%i", &m);
  if (str[8] != '0')
    sscanf(&(str[8]), "%2i", &d);
  else
    sscanf(&(str[9]), "%2i", &d);

  return std::make_tuple(y, m, d);
}

std::string NextDay(std::string const& currentday)
{
  Int_t dayPerMonth[12]
      = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  date const day = parse_date(currentday);
  int y = std::get<0>(day);
  int m = std::get<1>(day);
  int d = std::get<2>(day);

  if (!(y % 4))
    dayPerMonth[1] = 29;

  d++;
  if (d > dayPerMonth[m - 1]) {
    d = 1;
    m++;
  }

  if (m > 12) {
    m = 1;
    y++;
  }

  char buffer[12];
  std::snprintf(buffer, 12, "%i-%02i-%02i", y, m, d);
  return buffer;
}

bool IsInRange(std::string const& currentday, std::string const& lastday)
{
  date const current = parse_date(currentday);
  date const last = parse_date(lastday);

  int const y1 = std::get<0>(current);
  int const m1 = std::get<1>(current);
  int const d1 = std::get<2>(current);

  int const y2 = std::get<0>(last);
  int const m2 = std::get<1>(last);
  int const d2 = std::get<2>(last);

  if (y2 > y1)
    return true;
  else if (y2 < y1)
    return false;

  if (m2 > m1)
    return true;
  else if (m2 < m1)
    return false;

  if (d2 >= d1)
    return true;

  return false;
}

bool CfrString(const char* str1, const char* str2)
{
  return strncmp(str1, str2, 100) == 0;
}
