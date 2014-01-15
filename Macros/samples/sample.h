#ifndef SAMPLE_H
#define SAMPLE_H

#include <fstream>
#include "TFile.h"
#include "TTree.h"
#include "TChain.h"
#include "TH1.h"
#include "TLorentzVector.h"

#include "../environment.h"

/****************
 * Class sample *
 ****************/
class sample{
  protected:
    sample(TString name_);
    TString name, type, tag;
    double lumi, lumiWeight;
    bool skimmed;

  public:
    virtual TTree* getTree();
    virtual bool isData() = 0;
    virtual double getPileUpWeight(int nPileUp, TString puMode){				return 1.;};
    virtual double getWeight(int nPileUp, TString puMode){					return 1.;};
    virtual double muonEfficiency(TLorentzVector *l, TLorentzVector *l2 = nullptr) { 	 	return 1.;};
    virtual double electronEfficiency(TLorentzVector *l, TLorentzVector *l2 = nullptr) { 	return 1.;};

    TString getName(){										return name;};
    double getLumi(){										return lumi;};
    void setLumiWeight(double weight){  							lumiWeight = weight;};

    void useSkim(TString type_, TString tag_){ skimmed = true; type = type_; tag = tag_;};
};

sample::sample(TString name_){
  name = name_;
  skimmed = false;
  type == "";
  tag == "";
}

TTree* sample::getTree(){
  TString fileName = getTreeLocation() + "ewkv_" + name + ".root";
  if(skimmed){
    TString skimmedFileName = getTreeLocation() + "skimmed/" + type + "/" + tag + "/" + name + ".root";
    if(exists(skimmedFileName)) fileName = skimmedFileName;
    else std::cout << "sample:\t\t\t!!!\tNo skimmed file for " << name << ", full root tree is taken" << std::endl;
  }
  TFile* file = new TFile(fileName);
  if(file->IsZombie()){ std::cout << "sample:\t\t\tERROR\tCould not find "<< fileName << std::endl; exit(1);}
  TTree* tree = (TTree*) file->Get("EWKV");
  return tree;
}


/*****************
 * Class dataRun *
 *****************/
class dataRun : public sample{
  private:
    TString JSON;

  public:
    dataRun(TString name_, double lumi_);
    bool isData(){ 				return true;};
    TString getJSON(){ 				return JSON;};
};

dataRun::dataRun(TString name_, double lumi_) : sample(name_){
  lumi 		= lumi_;
  JSON		= getCMSSWBASE() + "src/EWKV/Crab/JSON/lumiSummary_" + name + ".json";
}


/********************
 * Class dataSample *
 ********************/
class dataSample : public dataRun{
  private:
    std::vector<dataRun*> runs;
    TString mergeString;

  public:
    dataSample(TString name_, std::vector<dataRun*> runs_);
    TTree* getTree();
    TString getMergeString(){ return mergeString;};
};

dataSample::dataSample(TString name_, std::vector<dataRun*> runs_) : dataRun(name_, 0){
  runs = runs_;
  mergeString = "";
  for(auto run = runs.begin(); run != runs.end(); ++run){
    lumi += (*run)->getLumi();
    mergeString += "_" + (*run)->getName();
  }
  std::cout << "dataSample:\t\t\t" << name << " with total lumi = " << lumi << "/pb" << std::endl;
}

TTree* dataSample::getTree(){
  if(skimmed){
    TFile* file = new TFile(getTreeLocation() + "skimmed/" + type + "/" + tag + "/data.root");
    if(file->IsZombie()){
      std::cout << "sample:\t\t\t!!!\tNo skimmed file for data, full root tree is taken" << std::endl;
    } else {
      TTree* tree = (TTree*) file->Get("EWKV");
      return tree; 
    }
  }
  TChain *chain = new TChain("EWKV");
  for(auto run = runs.begin(); run != runs.end(); ++run){
    if(!chain->Add(getTreeLocation() + "ewkv_" + (*run)->getName() + ".root", -1)){
      std::cout << "dataSample:\t\tERROR\tCouldn't add ewkv_" << (*run)->getName() + ".root to the tree" << std::endl;
      exit(1);
    }
  }
  return (TTree*) chain;
}


/******************
 * Class mcSample *
 ******************/
class mcSample : public sample{
  private:
    std::map<TString, std::vector<double>> weights;
    std::map<TString, TGraphAsymmErrors*> muonEfficiencies;
    TH2D* electronEfficiencies;
    double crossSection;
    int nEvents;

    double muonEfficiency(TString type, double pt, double eta);

