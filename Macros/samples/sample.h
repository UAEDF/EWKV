#ifndef SAMPLE_H
#define SAMPLE_H

#include <fstream>
#include "TFile.h"
#include "TTree.h"
#include "TChain.h"
#include "TH1.h"


/****************
 * Class sample *
 ****************/
class sample{
  protected:
    TString location, name;
    double lumi, lumiWeight;

  public:
    virtual TTree* getTree();
    virtual bool isData() = 0;
    virtual double getPileUpWeight(int nPileUp) = 0;
    virtual double getWeight(int nPileUp) = 0;
 
    TString getName(){		return name;};
    TString getLocation(){	return location;};
    double getLumi(){		return lumi;};
    void setLumiWeight(double weight){ lumiWeight = weight;};
};

TTree* sample::getTree(){
  TFile* file = new TFile(location + "ewkv_" + name + ".root");
  if(file->IsZombie()){
    std::cout << "sample:\t\t\tERROR\tCould not find "<< location << "ewkv_" << name << ".root" << std::endl;
    exit(1);
  }
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
    dataRun(TString name_, TString location_, double lumi_, TString JSON_);
    bool isData(){ return true;};
    double getPileUpWeight(int nPileUp){ return 1.;};
    double getWeight(int nPileUp){ return 1.;};
    TString getJSON(){ return JSON + "/lumiSummary_" + name + ".json";};
};

dataRun::dataRun(TString name_, TString location_, double lumi_, TString JSON_){
  name 		= name_; 
  location 	= location_;
  lumi 		= lumi_;
  JSON		= JSON_;
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

dataSample::dataSample(TString name_, std::vector<dataRun*> runs_) : dataRun(name_, "", 0, ""){
  runs = runs_;
  mergeString = "";
  for(auto run = runs.begin(); run != runs.end(); ++run){
    lumi += (*run)->getLumi();
    mergeString += "_" + (*run)->getName();
  }
  std::cout << "dataSample:\t\t\t" << name << " with total lumi = " << lumi << "/pb" << std::endl;
}

TTree* dataSample::getTree(){
  TChain *chain = new TChain("EWKV");
  for(auto run = runs.begin(); run != runs.end(); ++run){
    if(!chain->Add((*run)->getLocation() + "ewkv_" + (*run)->getName() + ".root", -1)){
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
    std::vector<double> weights;
    double crossSection;
    int nEvents;

  public:
    mcSample(TString name_, TString location_, double crossSection_, int nEvents_);
    bool setPileUpWeights(TString pileUpWeightsFile);
    TH1I* getPileUpHisto();
 
    bool isData(){ return false;};
    double getPileUpWeight(int nPileUp){ return (nPileUp < 51 && nPileUp >= 0) ? weights.at(nPileUp) : 0;};
    double getWeight(int nPileUp){ return getPileUpWeight(nPileUp)*lumiWeight;}; //Add other weights here (lepton efficiencies)
};

mcSample::mcSample(TString name_, TString location_, double crossSection_, int nEvents_){
  name 		= name_; 
  location 	= location_;
  crossSection 	= crossSection_;
  nEvents 	= nEvents_;
  lumi 		= nEvents/crossSection;

  std::cout << "mcSample:\t\t\t" << name << " added (lumi = " << lumi << "/pb)" << std::endl;
}

TH1I* mcSample::getPileUpHisto(){
  TFile* file = new TFile(location + "pileUp_" + name + ".root");
  if(file->IsZombie()){
    std::cout << "mcSample:\t\tERROR\tCould not find "<< location << "pileUp_" << name << ".root" << std::endl;
    exit(1);
  }
  TH1I* hist = (TH1I*) file->Get("pileUp");
  return hist;
}

bool mcSample::setPileUpWeights(TString pileUpWeightsFile){
  weights = std::vector<double>(51, 1); 
  std::ifstream readFile;
  readFile.open(pileUpWeightsFile.Data());
  if(!readFile.is_open()) return false;
  while(!readFile.eof()){
    TString name_;
    readFile >> name_;
    if(name_ == name){
      weights.clear();
      for(int j=0; j < 51; ++j){
        double weight = 1.;
        readFile >> weight;
        weights.push_back(weight);
      }
      return true;
    }
  }
  return false;
}

#endif
