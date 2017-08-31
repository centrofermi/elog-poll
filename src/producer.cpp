#include<stdlib.h>
#include "TFile.h"
#include "TChain.h"
#include "TString.h"
#include "TLeaf.h"
#include "TMath.h"

char *NextDay(const char *currentday);
bool IsInRange(const char *currentday,const char *lastday);

int main(){
  bool isRoot = 1;
  bool isTXT = !isRoot;

  const Int_t nvar = 3;
  TString var[nvar] = {"Seconds","Theta","Phi"};
  const char *type[nvar] = {"I","F","F"};

  void *addvar[nvar];

  Int_t ivar[nvar];
  Float_t fvar[nvar];

  for(Int_t i=0;i < nvar;i++){
    if(type[i] == "I") addvar[i] = &(ivar[i]);
    else addvar[i] = &(fvar[i]);
  }

  TString cutBase("(StatusCode==0)&&");
  TString cutMy("(1)");
  cutBase += cutMy;

  Int_t id=4;
  const char *pathToRecon="/recon2";
  const char *school="SAVO-01";
  const char *dateIn="2017-05-11";
  const char *dateOut="2017-05-12";

  const char *currentday = dateIn;


  Int_t ndays = 0;

  while(IsInRange(currentday,dateOut)){
    if(ndays==0) system(Form("ls %s/%s/%s/%s*.root >/tmp/%s-%s_%i.lst",pathToRecon,school,currentday,school,school,dateIn,id));
    else system(Form("ls %s/%s/%s/%s*.root >>/tmp/%s-%s_%i.lst",pathToRecon,school,currentday,school,school,dateIn,id));

    ndays++;
    
    currentday = NextDay(currentday);
  }

  if(ndays==0) return 1;

  char filerun[200];

  FILE *flist = fopen(Form("/tmp/%s-%s_%i.lst",school,dateIn,id),"r");
  if(!flist) return 2;

  Int_t nfile=0;
  TChain *chain = new TChain("Events");

  while(fscanf(flist,"%s",filerun) == 1){
    chain->AddFile(filerun);
    nfile++;
  }
  fclose(flist);

  if(!nfile) return 3;
  if(!chain->GetEntries()) return 4;
 
  // minimal info
  chain->SetBranchStatus("*",0);
  chain->SetBranchStatus("StatusCode",1);
  chain->SetBranchStatus("XDir",1);
  chain->SetBranchStatus("YDir",1);
  chain->SetBranchStatus("ZDir",1);

  for(Int_t j=0;j < nvar;j++){
    if(!(var[j].Contains("Theta") || var[j].Contains("Phi")))
      chain->SetBranchStatus(var[j].Data(),1);
  }

  TTree *workingtree = chain->CopyTree(cutBase.Data());

  TTree *outputTree = new TTree("eee","eee");
  for(Int_t j=0;j < nvar;j++)
    outputTree->Branch(var[j].Data(),addvar[j],Form("%s/%s",var[j].Data(),type[j]));

  Float_t xd,yd,zd;

  char *outname = Form("/tmp/%sfrom%sto%s_%i.csv",school,dateIn,dateOut,id);

  FILE *foutCSV = fopen(outname,"w");
  for(Int_t j=0;j < nvar;j++){
    if(j>0) fprintf(foutCSV,",");
    fprintf(foutCSV,"%s",var[j].Data());
  }
  fprintf(foutCSV,"\n");

  for(Int_t i=0;i < workingtree->GetEntries();i++){
    workingtree->GetEvent(i);

    xd = workingtree->GetLeaf("XDir")->GetValue();
    yd = workingtree->GetLeaf("YDir")->GetValue();
    zd = workingtree->GetLeaf("ZDir")->GetValue();

    for(Int_t j=0;j < nvar;j++){
      if(j>0) fprintf(foutCSV,",");

      if(!(var[j].Contains("Theta") || var[j].Contains("Phi"))){
	if(type[j] == "I") ivar[j] = workingtree->GetLeaf(var[j].Data())->GetValue();
	else fvar[j] = workingtree->GetLeaf(var[j].Data())->GetValue();
      }
      else if(var[j].Contains("Theta")){
	fvar[j] = TMath::ACos(zd)*TMath::RadToDeg();
      }
      else if(var[j].Contains("Phi")){
	fvar[j] = TMath::ATan2(yd,xd)*TMath::RadToDeg();
	if(fvar[j] < 0) fvar[j] += 360;
      }

      if(isTXT){
	if(type[j] == "I") fprintf(foutCSV,"%i",ivar[j]);
	else fprintf(foutCSV,"%f",fvar[j]);
      }

    }
    if(isRoot)
      outputTree->Fill();
    if(isTXT)
      fprintf(foutCSV,"\n");
  }

  fclose(foutCSV);

  if(isTXT){
    printf("%s\n",outname);
  }

  if(isRoot){
    outname = Form("/tmp/%sfrom%sto%s_%i.root",school,dateIn,dateOut,id);
    TFile *foutRoot = new TFile(outname,"RECREATE");
    outputTree->Write();
    foutRoot->Close();
    printf("%s\n",outname);
  }
 
  return 0;
}

char *NextDay(const char *currentday){
  Int_t dayPerMonth[12]={31,28,31,30,31,30,31,31,30,31,30,31};

  Int_t y,m,d;
  sscanf(currentday,"%4i-%2i-%2i",&y,&m,&d);
  if(!(y % 4)) dayPerMonth[1]=29;

  d++;
  if(d >dayPerMonth[m-1]){
    d=1;
    m++;
  }

  if(m>12){
    m=1;
    y++;
  }

  return Form("%i-%02i-%02i",y,m,d);
}

bool IsInRange(const char *currentday,const char *lastday){
  Int_t y,m,d;
  sscanf(currentday,"%4i-%2i-%2i",&y,&m,&d);
  Int_t y2,m2,d2;
  sscanf(lastday,"%4i-%2i-%2i",&y2,&m2,&d2);

  if(y2 > y) return 1;
  else if(y2 < y) return 0;

  if(m2 > m) return 1;
  else if(m2 < m) return 0;

  if(d2 >= d) return 1;

  return 0;
}