  public:
    mcSample(TString name_, double crossSection_, int nEvents_, std::map<TString, TGraphAsymmErrors*> efficiencies_, TH2D* electronEfficiencies_);
    bool setPileUpWeights(TString pileUpWeightsFile, TString puMode);
    TH1* getPileUpHisto();
 
    bool isData(){ return false;};
    double getPileUpWeight(int nPileUp, TString puMode){ return (nPileUp < weights["*"].size() && nPileUp >= 0) ? weights[(puMode == "" ? "*" : puMode)].at(nPileUp) : 0;};
    double muonEfficiency(TLorentzVector *l, TLorentzVector *l2 = nullptr);
    double electronEfficiency(TLorentzVector *l, TLorentzVector *l2 = nullptr);
    double getWeight(int nPileUp, TString puMode){ return getPileUpWeight(nPileUp, puMode)*lumiWeight;};
};

mcSample::mcSample(TString name_, double crossSection_, int nEvents_, std::map<TString, TGraphAsymmErrors*> muonEfficiencies_, TH2D* electronEfficiencies_) : sample(name_){
  crossSection 		= crossSection_;
  nEvents 		= nEvents_;
  muonEfficiencies  	= muonEfficiencies_;
  electronEfficiencies  = electronEfficiencies_;
  lumi 			= (double)nEvents/(double)crossSection;

  std::cout << "mcSample:\t\t\t" << name << " added (lumi = " << lumi << "/pb)" << std::endl;
}

TH1* mcSample::getPileUpHisto(){
  TFile* file = new TFile(getTreeLocation() + "pileUp_" + name + ".root");
  if(file->IsZombie()){ std::cout << "mcSample:\t\tERROR\tCould not find "<< (getTreeLocation() + "pileUp_" + name + ".root") << std::endl; exit(1);}
  TH1* hist; file->GetObject("pileUp", hist);
  if(!hist){ std::cout << "mcSample:\t\tERROR\tNo pileUp histogram in file " << (getTreeLocation() + "pileUp_" + name + ".root") << std::endl; exit(1);}
  return hist;
}

bool mcSample::setPileUpWeights(TString pileUpWeightsFile, TString puMode){
  weights[puMode] = std::vector<double>(101, 1); 
  std::ifstream readFile;
  if(!getStream(readFile, pileUpWeightsFile.Data(), true)) return false;
  std::stringstream line;
  while(getLine(readFile, line)){
    if(!TString(line.str()) == name) continue;
    getLine(readFile, line);
    weights[puMode].clear();
    double weight = 0;
    while(line >> weight) weights[puMode].push_back(weight);
    readFile.close();
    return true;
  }
  readFile.close();
  return false;
}

double mcSample::muonEfficiency(TLorentzVector *l, TLorentzVector *l2){
  if(!l) return 1.;
  return muonEfficiency("iso", l->Pt(), l->Eta())*muonEfficiency("id", l->Pt(), l->Eta())*muonEfficiency(l2);
}

double mcSample::muonEfficiency(TString type, double pt, double eta){ 
  TString etaBin;
  if(fabs(eta) < .9) etaBin = "<0.9";
  else if(fabs(eta) < 1.2) etaBin = "0.9-1.2";
  else if(fabs(eta) < 2.1) etaBin = "1.2-2.1";
  else if(fabs(eta) <= 2.4) etaBin = "2.1-2.4";
  else return 0.;
  if(muonEfficiencies.find(type + etaBin) == muonEfficiencies.end()){
    std::cout << "mcSample\t\t!!! No " << type << " efficiency found for eta=" << etaBin<< std::endl;
    return 1.;
  }
  double efficiency, ptlow, pthigh, ptbin;
  int i = 0;
  do{
    muonEfficiencies[type + etaBin]->GetPoint(i, ptbin, efficiency);
    ptlow = ptbin - muonEfficiencies[type + etaBin]->GetErrorXlow(i);
    pthigh = ptbin + muonEfficiencies[type + etaBin]->GetErrorXhigh(i++);
  } while((pthigh < pt) && (i < muonEfficiencies[type + etaBin]->GetN()));
  return efficiency;
}

double mcSample::electronEfficiency(TLorentzVector *l, TLorentzVector *l2){
  if(!electronEfficiencies || !l) return 1.;
  double eff = electronEfficiencies->GetBinContent(electronEfficiencies->FindBin(fabs(l->Eta()), l->Pt()));
  if(eff == 0) eff = 1.;
  return eff*electronEfficiency(l2);
}

#endif
