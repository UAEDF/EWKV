#ifndef HISTOS_H
#define HISTOS_H

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <TROOT.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TCanvas.h>
#include <TMarker.h>
#include <TFile.h>
#include <TTree.h>
#include <TChain.h>
#include <TH1D.h>
#include <THStack.h>
#include <TMath.h>

#include "../samples/sampleList.h"
#include "../samples/sample.h"
#include "../environment.h"


class histoCollection{
  private:
  std::map<TString, TH1D*> hist1DList;
  std::map<TString, TH2D*> hist2DList;
  std::map<TString, TProfile*> profileList;
  std::vector<TString> onBlackList;
  double fillWeight;
  TString branch;
  TFile* outputFile;
  sample* mySample;


  public:
  histoCollection(sample* mySample_, TFile* outputFile);
  ~histoCollection();

  void bookHistos();
  void bookHist1D(TString hName, int Xbins, double Xmin, double Xmax, bool logX = false);
  void bookHist2D(TString hName, int Xbins, double Xmin, double Xmax, bool logX, int Ybins, double Ymin, double Ymax, bool logY);
  void bookProfileHist(TString hName, int Xbins, double Xmin, double Xmax, bool logX = false);

  TH1D* makeBranch1D(TH1D* h);
  TH2D* makeBranch2D(TH2D* h);
  TProfile* makeBranchProfile(TProfile* h);

  void fillHist1D(TString hName, double Xvalue);
  void fillHist2D(TString hName, double Xvalue, double Yvalue);
  void fillProfileHist(TString hName, double Xvalue, double Yvalue);

  void toFile(std::vector<TString> selectedHists = std::vector<TString>(0)); 

  void setWeight(double weight){		fillWeight = weight;};
  void setBranch(TString branch_ = ""){		branch = branch_;};

  void binLogAxis(TAxis *axis);
  void binLogX(TH1* h){ binLogAxis(h->GetXaxis());}
};


histoCollection::histoCollection(sample* mySample_, TFile* outputFile_){
  gROOT->SetStyle("Plain");
  gStyle->SetErrorX(0);
  TH1::SetDefaultSumw2();
  mySample = mySample_;
  outputFile = outputFile_;
  fillWeight = 0;
  branch = "";
  bookHistos();
}


histoCollection::~histoCollection(){
  for(auto it = hist1DList.begin(); it != hist1DList.end(); ++it) delete it->second;
  for(auto it = hist2DList.begin(); it != hist2DList.end(); ++it) delete it->second;
  for(auto it = profileList.begin(); it != profileList.end(); ++it) delete it->second;
}


void histoCollection::bookHistos(){
  std::ifstream readFile;
  std::stringstream line;
  getStream(readFile, getCMSSWBASE() + "/src/EWKV/Macros/histos/1D.config"); 
  while(getLine(readFile, line)){
    TString useLine;
    line >> useLine;
    if(!useLine.IsDigit()) continue;
    TString name;
    int Xbins;
    double Xmin, Xmax;
    bool logX;
    line >> name >> Xbins >> Xmin >> Xmax >> logX;
    bookHist1D(name, Xbins, Xmin, Xmax, logX);
  }
  readFile.close();

  getStream(readFile, getCMSSWBASE() + "/src/EWKV/Macros/histos/2D.config"); 
  while(getLine(readFile, line)){
    TString useLine;
    line >> useLine;
    if(!useLine.IsDigit()) continue;
    TString name;
    int Xbins, Ybins;
    double Xmin, Xmax, Ymin, Ymax;
    bool logX, logY;
    line >> name >> Xbins >> Xmin >> Xmax >> logX >> Ybins >> Ymin >> Ymax >> logY;
    bookHist2D(name, Xbins, Xmin, Xmax, logX, Ybins, Ymin, Ymax, logY);
  }
  readFile.close();

  getStream(readFile, getCMSSWBASE() + "/src/EWKV/Macros/histos/profile.config");
  while(getLine(readFile, line)){
    TString useLine;
    line >> useLine;
    if(!useLine.IsDigit()) continue;
    TString name;
    int Xbins;
    double Xmin, Xmax;
    bool logX;
    line >> name >> Xbins >> Xmin >> Xmax >> logX;
    bookProfileHist(name, Xbins, Xmin, Xmax, logX);
  }
  readFile.close();
  return;
}


void histoCollection::bookHist1D(TString hName, int Xbins, double Xmin, double Xmax, bool logX){
  outputFile->cd();
  hist1DList[hName] = new TH1D(hName, hName, Xbins, Xmin, Xmax);
  if(logX) binLogX(hist1DList[hName]);
}

TH1D* histoCollection::makeBranch1D(TH1D* h){
  outputFile->cd();
  hist1DList[h->GetName()+branch] = new TH1D(h->GetName() + TString("_") + branch, h->GetTitle(), h->GetNbinsX(), h->GetXaxis()->GetXmin(), h->GetXaxis()->GetXmax());
  if(isLogX(h)) binLogX(hist1DList[h->GetName()+branch]);
}


void histoCollection::bookHist2D(TString hName, int Xbins, double Xmin, double Xmax, bool logX, int Ybins, double Ymin, double Ymax, bool logY){
  outputFile->cd();
  hist2DList[hName] = new TH2D(hName, hName, Xbins, Xmin, Xmax, Ybins, Ymin, Ymax);
  if(logX) binLogAxis(hist2DList[hName]->GetXaxis());
  if(logY) binLogAxis(hist2DList[hName]->GetYaxis());
}

