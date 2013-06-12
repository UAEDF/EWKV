#ifndef EWKV_H
#define EWKV_H

#include <iostream>
#include <TROOT.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TCanvas.h>
#include <TMarker.h>
#include <TFile.h>
#include <TTree.h>
#include <TChain.h>
#include <TH1F.h>
#include <TLatex.h>
#include <TCut.h>
#include <TGraph.h>
#include <TLorentzVector.h>
#include <TClonesArray.h>
#include <TMath.h>
#include <Math/VectorUtil.h>
#include <TF1.h>
#include <TLegend.h>
#include <TLine.h>
#include <TColor.h>
#include <TProfile.h>
#include <TMVA/Reader.h>

// Our header-files
#include "../samples/sample.h"
#include "../histos/histos.h"
#include "../cutFlow/cutFlow.h"

class ewkvAnalyzer{
  private:
    void analyze_Zjets();
    void analyze_Wjets(){std::cout << "ewkvAnalyzer:\t\t\tanalyze_WType() is not implemented yet" << std::endl;}
    void fillTMVAtree();
    void fillSkimTree();
    void initTMVAreader(TString type);

    enum VType { WMUNU, WENU, ZMUMU, ZEE, UNDEFINED};

    VType vType;
    TString type;
    int nRun, nEvent, nLumi;
    int nPileUp, nPriVtxs;
    float rho;
    int nGenPart, nJets, nLeptons, nSoftTrackJets;
    TClonesArray *vGenPart, *vLeptons, *vMET, *vMETCorr, *vMETCorrNoV, *vJets, *vSoftTrackJets;
    float met, metPhi, metSig;
    float totalSoftHT;
    int idGenPart[10], ncJets[15], leptonCharge[2];
    double jetUncertainty[15];
    float jetSmearedPt[15], genJetPt[15];
    bool Mu17_Mu8, Mu17_TkMu8, Ele17T_Ele8T;

    std::map<TString, std::vector<float>*> jetQGvariables;

    TTree* tree;
    sample* mySample;
    histoCollection* histos;
    cutFlow* cutflow;
    bool makeTMVAtree, firstTMVAevent;
    bool makeSkimTree, firstSkimEvent, alreadyOnSkimmedTree;
    std::map<TString, float> tmvaVariables;
    TFile *tmvaFile, *skimFile;
    TTree *tmvaTree, *skimTree;
    TString tmvaOutputTag, skimOutputTag;
    TMVA::Reader *tmvaReader;


  public:
    ewkvAnalyzer(sample* mySample_, TFile *outfile);
    ~ewkvAnalyzer();
    void loop(TString type_, double testFraction = 1.);
    histoCollection* getHistoCollection(){ return histos;}; 
    cutFlow* getCutFlow(){ return cutflow;};
    void setMakeTMVAtree(TString outputTag = ""){ makeTMVAtree = true; tmvaOutputTag = outputTag;};
    void setMakeSkimTree(TString outputTag = ""){ makeSkimTree = true; skimOutputTag = outputTag;};
};


ewkvAnalyzer::ewkvAnalyzer(sample* mySample_, TFile *outfile){
  makeTMVAtree = false;
  makeSkimTree = false;
  firstSkimEvent = true;
  mySample = mySample_;
  tree = mySample->getTree();
  histos = new histoCollection(mySample, outfile);
  cutflow = new cutFlow(mySample->getName());

  vGenPart = 		new TClonesArray("TLorentzVector", 10);
  vLeptons = 		new TClonesArray("TLorentzVector", 2);
  vMET = 		new TClonesArray("TLorentzVector", 1);
  vMETCorr = 		new TClonesArray("TLorentzVector", 1);
  vMETCorrNoV = 	new TClonesArray("TLorentzVector", 1);
  vJets = 		new TClonesArray("TLorentzVector", 15);
  vSoftTrackJets = 	new TClonesArray("TLorentzVector", 25);

  // tree addresses
  tree->SetBranchAddress("run", 		&nRun);
  tree->SetBranchAddress("event", 		&nEvent);
  tree->SetBranchAddress("lumi", 		&nLumi);

  tree->SetBranchAddress("nPileUp", 		&nPileUp);
  tree->SetBranchAddress("rhokt6PFJets", 	&rho);
  tree->SetBranchAddress("nPriVtxs", 		&nPriVtxs);

  tree->SetBranchAddress("nGenPart", 		&nGenPart);
  tree->SetBranchAddress("vGenPart", 		&vGenPart);
  tree->SetBranchAddress("idGenPart",		idGenPart);

  tree->SetBranchAddress("VType",		&vType);
  tree->SetBranchAddress("nLeptons",		&nLeptons);
  tree->SetBranchAddress("vLeptons",		&vLeptons);
  tree->SetBranchAddress("leptonCharge",	leptonCharge);

  tree->SetBranchAddress("vMET",		&vMET);
  tree->SetBranchAddress("vMETCorr",		&vMETCorr);
  tree->SetBranchAddress("vMETCorrNoV",		&vMETCorrNoV);
  tree->SetBranchAddress("met",			&met);
  tree->SetBranchAddress("metPhi",		&metPhi);
  tree->SetBranchAddress("metSig",		&metSig);

  tree->SetBranchAddress("nJets",		&nJets);
  tree->SetBranchAddress("vJets",		&vJets);
  tree->SetBranchAddress("jetUncertainty", 	jetUncertainty);
  tree->SetBranchAddress("genJetPt", 		genJetPt);
  tree->SetBranchAddress("jetSmearedPt", 	jetSmearedPt);

  for(TString product : {"qg","axis1","axis2","mult","ptD"}) 		tree->SetBranchAddress(product + "MLP", &jetQGvariables[product + "MLP"]);
  for(TString product : {"qg","axis2","mult","ptD"}) 			tree->SetBranchAddress(product + "Likelihood", &jetQGvariables[product + "Likelihood"]);
  for(TString product : {"qg","axis1","axis2","mult","R","pull"}) 	tree->SetBranchAddress(product + "HIG13011", &jetQGvariables[product + "HIG13011"]);

  tree->SetBranchAddress("nSoftTrackJets",	&nSoftTrackJets);
  tree->SetBranchAddress("vSoftTrackJets",	&vSoftTrackJets);
  tree->SetBranchAddress("totalSoftHT",		&totalSoftHT);

  tree->SetBranchAddress("HLT_Mu17_Mu8", 	&Mu17_Mu8);
  tree->SetBranchAddress("HLT_Mu17_TkMu8", 	&Mu17_TkMu8);
  tree->SetBranchAddress("HLT_Ele17_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL_Ele8_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL", &Ele17T_Ele8T);

  std::cout << std::endl << "ewkvAnalyzer:\t\t\tTree initialized for " << mySample->getName() << std::endl;
}


