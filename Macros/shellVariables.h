#ifndef SHELLVARIABLES_H
#define SHELLVARIABLES_H

#include <TString.h>
#include <stdlib.h>
#include <fstream>
#include <sys/stat.h>

TString getCMSSWBASE(){
  system("echo $CMSSW_BASE > .myCMSSW_BASE.txt");
  std::ifstream readFile;
  readFile.open(".myCMSSW_BASE.txt");
  TString base;
  readFile >> base;
  readFile.close();
  system("rm .myCMSSW_BASE.txt");
  return base;
}

TString getHost(){
  system("echo $HOSTNAME > .myHOST.txt");
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

bool exists(TString path){ struct stat buffer; return (stat (path.Data(), &buffer) == 0);};

#endif