TH2D* histoCollection::makeBranch2D(TH2D* h){
  outputFile->cd();
  hist2DList[h->GetName()+branch] = new TH2D(h->GetName() + TString("_") + branch, h->GetTitle(), 
						 h->GetNbinsX(), h->GetXaxis()->GetXmin(), h->GetXaxis()->GetXmax(),
						 h->GetNbinsY(), h->GetYaxis()->GetXmin(), h->GetYaxis()->GetXmax());
  if(isLogX(h->GetXaxis())) binLogAxis(hist2DList[h->GetName()+branch]->GetXaxis());
  if(isLogX(h->GetYaxis())) binLogAxis(hist2DList[h->GetName()+branch]->GetYaxis());
}


void histoCollection::bookProfileHist(TString hName, int Xbins, double Xmin, double Xmax, bool logX){
  outputFile->cd();
  profileList[hName] = new TProfile(hName, hName, Xbins, Xmin, Xmax);
  if(logX) binLogX(profileList[hName]);
}

TProfile* histoCollection::makeBranchProfile(TProfile* h){
  outputFile->cd();
  profileList[h->GetName()+branch] = new TProfile(h->GetName() + TString("_") + branch, h->GetTitle(), h->GetNbinsX(), h->GetXaxis()->GetXmin(), h->GetXaxis()->GetXmax());
  if(isLogX(h)) binLogX(profileList[h->GetName()+branch]);
}


void histoCollection::fillHist1D(TString hName, double Xvalue){
  auto hid = hist1DList.find(hName+branch);
  if(hid == hist1DList.end()){
    auto hidMainBranch = hist1DList.find(hName);
    if(hidMainBranch == hist1DList.end()){
      if(std::find(onBlackList.begin(), onBlackList.end(), hName) != onBlackList.end()) return;
      std::cout << "histoCollection:\t!!!\tTH1D " << hName << " was not booked" << std::endl;
      onBlackList.push_back(hName);
      return;
    } else { 
      makeBranch1D(hidMainBranch->second);
      hid = hist1DList.find(hName+branch);
    }
  }
  hid->second->Fill(Xvalue, fillWeight);
}


void histoCollection::fillHist2D(TString hName, double Xvalue, double Yvalue){
  auto hid = hist2DList.find(hName+branch);
  if(hid == hist2DList.end()){
    auto hidMainBranch = hist2DList.find(hName);
    if(hidMainBranch == hist2DList.end()){
      if(std::find(onBlackList.begin(), onBlackList.end(), hName) != onBlackList.end()) return;
      std::cout << "histoCollection:\t!!!\tTH2D " << hName << " was not booked" << std::endl;
      onBlackList.push_back(hName);
      return;
    } else {
      makeBranch2D(hidMainBranch->second);
      hid = hist2DList.find(hName+branch);
    }
  }
  hid->second->Fill(Xvalue, Yvalue, fillWeight);
}


void histoCollection::fillProfileHist(TString hName, double Xvalue, double Yvalue){
  auto hid = profileList.find(hName + branch);
  if(hid == profileList.end()){
    auto hidMainBranch = profileList.find(hName);
    if(hidMainBranch == profileList.end()){
      if(std::find(onBlackList.begin(), onBlackList.end(), hName) != onBlackList.end()) return;
      std::cout << "histoCollection:\t!!!\tTProfile " << hName << " was not booked" << std::endl;
      onBlackList.push_back(hName);
      return;
    } else {
      makeBranchProfile(hidMainBranch->second);
      hid = profileList.find(hName+branch);
    }
  }
  hid->second->Fill(Xvalue, Yvalue, fillWeight);
}


void histoCollection::binLogAxis(TAxis *axis){
  if(axis->GetXmin() == 0) std::cout << "histoCollection:\t!!!\tLogX is not compatible with x_min = 0" << std::endl;
  int bins = axis->GetNbins();
  Axis_t logMin = TMath::Log10(axis->GetXmin());
  Axis_t logMax = TMath::Log10(axis->GetXmax());
  Axis_t logWidth = (logMax-logMin)/bins;
  Axis_t *new_bins = new Axis_t[bins + 1];
  for(int i = 0; i <= bins; i++) new_bins[i] = TMath::Power(10, logMin + i*logWidth);
  axis->Set(bins, new_bins);
  axis->SetMoreLogLabels(); axis->SetNoExponent();
  delete [] new_bins;
}


void histoCollection::toFile(std::vector<TString> selectedHists){
  outputFile->cd();
  TDirectory *dir = outputFile->mkdir(mySample->getName());
  dir->cd();
  for(auto hid = hist1DList.begin(); hid != hist1DList.end(); ++hid){
    if(!selectedHists.empty() && std::find(selectedHists.begin(), selectedHists.end(), hid->first) == selectedHists.end()) continue;
    hid->second->SetName(hid->first);
    hid->second->Write();
  }
  for(auto hid = hist2DList.begin(); hid != hist2DList.end(); ++hid){
    if(!selectedHists.empty() && std::find(selectedHists.begin(), selectedHists.end(), hid->first) == selectedHists.end()) continue;
    hid->second->SetName(hid->first);
    hid->second->Write();
  }
  for(auto hid = profileList.begin(); hid != profileList.end(); ++hid){
    if(!selectedHists.empty() && std::find(selectedHists.begin(), selectedHists.end(), hid->first) == selectedHists.end()) continue;
    hid->second->SetName(hid->first);
    hid->second->Write();
  }
  std::cout << "HistoCollection:\t\tHistos from " << mySample->getName() << " saved in " << outputFile->GetName() << std::endl;
}
#endif