ewkvAnalyzer::~ewkvAnalyzer(){
  delete tree, tmvaTree, tmvaFile;
  delete vGenPart, vLeptons, vMET, vJets, vSoftTrackJets;
  delete histos;
}


void ewkvAnalyzer::loop(TString type_, double testFraction){
  firstTMVAevent = true;
  type = type_;
  VType theType;
  if(type == "ZMUMU") theType = ZMUMU;
  else if(type == "ZEE") theType = ZEE;
  else if(type == "WMUNU") theType = WMUNU;
  else if(type == "WENU") theType = WENU;
  else {
    std::cout << "ewkvAnalyzer:\t\tERROR\tType unknown" << std::endl;
    return;
  }
  initTMVAreader(type);

  std::cout << "ewkvAnalyzer:\t\t\tLoop over " << mySample->getName() << " tree (" << type << " mode)" << std::endl;
  int nEntries = tree->GetEntries();
  if(testFraction != 1.) nEntries = (int) nEntries*testFraction;
  int bar = 0;
  for(int i = 0; i < nEntries; ++i){
    alreadyOnSkimmedTree = false;
    while(bar < ((double)i/(double)nEntries)*100){ std::cout << "="<< std::flush; ++bar;}
    tree->GetEntry(i);
    if(vType != theType) continue;
    histos->setBranch();
    cutflow->setBranch();
    if(theType == ZMUMU || theType == ZEE) analyze_Zjets();
    else analyze_Wjets();
  }
  while(bar < 100){ std::cout << "="<< std::flush; ++bar;}
  std::cout << std::endl;

  if(makeTMVAtree && !firstTMVAevent){
    tmvaFile->cd();
    tmvaFile->WriteTObject(tmvaTree);
    tmvaFile->Close();
    std::cout << "ewkvAnalyzer:\t\t\tTMVA tree prepared" << std::endl;
  }

  if(makeSkimTree && !firstSkimEvent){
    skimFile->cd();
    skimFile->WriteTObject(skimTree);
    skimFile->Close();
    std::cout << "ewkvAnalyzer:\t\t\tSkim of tree done" << std::endl;
  }

  return;
}

void ewkvAnalyzer::fillTMVAtree(){
  if(!makeTMVAtree) return;
  if(firstTMVAevent){
    TString fileName = mySample->getLocation() + "tmva-input/";
    if(!exists(fileName)) system("mkdir " + fileName);
    fileName += type + "/";
    if(!exists(fileName)) system("mkdir " + fileName);
    fileName += tmvaOutputTag + "/";
    if(!exists(fileName)) system("mkdir " + fileName);
    fileName += mySample->getName() + ".root";

    tmvaFile = new TFile(fileName ,"new");
    if(!tmvaFile->IsOpen()){
      std::cout << "ewkvAnalyzer:\t\t!!!\tCould not create " << fileName << " (maybe already exists)" << std::endl;
      makeTMVAtree = false;
      return;
    }
    tmvaFile->cd();
    tmvaTree = new TTree("ewkv-TMVA-input","tree used for TMVA input");
    for(auto tmvaVariable = tmvaVariables.begin(); tmvaVariable != tmvaVariables.end(); ++tmvaVariable){
      tmvaTree->Branch(tmvaVariable->first, &tmvaVariables[tmvaVariable->first], tmvaVariable->first + "/F");
    }
    firstTMVAevent = false;
  }

  tmvaTree->Fill();
}


void ewkvAnalyzer::fillSkimTree(){
  if(!makeSkimTree) return;
  if(alreadyOnSkimmedTree) return;
  if(firstSkimEvent){
    TString fileName = mySample->getLocation() + "skimmed/";
    if(!exists(fileName)) system("mkdir " + fileName);
    fileName += type + "/";
    if(!exists(fileName)) system("mkdir " + fileName);
    fileName += skimOutputTag + "/";
    if(!exists(fileName)) system("mkdir " + fileName);
    fileName += mySample->getName() + ".root";

    skimFile = new TFile(fileName ,"new");
    if(!skimFile->IsOpen()){
      std::cout << "ewkvAnalyzer:\t\t!!!\tCould not create " << fileName << " (maybe already exists)" << std::endl;
      makeSkimTree = false;
      return;
    }
    skimFile->cd();
    skimTree = tree->CloneTree(0);
    firstSkimEvent = false;
  }
  skimTree->Fill();
  alreadyOnSkimmedTree = true;
}
#endif
