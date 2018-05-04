#include <iostream>
#include <iomanip>      // std::setprecision
#include <fstream>
#include <string>
#include <sstream>
#include <tuple>
#include <stdlib.h>
#include <TFile.h>
#include <TChain.h>
#include <TString.h>
#include <TLeaf.h>
#include <TMath.h>
#include <TString.h>
#include <TBranch.h>
#include <TTree.h>

#include <directory.hpp>

using date = std::tuple<int, int, int>;

char* NextDay(const char* currentday);
bool IsInRange(const char* currentday, const char* lastday);
bool CfrString(const char* str1, const char* str2);

bool ProcessSingleFile(const char *filename,std::ofstream &outCSV,Int_t nvar,bool isRoot,bool reqPressure,TString &cut);

const Int_t nmaxvar = 20;
TString var[nmaxvar];
const char* type[nmaxvar];
void* addvar[nmaxvar];
Int_t ivar[nmaxvar];
Float_t fvar[nmaxvar];
bool isInteger[nmaxvar];
TTree *outputTree;
Int_t ntotevent = 0;

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

  TString cutMy(Form("(%s)", argv[5]));
  if (CfrString(argv[5], "")) {
    cutMy = "(1)";
  }

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

  // variable to add pressure as an option for the output
  bool reqPressure = false;

  for (Int_t j = 0; j < nvar; j++) {
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

  if (cutMy.Contains("ChiSquare")) {
    cutMy.ReplaceAll("ChiSquare", "ChiSquare[0]");
  }
  if (cutMy.Contains("TimeOfFlight")) {
    cutMy.ReplaceAll("TimeOfFlight", "TimeOfFlight[0]");
  }
  if (cutMy.Contains("TrackLength")) {
    cutMy.ReplaceAll("TrackLength", "TrackLength[0]");
  }

  cutBase += cutMy;

  std::ostringstream oss;
  oss << "/tmp/" << school << "from" << dateIn << "to" << dateOut << (isRoot ? ".root" : ".csv");

  std::string const outname = oss.str();

  TFile *foutRoot;
  if(isRoot) foutRoot =  new TFile(outname.c_str(), "RECREATE");
  outputTree = new TTree("eee", "eee");
  for (int j = 0; j != nvar; ++j) {
    outputTree->Branch(
		       var[j].Data(), addvar[j], Form("%s/%s", var[j].Data(), type[j]));
  }
  
  const char *nameCSV = outname.c_str();
  if(isRoot) nameCSV = "/tmp/dummy";

  std::ofstream outCSV(nameCSV);
  outCSV << std::setprecision(9) << std::fixed;

  for (int i = 0; i != nvar - 1; ++i) {
    outCSV << var[i].Data() << ',';
  }
  outCSV << var[nvar - 1] << '\n';

  // analyze single files
  while (IsInRange(currentday, dateOut) && ndays < 30) {
    directory dir = open_dir(Form("%s/%s/%s/",pathToRecon,school,currentday));
    
    std::vector<std::string> file_dot_root = matching_items(dir, ".*\.root"); // la stringa passata alla funzione è una regular expression

    nfile=file_dot_root.size();
    for(int i=0;i<nfile;i++){
//      std::cout << i << ")" << file_dot_root.at(i)<< std::endl;
      std::string filewithpath = Form("%s/%s/%s/",pathToRecon,school,currentday);
      filewithpath += file_dot_root.at(i);
      ProcessSingleFile(filewithpath.c_str(),outCSV,nvar,isRoot,reqPressure,cutBase);
      
    }       
  
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

  if (isRoot) {
    foutRoot->cd();
    outputTree->Write();
    foutRoot->Close();
  }
  else{
   outCSV.close();
 }

  std::cout << outname << '\n';
  std::cout.flush();
  _exit(0);
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

bool ProcessSingleFile(const char *filename,std::ofstream &outCSV,Int_t nvar,bool isRoot,bool reqPressure,TString &cut){
  TFile fin(filename);
  TTree *tree = (TTree *) fin.Get("Events");
  if(!tree) return 1;
  TTree *chain = tree->CopyTree(cut.Data());

  TTree* weather = (TTree *) fin.Get("Weather");
  weather->GetEvent(0);
  Float_t pressure = weather->GetLeaf("Pressure")->GetValue();

  Long64_t const nmaxentr = 12500000 / nvar;

  if(ntotevent > nmaxentr) return 1;

  int nentries = chain->GetEntriesFast();

  if(ntotevent + nentries > nmaxentr) nentries = nmaxentr - ntotevent;

  if (isRoot) {
    for (int i = 0; i != nentries; ++i) {
      chain->GetEvent(i);
      ntotevent++;
      Float_t const xd = chain->GetLeaf("XDir")->GetValue();
      Float_t const yd = chain->GetLeaf("YDir")->GetValue();
      Float_t const zd = chain->GetLeaf("ZDir")->GetValue();
      
      for (int j = 0; j != nvar; ++j) {
        if (var[j].Contains("Pressure")) {
          fvar[j] = pressure;
        } 
        else if (var[j].Contains("Theta")) {
          fvar[j] = TMath::ACos(zd) * TMath::RadToDeg();
        } 
	else if (var[j].Contains("Phi")) {
          fvar[j] = TMath::ATan2(yd, xd) * TMath::RadToDeg();
        }
	else {
          if (isInteger[j]) {
            ivar[j] = chain->GetLeaf(var[j].Data())->GetValue();
          } else {
            fvar[j] = chain->GetLeaf(var[j].Data())->GetValue();
          }
        }
      }
      outputTree->Fill();
    }
  }
  
  else {
    for (int i = 0; i != nentries; ++i) {
      chain->GetEvent(i);
      ntotevent++;
      
      Float_t const xd = chain->GetLeaf("XDir")->GetValue();
      Float_t const yd = chain->GetLeaf("YDir")->GetValue();
      Float_t const zd = chain->GetLeaf("ZDir")->GetValue();
      
      for (int j = 0; j != nvar - 1; ++j) {
	if (var[j].Contains("Pressure")) {
	  outCSV << int(pressure) << ',';
	} else if (var[j].Contains("Theta")) {
	  outCSV << TMath::ACos(zd) * TMath::RadToDeg() << ',';
	} else if (var[j].Contains("Phi")) {
	  outCSV << TMath::ATan2(yd, xd) * TMath::RadToDeg() << ',';
	} else {
	  if (isInteger[j]) {
	    outCSV << static_cast<int64_t>(chain->GetLeaf(var[j].Data())->GetValue()) << ',';
	  } else {
	    outCSV << chain->GetLeaf(var[j].Data())->GetValue() << ',';
	  }
	}
      }
	
      if (var[nvar-1].Contains("Pressure")) {
        outCSV << int(pressure) << '\n';
      } else if (var[nvar-1].Contains("Theta")) {
        outCSV << TMath::ACos(zd) * TMath::RadToDeg() << '\n';
      } else if (var[nvar-1].Contains("Phi")) {
        outCSV << TMath::ATan2(yd, xd) * TMath::RadToDeg() << '\n';
      } 
      else {
        if (isInteger[nvar-1]) {
          outCSV << static_cast<int64_t>(chain->GetLeaf(var[nvar-1].Data())->GetValue()) << '\n';
        } else {
          outCSV << chain->GetLeaf(var[nvar-1].Data())->GetValue() << '\n';
        }
      }
    }
  }
  
  if(chain) delete chain;
}
