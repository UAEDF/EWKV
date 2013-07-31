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
};

sampleList::~sampleList(){
  for(iterator it = samples.begin(); it != samples.end(); ++it) delete *it;
  for(std::vector<dataRun*>::iterator it = dataRuns.begin(); it != dataRuns.end(); ++it) delete *it;
}

bool sampleList::init(TString dataFile, TString mcFile, TString mode){
  if(!(readInitFile(dataFile, "data", mode == "pileUp") && readInitFile(mcFile, "mc", mode == "pileUp"))) return false;
  if(mode == "pileUp") return true;

  //pile-up weights
  dataSample *data = (dataSample*) get("data");
  TString puWeightsFile = getCMSSWBASE() + "/src/EWKV/Macros/pileUp/weights" + data->getMergeString() + ".txt";
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

  if(!readLeptonEfficiencies()) std::cout << "sampleList:\t\t!!!\tNo lepton efficiencies implemented" << std::endl;
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
      samples.push_back(new mcSample(name, crossSection, nEvents));
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

//Still to be implemented
bool sampleList::readLeptonEfficiencies(){
  return false;
  TFile *file = new TFile("STILL TO BE IMPLEMENTED");
  if(file->IsZombie()) return false;
  TString isolationKey = "relTKISO10";
//  leptonEfficiencies["2011Aeta_pt>20"] = (TGraphAsymmErrors*) efficiencyFile->Get(isolationKey + "_" + "2011Aeta_pt>20");
//  leptonEfficiencies["2011Beta_pt>20"] = (TGraphAsymmErrors*) efficiencyFile->Get(isolationKey + "_" + "2011Beta_pt>20");
  delete file;
  return true;
}
/*
double sampleTable::leptonEfficiency(TGraphAsymmErrors *efficiencies, double x){
  double efficiency, xlow, xhigh, xbin;
  int i = 0;
  do{
    efficiencies->GetPoint(i, xbin, efficiency);
    xlow = xbin - efficiencies->GetErrorXlow(i);
    xhigh = xbin + efficiencies->GetErrorXhigh(i++);
  } while((xhigh < x) && (i < efficiencies->GetN()));
  return efficiency;
}

double sampleTable::leptonEfficiency(double eta){
  return (lumiA*leptonEfficiency(leptonEfficiencies["2011Aeta_pt>20"], eta)+lumiB*leptonEfficiency(leptonEfficiencies["2011Beta_pt>20"],eta))/(lumiA+lumiB);
}
*/

#endif
