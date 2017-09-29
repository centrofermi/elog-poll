#include <string>
#include <sstream>
#include <tuple>
#include <stdlib.h>
#include <TFile.h>
#include <TChain.h>
#include <TString.h>
#include <TLeaf.h>
#include <TMath.h>

using date = std::tuple<int, int, int>;

char* NextDay(const char* currentday);
bool IsInRange(const char* currentday, const char* lastday);
bool CfrString(const char* str1, const char* str2);

int main(int argc, char** argv)
{
  gErrorIgnoreLevel = kFatal;

  const char* pathToRecon = "/recon2";

  if (argc < 9){
    printf("Error: Something is missing (please check your submission)!\n");
    return 10;  // at least 8 arguments needed
  }

  bool isRoot = 1;
  if (CfrString(argv[1], "ROOT"))
    isRoot = 1;
  else if (CfrString(argv[1], "CSV"))
    isRoot = 0;
  else{
    printf("Error: CSV or ROOT should be set!\n");
    return 11;  // first argument should be CSV or ROOT
  }
  bool isTXT = !isRoot;

  const char* school = argv[2];
  const char* schoolInFile = argv[2];

  const char* dateIn = argv[3];
  const char* dateOut = argv[4];

  TString cutMy(Form("(%s)", argv[5]));
  if (CfrString(argv[5], ""))
    cutMy = "(1)";

  const Int_t nmaxvar = 20;
  TString var[nmaxvar];
  const char* type[nmaxvar];
  void* addvar[nmaxvar];
  Int_t ivar[nmaxvar];
  Float_t fvar[nmaxvar];
  Bool_t isInteger[nmaxvar];


  bool const is_mc = CfrString(argv[6], "1");

  if(is_mc){
     pathToRecon = "/MC";

     // temporary: data set to 1st October 2017
     dateIn="2017-10-01";
     dateOut="2017-10-01";
     schoolInFile="MONT-01";
  }

  Int_t nvar = 0;
  for (Int_t k = 8; k < argc; k += 2) {
    type[nvar] = argv[k - 1];
    var[nvar] = argv[k];
    nvar++;
    if (nvar == nmaxvar)
      k = argc;
  }

  if(nvar==0){
    printf("Error: At least one variable is needed!\n");
    return 12;
  }

  for (Int_t j = 0; j < nvar; j++) {
    if (CfrString(type[j], "I")) {
      addvar[j] = &(ivar[j]);
      isInteger[j] = 1;
    } else {
      addvar[j] = &(fvar[j]);
      isInteger[j] = 0;
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

  if (ndays == 0){
    printf("Error: No data available in the requested period!\n");
    return 1;
  }

  if (!nfile){
    printf("Error: No data available in the requested period!\n");
    return 3;
  }

  if (!chain.GetEntriesFast()){
    printf("Error: No data available in the requested period!\n");
    return 4;
  }

  // minimal info
  chain.SetBranchStatus("*", 0);
  chain.SetBranchStatus("StatusCode", 1);
  chain.SetBranchStatus("XDir", 1);
  chain.SetBranchStatus("YDir", 1);
  chain.SetBranchStatus("ZDir", 1);

  for (Int_t j = 0; j < nvar; j++) {
    if (!(var[j].Contains("Theta") || var[j].Contains("Phi")))
      chain.SetBranchStatus(var[j].Data(), 1);
  }

  if(cutMy.Contains("Theta")) {
    cutMy.ReplaceAll("Theta", "TMath::ACos(ZDir[0])*TMath::RadToDeg()");
  }
  if(cutMy.Contains("Phi")) {
    cutMy.ReplaceAll("Phi", "TMath::ATan2(YDir[0],XDir[0])*TMath::RadToDeg()");
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

  TTree* workingtree = chain.CopyTree(cutBase.Data());

  TTree outputTree("eee", "eee");
  for (Int_t j = 0; j < nvar; j++)
    outputTree.Branch(
        var[j].Data(), addvar[j], Form("%s/%s", var[j].Data(), type[j]));

  Float_t xd, yd, zd;

  char* outname = Form("/tmp/%sfrom%sto%s.csv", school, dateIn, dateOut);

  FILE* foutCSV = fopen(outname, "w");
  for (Int_t j = 0; j < nvar; j++) {
    if (j > 0)
      fprintf(foutCSV, ",");
    fprintf(foutCSV, "%s", var[j].Data());
  }
  fprintf(foutCSV, "\n");

  Long64_t nmaxentr = 12500000/nvar;
  int nentries=TMath::Min(workingtree->GetEntriesFast(),nmaxentr);

  for (Int_t i = 0; i < nentries; i++) {
    workingtree->GetEvent(i);

    xd = workingtree->GetLeaf("XDir")->GetValue();
    yd = workingtree->GetLeaf("YDir")->GetValue();
    zd = workingtree->GetLeaf("ZDir")->GetValue();

    for (Int_t j = 0; j < nvar; j++) {
      if (j > 0)
        fprintf(foutCSV, ",");

      if (!(var[j].Contains("Theta") || var[j].Contains("Phi"))) {
        if (isInteger[j])
          ivar[j] = workingtree->GetLeaf(var[j].Data())->GetValue();
        else
          fvar[j] = workingtree->GetLeaf(var[j].Data())->GetValue();
      } else if (var[j].Contains("Theta")) {
        fvar[j] = TMath::ACos(zd) * TMath::RadToDeg();
      } else if (var[j].Contains("Phi")) {
        fvar[j] = TMath::ATan2(yd, xd) * TMath::RadToDeg();
      }

      if (isTXT) {
        if (isInteger[j])
          fprintf(foutCSV, "%i", ivar[j]);
        else
          fprintf(foutCSV, "%f", fvar[j]);
      }
    }
    if (isRoot)
      outputTree.Fill();
    if (isTXT)
      fprintf(foutCSV, "\n");
  }

  fclose(foutCSV);

  if (isTXT) {
    printf("%s\n", outname);
  }

  if (isRoot) {
    outname = Form("/tmp/%sfrom%sto%s.root", school, dateIn, dateOut);
    TFile foutRoot(outname, "RECREATE");
    outputTree.Write();
    foutRoot.Close();
    printf("%s\n", outname);
  }

  return 0;
}

date parse_date(char const* str)
{
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

char* NextDay(const char* currentday)
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

  return Form("%i-%02i-%02i", y, m, d);
}

bool IsInRange(const char* currentday, const char* lastday)
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
