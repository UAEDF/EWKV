#ifndef CMSSWBASE_H
#define CMSSWBASE_H

#include <TString.h>
#include <stdlib.h>
#include <fstream>

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

#endif
