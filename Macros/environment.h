// Functions to locate files and test their existence
#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <TString.h>
#include <stdlib.h>
#include <fstream>
#include <sys/stat.h>
#include <TH1D.h>
#include <TProfile.h>

bool exists(TString path){ struct stat buffer; return (stat (path.Data(), &buffer) == 0);};

void makeDirectory(TString path){
  if(!exists(path)) system("mkdir -p " + path);
}

TString getProduction(){
  return "2013-06-JetIDfix";
}

TString getCMSSWBASE(){
  while(!exists(".myCMSSW_BASE.txt")) system("echo $CMSSW_BASE > .myCMSSW_BASE.txt");
  std::ifstream readFile;
  readFile.open(".myCMSSW_BASE.txt");
  TString base;
  readFile >> base;
  readFile.close();
  return base + "/";
}

TString getHost(){
  while(!exists(".myHOST.txt")) system("echo $HOSTNAME > .myHOST.txt");
  std::ifstream readFile;
  readFile.open(".myHOST.txt");
  TString host;
  readFile >> host;
  readFile.close();
  if(host.Contains("iihe")) return "iihe";
  if(host.Contains("gridui")) return "infn";
  if(host.Contains("lxplus")) return "lxplus";
  return host;
}

TString getTreeLocation(){
  TString host = getHost();
  TString location;
  if(host == "iihe") location = "/user/tomc/public/merged/EWKV/";
  if(host == "infn") location = "/gpfs/gpfsddn/cms/user/EWKV/";
  if(host == "lxplus") location = "/afs/cern.ch/work/t/tomc/public/EWKV/";
  location += getProduction() + "/";
  return location;
}

TH1D* getPlot(TFile *file, TString sample, TString plot, bool ignoreNonExist = false){
  if(TH1D *hist = (TH1D*) file->Get(sample + "/" + plot)) return (TH1D*) hist->Clone();
  if(TH1D *hist = (TH1D*) file->Get(plot + "_" + sample)) return (TH1D*) hist->Clone();
  if(!ignoreNonExist){
    std::cout << sample << "/" << plot << " not found!" << std::endl;
    exit(1);
  } else return NULL;
}

TProfile* getProfile(TFile *file, TString sample, TString plot, bool ignoreNonExist = false){
  if(TProfile *hist = (TProfile*) file->Get(sample + "/" + plot)) return (TProfile*) hist->Clone();
  if(TProfile *hist = (TProfile*) file->Get(plot + "_" + sample)) return (TProfile*) hist->Clone();
  if(!ignoreNonExist){
    std::cout << sample << "/" << plot << " not found!" << std::endl;
    exit(1);
  } else return NULL;
}

void safeAdd(TH1D *first, TH1D *second, TH1D *merged = NULL){
  if(!merged) merged = first;
  if(!second){ merged = first; return;}
  if(!first){ merged = second; return;}
  merged->Add(first, second);
}
#endif
