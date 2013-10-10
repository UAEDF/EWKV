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

    typedef std::vector<sample*>::iterator iterator;
    iterator begin(){ return samples.begin();};
    iterator end(){ return samples.end();};
    int size(){ return samples.size();};

    typedef std::vector<dataRun*>::iterator runIterator;
    runIterator first(){ return dataRuns.begin();};
    runIterator last(){ return dataRuns.end();};
   

  private:
    bool readInitFile(TString file, TString type, bool useAll = false);
    bool readLeptonEfficiencies();

    std::vector<sample*> samples;
    std::vector<dataRun*> dataRuns;
    std::map<TString, TGraphAsymmErrors*> efficiencies;
};

sampleList::~sampleList(){
  for(iterator it = samples.begin(); it != samples.end(); ++it) delete *it;
  for(std::vector<dataRun*>::iterator it = dataRuns.begin(); it != dataRuns.end(); ++it) delete *it;
}

bool sampleList::init(TString dataFile, TString mcFile, TString mode){
  if(!readLeptonEfficiencies()) std::cout << "sampleList:\t\t!!!\tFailed reading lepton efficiencies" << std::endl;
  if(!(readInitFile(dataFile, "data", mode == "pileUp") && readInitFile(mcFile, "mc", mode == "pileUp"))) return false;
  if(mode == "pileUp") return true;

  //pile-up weights
  dataSample *data = (dataSample*) get("data");
  TString puWeightsFile = getCMSSWBASE() + "/src/EWKV/Macros/pileUp/weights" + data->getMergeString() + "_70300_old.txt";
  if(!exists(puWeightsFile)) std::cout << "sampleList:\t\t!!!\t" + puWeightsFile + " not found, run pileUp.C first" << std::endl;
  else {
    for(iterator it = samples.begin(); it != samples.end(); ++it){
      if((*it)->isData()) continue;
      mcSample* mc = (mcSample*) (*it);
      if(!mc->setPileUpWeights(puWeightsFile)) std::cout << "sampleList:\t\t!!!\tNo pile-up reweighting for " << mc->getName() << std::endl;
    }
  }

  //lumi weights
  for(iterator it = samples.begin(); it != samples.end(); ++it){ (*it)->setLumiWeight(get("data")->getLumi()/(*it)->getLumi());}

  return true;
}

bool sampleList::readInitFile(TString file, TString type, bool useAll){
  std::ifstream readFile;
  readFile.open(file.Data());
  if(!readFile.is_open()){
    std::cout << "sampleList:\t\t!!!\t" << file << " not found!" << std::endl;
    return false;
  }
  while(!readFile.eof()){
    TString useLine;
    readFile >> useLine;
    if(useLine != "1"){
      if(!((useLine == "0") && useAll)){
        readFile.ignore(unsigned(-1), '\n');
        continue;
      }
    }
    if(type == "data"){
      TString name; double lumi;
      readFile >> name >> lumi;
      dataRuns.push_back(new dataRun(name, lumi));
    } else {
      TString name; double crossSection; int nEvents;
      readFile >> name >> crossSection >> nEvents;
      samples.push_back(new mcSample(name, crossSection, nEvents, efficiencies));
    }    
  }
  if(type == "data") samples.push_back(new dataSample("data", dataRuns));
  readFile.close();
  return true;
}


sample* sampleList::get(TString name){
  for(iterator it = samples.begin(); it != samples.end(); ++it){
    if((*it)->getName() == name) return *it;
  }
  std::cout << "sampleList\t\t!!!\t" << name << " not found!" << std::endl;
  return NULL;
}


bool sampleList::readLeptonEfficiencies(){
  TFile *iso_file = new TFile(getCMSSWBASE() + "/src/EWKV/Macros/samples/efficiencies/MuonEfficiencies_ISO_Run_2012ReReco_53X.root");
  TFile *id_file = new TFile(getCMSSWBASE() + "/src/EWKV/Macros/samples/efficiencies/MuonEfficiencies_ID_Run_2012ReReco_53X.root");
  if(iso_file->IsZombie() || id_file->IsZombie()) return false;
  TString isolationKey = "tkRelIso_Tight";
  TString idKey = "Tight";
  for(TString etaBin : {"<0.9","0.9-1.2","1.2-2.1","2.1-2.4"}){
    efficiencies["iso" + etaBin] = (TGraphAsymmErrors*) iso_file->Get("DATA_over_MC_" + isolationKey + "_pt_abseta" + etaBin);
    efficiencies["id" + etaBin] = (TGraphAsymmErrors*) id_file->Get("DATA_over_MC_" + idKey + "_pt_abseta" + etaBin);
  }
  delete iso_file, id_file;
  return true;
}


#endif
