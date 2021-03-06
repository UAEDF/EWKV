#include <iostream>
#include <fstream>
#include <sstream>
#include <TROOT.h>
#include <TString.h>
#include <TFile.h>
#include <TH1.h>

#include "../samples/sampleList.h" 
#include "../samples/sample.h" 
#include "../environment.h"

void pileUpWeights(TH1 *pileUpData, TH1 *pileUpMC, TString sampleName);
std::ofstream writeFile;

int main(int argc, char *argv[]){
  TString minBiasXsec = "70300";
  TString puType = "observed";
  TString option = "";
  bool force = false, pixel = false;
  if(argc > 2) minBiasXsec = (TString) argv[2];
  if(argc > 3){
    if(((TString) argv[3]).Contains("f")) force = true;
    if(((TString) argv[3]).Contains("p")){ pixel = true; option += "_pixel";}
    if(((TString) argv[3]).Contains("t")){ puType = "true"; option += "_true";}
  }
  TString puJSON = "pileup_JSON_DCSONLY_190389-208686_corr.txt";
  if(pixel) puJSON = "pileup_JSON_DCSONLY_190389-208686_All_2012_pixelcorr.txt";

  for(TString type : typeSelector(argc, argv)){
    //Check data configuration
    sampleList *samples = new sampleList();
    TString samplesDir = getCMSSWBASE() + "src/EWKV/Macros/samples/";
    if(!samples->init(samplesDir + "data_" + type + "_pixel.config", samplesDir + "mc.config", "pileUp")) return 1;
    TString mergeString = "";
    TString listJSON = "";
    for(sampleList::runIterator run = samples->first(); run != samples->last(); ++run){
      std::cout << "pileUp.C:\t\t\tAdd " << (*run)->getName() << " with lumi = " << (*run)->getLumi() << "/pb (" << (*run)->getJSON() << ")" << std::endl;
      mergeString += "_" + (*run)->getName();
      listJSON += " " + (*run)->getJSON();
    }

    //Get pile-up distribution for data
    TString mergedJSON = getCMSSWBASE() + "src/EWKV/Macros/pileUp/lumiSummary" + mergeString + ".json";
    TString mergedROOT = getCMSSWBASE() + "src/EWKV/Macros/pileUp/pileUp" + mergeString + "_" + minBiasXsec + option + ".root";
    if(!exists(mergedROOT) || force){
      std::cout << "pileUp.C:\t\t\tPile-up calculation of the data (minBiasXsec = " << minBiasXsec << "): this will take some time..." << std::endl;
      system(("mergeJSON.py" + listJSON + " --output="+mergedJSON).Data());
      TString command = "pileupCalc.py -i "+mergedJSON+" --inputLumiJSON " + puJSON + " --calcMode " + puType + " --minBiasXsec " + minBiasXsec + " --maxPileupBin 100 --numPileupBins 100 " + mergedROOT;
      system(command); 
    } else { std::cout << "pileUp.C:\t\t!!!\tWill use existing pileUp" << mergeString << "_" << minBiasXsec << option << ".root file, use -f to recreate this file" << std::endl;}
    TFile *file_data = new TFile(mergedROOT);
    TH1D *pileUpData = (TH1D*) file_data->Get("pileup");

    //Calculate the weights
    writeFile.open((getCMSSWBASE() + "src/EWKV/Macros/pileUp/weights" + mergeString + "_" + minBiasXsec + option + ".txt").Data());
    for(sampleList::iterator it = samples->begin(); it != samples->end(); ++it){
      if((*it)->isData()) continue;
      mcSample *mc = (mcSample*) (*it);
      pileUpWeights(pileUpData, mc->getPileUpHisto(), mc->getName());
    }
    writeFile.close();
    file_data->Close();
  }
  return 0;
}


void pileUpWeights(TH1 *pileUpData, TH1 *pileUpMC, TString sampleName){
  double sumData = pileUpData->Integral();
  double sumMC = pileUpMC->Integral();

  writeFile << sampleName << std::endl;
  for(int i = 1; i <= pileUpData->GetNbinsX(); ++i){
    double weight = (double) pileUpData->GetBinContent(i)/ (double)pileUpMC->GetBinContent(i);
    weight *= sumMC/sumData;
    if(pileUpMC->GetBinContent(i) == 0) weight = 0;
    writeFile << weight << "\t";
  }
  writeFile << std::endl;
  return;
}
