#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <tuple>
#include <stdexcept>
#include <vector>
#include <stdlib.h>
#include <cstdio>
#include <TFile.h>
#include <TChain.h>
#include <TString.h>
#include <TLeaf.h>
#include <TMath.h>
#include <TString.h>
#include <TBranch.h>

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
    m_storage.ivar = val;
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

int main(int argc, char** argv)
{
  gErrorIgnoreLevel = kFatal;

  const char* pathToRecon = "/recon2";

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

  bool isTXT = !isRoot;

  const char* school = argv[2];
  const char* schoolInFile = argv[2];

  const char* dateIn = argv[3];
  const char* dateOut = argv[4];

  TString cutMy(CfrString(argv[5], "") ? "(1)" : argv[5]);

  const Int_t nmaxvar = 20;
  TString var[nmaxvar];
  const char* type[nmaxvar];
  void* addvar[nmaxvar];
  Int_t ivar[nmaxvar];
  Float_t fvar[nmaxvar];
  bool isInteger[nmaxvar];

  bool const is_mc = CfrString(argv[6], "1");

  if (is_mc) {
    pathToRecon = "/MC";

    // temporary: data set to 1st October 2017
    dateIn = "2017-10-01";
    dateOut = "2017-10-01";
    schoolInFile = "MONT-01";
  }

  Int_t nvar = 0;
  for (Int_t k = 8; k < argc; k += 2) {
    type[nvar] = argv[k - 1];
    var[nvar] = argv[k];
    nvar++;
    if (nvar == nmaxvar)
      k = argc;
  }

  if (nvar == 0) {
    std::cout << "Error: At least one variable is needed!\n";
    return 12;
  }

  for (Int_t j = 0; j < nvar; j++) {
    if (CfrString(type[j], "I")) {
      addvar[j] = &(ivar[j]);
      isInteger[j] = true;
    } else {
      addvar[j] = &(fvar[j]);
      isInteger[j] = false;
    }
  }

  TString cutBase("(StatusCode==0)&&");

  const char* currentday = dateIn;

  Int_t ndays = 0;
  int nfile = 0;

  TChain chain("Events");

  while (IsInRange(currentday, dateOut) && ndays < 30) {
    std::ostringstream oss;

    oss
      << pathToRecon << '/'
      << school << '/'
      << currentday << '/'
      << schoolInFile << "*dst.root";

    nfile += chain.Add(oss.str().c_str());

    ++ndays;

    currentday = NextDay(currentday);
  }

  if (ndays == 0) {
    std::cout << "Error: No data available in the requested period!\n";
    return 1;
  }

  if (!nfile) {
    std::cout << "Error: No data available in the requested period!\n";
    return 3;
  }

  if (!chain.GetEntriesFast()) {
    std::cout << "Error: No data available in the requested period!\n";
    return 4;
  }

  // minimal info
  chain.SetBranchStatus("*", 0);
  chain.SetBranchStatus("StatusCode", 1);
  chain.SetBranchStatus("XDir", 1);
  chain.SetBranchStatus("YDir", 1);
  chain.SetBranchStatus("ZDir", 1);

  // variable to add pressure as an option for the output
  bool reqPressure = false;

  for (Int_t j = 0; j < nvar; j++) {
    if (!(var[j].Contains("Theta") || var[j].Contains("Phi")))
      chain.SetBranchStatus(var[j].Data(), 1);

    if (var[j].Contains("Pressure")) {
      reqPressure = true;
    }
  }

  if (cutMy.Contains("Theta")) {
    cutMy.ReplaceAll("Theta", "TMath::ACos(ZDir[0])*TMath::RadToDeg()");
  }
  if (cutMy.Contains("Phi")) {
    cutMy.ReplaceAll(
        "Phi", "TMath::ATan2(YDir[0],XDir[0])*TMath::RadToDeg()");
  }

  if (cutMy.Contains("RunNumber"))
    chain.SetBranchStatus("RunNumber", 1);
  if (cutMy.Contains("Seconds"))
    chain.SetBranchStatus("Seconds", 1);
  if (cutMy.Contains("Nanoseconds"))
    chain.SetBranchStatus("Nanoseconds", 1);
  if (cutMy.Contains("ChiSquare")) {
    chain.SetBranchStatus("ChiSquare", 1);
    cutMy.ReplaceAll("ChiSquare", "ChiSquare[0]");
  }
  if (cutMy.Contains("TimeOfFlight")) {
    chain.SetBranchStatus("TimeOfFlight", 1);
    cutMy.ReplaceAll("TimeOfFlight", "TimeOfFlight[0]");
  }
  if (cutMy.Contains("TrackLength")) {
    chain.SetBranchStatus("TrackLength", 1);
    cutMy.ReplaceAll("TrackLength", "TrackLength[0]");
  }
  if (cutMy.Contains("DeltaTime"))
    chain.SetBranchStatus("DeltaTime", 1);

  cutBase += cutMy;

  std::ostringstream oss;
  oss << "/tmp/" << school << "from" << dateIn << "to" << dateOut << (isRoot ? ".root" : ".csv");

  std::string const outname = oss.str();

  // before to apply cuts add extra branch for pressure
  TTree *cloned = chain.CloneTree();
  if (reqPressure) {
    Float_t pressure = 0;
    TBranch *bPr = cloned->Branch("Pressure", &pressure, "Pressure/F");
    TString namefile;
    for (Int_t i = 0; i < chain.GetEntries(); ++i) {
      chain.GetEvent(i);

      if (namefile.CompareTo(chain.GetFile()->GetName())) {
        // get pressure
        TFile ftemp(chain.GetFile()->GetName());
        TTree* weather = (TTree *) ftemp.Get("Weather");
        weather->GetEvent(0);
        pressure = weather->GetLeaf("Pressure")->GetValue();
        namefile = chain.GetFile()->GetName();
      }
      bPr->Fill();
    }
  }

  TTree* const workingtree = cloned->CopyTree(cutBase.Data());

  Long64_t const nmaxentr = 12500000 / nvar;
  int const nentries = TMath::Min(workingtree->GetEntriesFast(), nmaxentr);

  if (isRoot) {
    TFile foutRoot(outname.c_str(), "RECREATE");
    TTree outputTree("eee", "eee");
    for (int j = 0; j != nvar; ++j) {
      outputTree.Branch(
          var[j].Data(), addvar[j], Form("%s/%s", var[j].Data(), type[j]));
    }

    for (int i = 0; i != nentries; ++i) {
      workingtree->GetEvent(i);

      Float_t const xd = workingtree->GetLeaf("XDir")->GetValue();
      Float_t const yd = workingtree->GetLeaf("YDir")->GetValue();
      Float_t const zd = workingtree->GetLeaf("ZDir")->GetValue();

      for (int j = 0; j != nvar; ++j) {
        if (var[j].Contains("Theta")) {
          fvar[j] = TMath::ACos(zd) * TMath::RadToDeg();
        } else if (var[j].Contains("Phi")) {
          fvar[j] = TMath::ATan2(yd, xd) * TMath::RadToDeg();
        } else {
          if (isInteger[j]) {
            ivar[j] = workingtree->GetLeaf(var[j].Data())->GetValue();
          } else {
            fvar[j] = workingtree->GetLeaf(var[j].Data())->GetValue();
          }
        }
      }
      outputTree.Fill();
    }

//    TFile foutRoot(outname.c_str(), "RECREATE");
    outputTree.Write();
    foutRoot.Close();
  } else {
    std::ofstream outCSV(outname.c_str());
    outCSV << std::fixed;

    for (int i = 0; i != nvar - 1; ++i) {
      outCSV << var[i].Data() << ',';
    }
    outCSV << var[nvar - 1] << '\n';

    for (int i = 0; i != nentries; ++i) {
      workingtree->GetEvent(i);

      Float_t const xd = workingtree->GetLeaf("XDir")->GetValue();
      Float_t const yd = workingtree->GetLeaf("YDir")->GetValue();
      Float_t const zd = workingtree->GetLeaf("ZDir")->GetValue();

      for (int j = 0; j != nvar - 1; ++j) {
        if (var[j].Contains("Theta")) {
          outCSV << TMath::ACos(zd) * TMath::RadToDeg() << ',';
        } else if (var[j].Contains("Phi")) {
          outCSV << TMath::ATan2(yd, xd) * TMath::RadToDeg() << ',';
        } else {
          if (isInteger[j]) {
            outCSV << static_cast<int64_t>(workingtree->GetLeaf(var[j].Data())->GetValue()) << ',';
          } else {
            outCSV << workingtree->GetLeaf(var[j].Data())->GetValue() << ',';
          }
        }
      }

      {
        int const j = nvar - 1;

        if (var[j].Contains("Theta")) {
          outCSV << TMath::ACos(zd) * TMath::RadToDeg() << '\n';
        } else if (var[j].Contains("Phi")) {
          outCSV << TMath::ATan2(yd, xd) * TMath::RadToDeg() << '\n';
        } else {
          if (isInteger[j]) {
            outCSV << static_cast<int64_t>(workingtree->GetLeaf(var[j].Data())->GetValue()) << '\n';
          } else {
            outCSV << workingtree->GetLeaf(var[j].Data())->GetValue() << '\n';
          }
        }
      }
    }
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
