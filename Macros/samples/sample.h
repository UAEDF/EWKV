#ifndef SAMPLE_H
#define SAMPLE_H

#include <fstream>
#include "TFile.h"
#include "TTree.h"
#include "TChain.h"
#include "TH1.h"

#include "../environment.h"

/****************
 * Class sample *
 ****************/
class sample{
  protected:
    TString name;
    double lumi, lumiWeight;
    bool skimmed;
    TString type, tag;

  public:
    virtual TTree* getTree();
    virtual bool isData() = 0;
    virtual double getPileUpWeight(int nPileUp) = 0;
    virtual double getWeight(int nPileUp) = 0;
    virtual double muonEfficiency(TLorentzVector *l) = 0;

    TString getName(){		return name;};
    double getLumi(){		return lumi;};
    void setLumiWeight(double weight){ lumiWeight = weight;};

    void useSkim(TString type_, TString tag_){ skimmed = true; type = type_; tag = tag_;};
};

TTree* sample::getTree(){
  TString fileName = getTreeLocation() + "ewkv_" + name + ".root";
  if(skimmed){
    TString skimmedFileName = getTreeLocation() + "skimmed/" + type + "/" + tag + "/" + name + ".root";
    if(exists(skimmedFileName)) fileName = skimmedFileName;
    else std::cout << "sample:\t\t\t!!!\tNo skimmed file for " << name << ", full root tree is taken" << std::endl;
  }
  TFile* file = new TFile(fileName);
  if(file->IsZombie()){
    std::cout << "sample:\t\t\tERROR\tCould not find "<< fileName << std::endl;
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
    dataRun(TString name_, double lumi_);
    bool isData(){ 				return true;};
    double getPileUpWeight(int nPileUp){ 	return 1.;};
    double getWeight(int nPileUp){ 		return 1.;};
    double muonEfficiency(TLorentzVector *l){	return 1.;};
    TString getJSON(){ 				return JSON;};
};

dataRun::dataRun(TString name_, double lumi_){
  name 		= name_; 
  lumi 		= lumi_;
  JSON		= getCMSSWBASE() + "/src/EWKV/Crab/JSON/lumiSummary_" + name + ".json";
  skimmed	= false;
  type		= "";
  tag		= "";
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
    TString fileName = getTreeLocation() + "skimmed/" + type + "/" + tag + "/data.root";
    TFile* file = new TFile(fileName);
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
    std::vector<double> weights;
    std::map<TString, TGraphAsymmErrors*> efficiencies;
    double crossSection;
    int nEvents;

    double muonEfficiency(TString type, double pt, double eta);

  public:
    mcSample(TString name_, double crossSection_, int nEvents_, std::map<TString, TGraphAsymmErrors*> efficiencies_);
    bool setPileUpWeights(TString pileUpWeightsFile);
    TH1I* getPileUpHisto();
 
    bool isData(){ return false;};
    double getPileUpWeight(int nPileUp){ return (nPileUp < 51 && nPileUp >= 0) ? weights.at(nPileUp) : 0;};
    double muonEfficiency(TLorentzVector *l);
    double getWeight(int nPileUp){ return getPileUpWeight(nPileUp)*lumiWeight;};
};

mcSample::mcSample(TString name_, double crossSection_, int nEvents_, std::map<TString, TGraphAsymmErrors*> efficiencies_){
  name 		= name_; 
  crossSection 	= crossSection_;
  nEvents 	= nEvents_;
  efficiencies  = efficiencies_;
  lumi 		= (double)nEvents/(double)crossSection;
  skimmed	= false;
  type		= "";
  tag		= "";

  std::cout << "mcSample:\t\t\t" << name << " added (lumi = " << lumi << "/pb)" << std::endl;
}

TH1I* mcSample::getPileUpHisto(){
  TFile* file = new TFile(getTreeLocation() + "pileUp_" + name + ".root");
  if(file->IsZombie()){
    std::cout << "mcSample:\t\tERROR\tCould not find "<< getTreeLocation() << "pileUp_" << name << ".root" << std::endl;
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
      readFile.close();
      return true;
    }
  }
  readFile.close();
  return false;
}


double mcSample::muonEfficiency(TLorentzVector *l){
  double pt = l->Pt();
  double eta = l->Eta();
  return muonEfficiency("iso", pt, eta)*muonEfficiency("id", pt, eta);
}
 
double mcSample::muonEfficiency(TString type, double pt, double eta){ 
  TString etaBin;
  if(fabs(eta) < .9) etaBin = "<0.9";
  else if(fabs(eta) < 1.2) etaBin = "0.9-1.2";
  else if(fabs(eta) < 2.1) etaBin = "1.2-2.1";
  else if(fabs(eta) < 2.4) etaBin = "2.1-2.4";
  else return 0.;
  if(efficiencies.find(type + etaBin) == efficiencies.end()){
    std::cout << "mcSample\t\t!!! No " << type << " efficiency found for pt=" << pt << " and eta=" << eta << std::endl;
    return 1.;
  }
  double efficiency, ptlow, pthigh, ptbin;
  int i = 0;
  do{
    efficiencies[type + etaBin]->GetPoint(i, ptbin, efficiency);
    ptlow = ptbin - efficiencies[type + etaBin]->GetErrorXlow(i);
    pthigh = ptbin + efficiencies[type + etaBin]->GetErrorXhigh(i++);
  } while((pthigh < pt) && (i < efficiencies[type + etaBin]->GetN()));
  return efficiency;
}

#endif
