// Functions to locate files and test their existence
#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <TString.h>
#include <stdlib.h>
#include <fstream>
#include <sys/stat.h>

bool exists(TString path){ struct stat buffer; return (stat (path.Data(), &buffer) == 0);};

void makeDirectory(TString path){
  if(!exists(path)) system("mkdir -p " + path);
}

TString getProduction(){
  return "2013-06-JetIDfix";
}

TString getCMSSWBASE(){
  if(!exists(".myCMSSW_BASE.txt"))system("echo $CMSSW_BASE > .myCMSSW_BASE.txt");
  std::ifstream readFile;
  readFile.open(".myCMSSW_BASE.txt");
  TString base;
  readFile >> base;
  readFile.close();
  system("rm .myCMSSW_BASE.txt");
  return base;
}

TString getHost(){
  if(!exists(".myHOST.txt")) system("echo $HOSTNAME > .myHOST.txt");
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

#endif
