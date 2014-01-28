// Functions to locate files and test their existence
#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <TFile.h>
#include <TH1D.h>
#include <TProfile.h>
#include <TString.h>
#include <TMath.h>

bool exists(TString path){ struct stat buffer; return (stat (path.Data(), &buffer) == 0);};
void makeDirectory(TString path){ if(!exists(path)) system("mkdir -p " + path);}

TString getProduction(){ return "2013-12";}

TString getStringFromFile(TString fileName){
  std::ifstream readFile; 
  readFile.open(fileName);
  TString stringInFile;
  readFile >> stringInFile;
  readFile.close();
  return stringInFile;
}

TString getCMSSWBASE(){
  while(!exists(".myCMSSW_BASE.txt")) system("echo $CMSSW_BASE > .myCMSSW_BASE.txt");
  return getStringFromFile(".myCMSSW_BASE.txt") + "/";
}

TString getHost(){
  while(!exists(".myHOST.txt")) system("echo $HOSTNAME > .myHOST.txt");
  TString host = getStringFromFile(".myHOST.txt");
  if(host.Contains("iihe")) return "iihe";
  if(host.Contains("gridui")) return "infn";
  if(host.Contains("lxplus")) return "lxplus";
  return host;
}

TString getTreeLocation(){
  TString host = getHost();
  TString location = "UNDEFINED";
  if(host == "iihe") location = "/user/tomc/public/merged/EWKV/";
  if(host == "infn") location = "/gpfs/gpfsddn/cms/user/EWKV/";
  if(host == "lxplus") location = "/afs/cern.ch/work/t/tomc/public/EWKV/";
  location += getProduction() + "/";
  return location;
}

bool getStream(std::ifstream &readFile, TString path, bool ignoreNonExist = false){
  if(exists(path)){
    readFile.open(path); 
    if(readFile.is_open()) return true;
  }
  std::cout << ("environment.h:\t\t!!!\tCould not open " + path + "!") << std::endl;
  if(ignoreNonExist) return false;
  else exit(1);
}

bool getLine(std::ifstream &readFile, std::stringstream &streamLine){
  std::string line;
  bool success = std::getline(readFile, line);
  streamLine.str("");
  streamLine.clear();
  streamLine << line;
  return success;
}


std::vector<TString> typeSelector(int argc, char *argv[]){
  std::vector<TString> types {"ZEE","ZMUMU"};
  if(argc > 1){
    if(((TString) argv[1]) == "ZEE") types = {"ZEE"};
    else if(((TString) argv[1]) == "ZMUMU") types = {"ZMUMU"};
    else if(((TString) argv[1]) != "ALL"){ std::cout << "environment.h:\t\t\tType not known" << std::endl; exit(1);}
  } else std::cout << "environment.h:\t\t\tAutomatic type selector (ALL)" << std::endl;
  return types;
}

TH1D* addErrorToHist(TH1D *h, double sigma){
  if(sigma == 0.) return h;
  for(int bin = 0; bin <= h->GetXaxis()->GetNbins() + 1; ++bin) h->SetBinContent(bin, h->GetBinContent(bin) + h->GetBinError(bin)*sigma);
  return h;
}

TH1D* getPlot(TFile *file, TString sample, TString plot, bool ignoreNonExist = false, double addError = 0.){
  if(TH1D *hist = (TH1D*) file->Get(sample + "/" + plot)) if(hist->GetEntries() != 0) return addErrorToHist((TH1D*) hist->Clone(), addError);
  if(TH1D *hist = (TH1D*) file->Get(plot + "_" + sample)) if(hist->GetEntries() != 0) return addErrorToHist((TH1D*) hist->Clone(), addError);
  if(!ignoreNonExist){
    std::cout << ("environment.h:\t\t!!!\t" + sample + "/" + plot + " not found!") << std::endl;
    exit(1);
  } else return nullptr;
}

TProfile* getProfile(TFile *file, TString sample, TString plot, bool ignoreNonExist = false){
  if(TProfile *hist = (TProfile*) file->Get(sample + "/" + plot)) if(hist->GetEntries() != 0) return (TProfile*) hist->Clone();
  if(TProfile *hist = (TProfile*) file->Get(plot + "_" + sample)) if(hist->GetEntries() != 0) return (TProfile*) hist->Clone();
  if(!ignoreNonExist){
    std::cout << (sample + "/" + plot + " not found!") << std::endl;
    exit(1);
  } else return nullptr;
}

TH1D* safeAdd(TH1D *first, TH1D *second){
  if(!second) return first;
  if(!first) return second;
  TH1D* merged = (TH1D*) first->Clone();
  merged->Add(second);
  return merged;
}

TProfile* safeAdd(TProfile *first, TProfile *second){
  if(!second) return first;
  if(!first) return second;
  TProfile* merged = (TProfile*) first->Clone();
  merged->Add(second);
  return merged;
}

bool isLogX(TH1 *h){ return (h->GetBinWidth(1) != h->GetBinWidth(2));}
bool isLogX(TAxis *axis){ return (axis->GetBinWidth(1) != axis->GetBinWidth(2));}

TH1* displaceBins(TH1* hh, double displaceFactor){
  TH1* h = (TH1*) hh->Clone();
  TAxis *axis = h->GetXaxis();
  int bins = axis->GetNbins();
  Axis_t logWidth = (TMath::Log10(axis->GetXmax())-TMath::Log10(axis->GetXmin()))/bins;
  Axis_t width = (axis->GetXmax() - axis->GetXmin())/bins;
  Axis_t *new_bins = new Axis_t[bins + 1];
  for(int i = 0; i <= bins; i++){
    if(isLogX(hh)) new_bins[i] = TMath::Power(10, TMath::Log10(axis->GetXmin()) + logWidth*(i+displaceFactor)); 
    else new_bins[i] = axis->GetXmin() + width*(i+displaceFactor);
  }
  axis->Set(bins, new_bins);
  for(int i = 1; i <= bins; i++) h->Fill(h->GetBinCenter(i), hh->GetBinContent(i));
  delete [] new_bins;
  return h;
}

void fixRange(TH1* a, TH1 *b){
  if(b->GetMaximum()*1.1 > a->GetMaximum()) a->SetMaximum(b->GetMaximum()*1.1);
  if(b->GetMinimum()*.9 < a->GetMinimum())  a->SetMinimum(b->GetMinimum()*.9);
}

#endif
