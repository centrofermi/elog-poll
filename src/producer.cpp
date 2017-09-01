#include <string>
#include <sstream>
#include <stdlib.h>
#include <TFile.h>
#include <TChain.h>
#include <TString.h>
#include <TLeaf.h>
#include <TMath.h>

char* NextDay(const char* currentday);
bool IsInRange(const char* currentday, const char* lastday);
bool CfrString(const char* str1, const char* str2);

bool matches(std::string const& head, std::string const& tail, std::string const& s)
{
  return s.find(head) == 0 && s.find(tail) == s.size() - tail.size();
}

int main(int argc, char** argv)
{
  gErrorIgnoreLevel = kFatal;

  const char* pathToRecon = "/recon2";

  if (argc < 8)
    return 10;  // at least 8 arguments needed

  bool isRoot = 1;
  if (CfrString(argv[1], "ROOT"))
    isRoot = 1;
  else if (CfrString(argv[1], "CSV"))
    isRoot = 0;
  else
    return 11;  // first argument should be CSV or ROOT
  bool isTXT = !isRoot;

  const char* school = argv[2];

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

  Int_t nvar = 0;
  for (Int_t k = 7; k < argc; k += 2) {
    type[nvar] = argv[k - 1];
    var[nvar] = argv[k];
    nvar++;
    if (nvar == nmaxvar)
      k = argc;
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
  cutBase += cutMy;

  const char* currentday = dateIn;

  Int_t ndays = 0;
  int nfile = 0;

  TChain chain("Events");

  while (IsInRange(currentday, dateOut)) {
    std::ostringstream oss;

    oss
      << pathToRecon << '/'
      << school << '/'
      << currentday << '/'
      << school << "*.root";

    nfile += chain.Add(oss.str().c_str());

    ++ndays;

    currentday = NextDay(currentday);
  }

  if (ndays == 0)
    return 1;

  if (!nfile)
    return 3;

  if (!chain.GetEntriesFast())
    return 4;

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

  for (Int_t i = 0; i < workingtree->GetEntriesFast(); i++) {
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
        if (fvar[j] < 0)
          fvar[j] += 360;
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

char* NextDay(const char* currentday)
{
  Int_t dayPerMonth[12]
      = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  Int_t y, m, d;
  sscanf(currentday, "%4i", &y);
  if (currentday[5] != '0')
    sscanf(&(currentday[5]), "%i", &m);
  else
    sscanf(&(currentday[6]), "%i", &m);
  if (currentday[8] != '0')
    sscanf(&(currentday[8]), "%2i", &d);
  else
    sscanf(&(currentday[9]), "%2i", &d);

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
  Int_t y, m, d;
  sscanf(currentday, "%4i-%2i-%2i", &y, &m, &d);
  Int_t y2, m2, d2;
  sscanf(lastday, "%4i-%2i-%2i", &y2, &m2, &d2);

  if (y2 > y)
    return 1;
  else if (y2 < y)
    return 0;

  if (m2 > m)
    return 1;
  else if (m2 < m)
    return 0;

  if (d2 >= d)
    return 1;

  return 0;
}

bool CfrString(const char* str1, const char* str2)
{
  return strncmp(str1, str2, 100) == 0;
}
