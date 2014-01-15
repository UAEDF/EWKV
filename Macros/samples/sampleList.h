#ifndef SAMPLELIST_H
#define SAMPLELIST_H

#include <TString.h>
#include <TGraphAsymmErrors.h>
#include <fstream>
#include <map>
#include <stdlib.h>

#include "sample.h"
#include "../environment.h"

class sampleList{
  public:
    sampleList(){};
    ~sampleList();
    bool init(TString dataFile, TString mcFile, TString mode = "");
    sample* get(TString name);

    std::vector<sample*>::iterator begin(){ return samples.begin();};
    std::vector<sample*>::iterator end(){ return samples.end();};
    int size(){ return samples.size();};

    std::vector<dataRun*>::iterator first(){ return dataRuns.begin();};
    std::vector<dataRun*>::iterator last(){ return dataRuns.end();};
   

  private:
    void readInitFile(TString file, TString type, bool useAll = false);
    bool readLeptonEfficiencies();

    std::vector<sample*> samples;
    std::vector<dataRun*> dataRuns;
    std::map<TString, TGraphAsymmErrors*> muonEfficiencies;
    TH2D *electronEfficiencies;
};

sampleList::~sampleList(){
  for(auto it = samples.begin(); it != samples.end(); ++it) delete *it;
  for(auto it = dataRuns.begin(); it != dataRuns.end(); ++it) delete *it;
  for(auto it = muonEfficiencies.begin(); it != muonEfficiencies.end(); ++it) delete it->second;
}

bool sampleList::init(TString dataFile, TString mcFile, TString mode){
  if(!readLeptonEfficiencies()) std::cout << "sampleList:\t\t!!!\tFailed reading lepton efficiencies" << std::endl;
  readInitFile(dataFile, "data", mode == "pileUp");
  readInitFile(mcFile, "mc", mode == "pileUp");
  if(mode == "pileUp") return true;

  //pile-up weights
  dataSample *data = (dataSample*) get("data");
  for(TString puMode : {"*","PUUp","PUDown"}){
    TString minBiasXsec = "70300";
    if(puMode == "PUDown") minBiasXsec = "66785";
    if(puMode == "PUUp") minBiasXsec = "73815";
    TString puWeightsFile = getCMSSWBASE() + "src/EWKV/Macros/pileUp/weights" + data->getMergeString() + "_" + minBiasXsec + ".txt";
    if(!exists(puWeightsFile)) std::cout << "sampleList:\t\t!!!\t" + puWeightsFile + " not found, run pileUp.C first" << std::endl;
    else {
      for(auto it = samples.begin(); it != samples.end(); ++it){
        if((*it)->isData()) continue;
        mcSample* mc = (mcSample*) (*it);
        if(!mc->setPileUpWeights(puWeightsFile, puMode)) std::cout << "sampleList:\t\t!!!\tNo pile-up reweighting for " << mc->getName() << " " << puMode << std::endl;
      }
    }
  }

  //lumi weights
  for(auto it = samples.begin(); it != samples.end(); ++it){ (*it)->setLumiWeight(get("data")->getLumi()/(*it)->getLumi());}

  return true;
}

void sampleList::readInitFile(TString file, TString type, bool useAll){
  std::ifstream readFile;
  getStream(readFile, file.Data());
  std::stringstream line;
  while(getLine(readFile, line)){
    TString useLine;
    line >> useLine;
    if(!useLine.IsDigit()) continue;
    if(useLine == "1" || useAll){
      if(type == "data"){
        TString name; double lumi;
        line >> name >> lumi;
        dataRuns.push_back(new dataRun(name, lumi));
      } else {
        TString name; double crossSection; int nEvents;
        line >> name >> crossSection >> nEvents;
        samples.push_back(new mcSample(name, crossSection, nEvents, muonEfficiencies, electronEfficiencies));
      }
    }    
  }
  if(type == "data") samples.push_back(new dataSample("data", dataRuns));
  readFile.close();
}


sample* sampleList::get(TString name){
  for(auto it = samples.begin(); it != samples.end(); ++it){
    if((*it)->getName() == name) return *it;
  }
  std::cout << "sampleList\t\t!!!\t" << name << " not found!" << std::endl;
  return nullptr;
}


bool sampleList::readLeptonEfficiencies(){
  TFile *iso_file = new TFile(getCMSSWBASE() + "src/EWKV/Macros/samples/efficiencies/MuonEfficiencies_ISO_Run_2012ReReco_53X.root");
  TFile *id_file = new TFile(getCMSSWBASE() + "src/EWKV/Macros/samples/efficiencies/MuonEfficiencies_ID_Run_2012ReReco_53X.root");
  TFile *e_file = new TFile(getCMSSWBASE() + "src/EWKV/Macros/samples/efficiencies/CutBasedIdScaleFactors.root");
  if(iso_file->IsZombie() || id_file->IsZombie() || e_file->IsZombie()) return false;

  TString isolationKey = "tkRelIso_Tight";
  TString idKey = "Tight";
  for(TString etaBin : {"<0.9","0.9-1.2","1.2-2.1","2.1-2.4"}){
    iso_file->GetObject("DATA_over_MC_" + isolationKey + "_pt_abseta" + etaBin, muonEfficiencies["iso"+etaBin]);
    id_file->GetObject("DATA_over_MC_" + idKey + "_pt_abseta" + etaBin, muonEfficiencies["id"+etaBin]);
  }

  electronEfficiencies = nullptr;
  e_file->GetObject("sfLOOSE", electronEfficiencies);
  if(!electronEfficiencies) return false;

  delete iso_file, id_file, e_file;
  return true;
}


#endif
