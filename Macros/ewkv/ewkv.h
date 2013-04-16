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
    void initTMVAreader(TString type);

    enum VType { WMUNU, WENU, ZMUMU, ZEE, UNDEFINED};

    VType vType;
    int nRun, nEvent, nLumi;
    int nPileUp, nPriVtxs;
    float rho;
    int nGenPart, nJets, nLeptons, nSoftTrackJets;
    TClonesArray *vGenPart, *vLeptons, *vMET, *vJets, *vSoftTrackJets;
    float met, metPhi, metSig;
    int idGenPart[10], ncJets[15], leptonCharge[2];
    double jetUncertainty[15];
    float jetSmearedPt[15], genJetPt[15];
    float jetQGMLP[15], jetQGLikelihood[15];
    bool jetID[15];
    bool Mu17_Mu8, Mu17_TkMu8, Ele17_Ele8, Ele17T_Ele8T;

    TTree* tree;
    sample* mySample;
    histoCollection* histos;
    cutFlow* cutflow;
    bool makeTMVAtree, firstTMVAevent;
    std::map<TString, float> tmvaVariables;
    TFile *tmvaFile;
    TTree *tmvaTree;
    TString tmvaLocation;
    TMVA::Reader *tmvaReader;

  public:
    ewkvAnalyzer(sample* mySample_, TFile *outfile);
    ~ewkvAnalyzer();
    void loop(TString type, double testFraction = 1.);
    histoCollection* getHistoCollection(){ return histos;}; 
    cutFlow* getCutFlow(){ return cutflow;};
    void setMakeTMVAtree(TString location){ makeTMVAtree = true; tmvaLocation = location;}
};


ewkvAnalyzer::ewkvAnalyzer(sample* mySample_, TFile *outfile){
  makeTMVAtree = false; tmvaLocation = ".";
  mySample = mySample_;
  tree = mySample->getTree();
  histos = new histoCollection(mySample, outfile);
  cutflow = new cutFlow(mySample->getName());

  vGenPart = 		new TClonesArray("TLorentzVector", 10);
  vLeptons = 		new TClonesArray("TLorentzVector", 2);
  vMET = 		new TClonesArray("TLorentzVector", 1);
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
  tree->SetBranchAddress("met",			&met);
  tree->SetBranchAddress("metPhi",		&metPhi);
  tree->SetBranchAddress("metSig",		&metSig);

  tree->SetBranchAddress("nJets",		&nJets);
  tree->SetBranchAddress("vJets",		&vJets);
  tree->SetBranchAddress("jetUncertainty", 	jetUncertainty);
  tree->SetBranchAddress("jetQGMLP", 		jetQGMLP);
  tree->SetBranchAddress("jetQGLikelihood", 	jetQGLikelihood);
  tree->SetBranchAddress("genJetPt", 		genJetPt);
  tree->SetBranchAddress("jetSmearedPt", 	jetSmearedPt);

  tree->SetBranchAddress("nSoftTrackJets",	&nSoftTrackJets);
  tree->SetBranchAddress("vSoftTrackJets",	&vSoftTrackJets);

  tree->SetBranchAddress("HLT_Mu17_TkMu8", 	&Mu17_Mu8);
  tree->SetBranchAddress("HLT_Mu17_TkMu8", 	&Mu17_TkMu8);
  tree->SetBranchAddress("HLT_Ele17_CaloIdL_CaloIsoVL_Ele8_CaloIdL_CaloIsoVL", &Ele17_Ele8);
  tree->SetBranchAddress("HLT_Ele17_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL_Ele8_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL", &Ele17T_Ele8T);

  std::cout << "ewkvAnalyzer:\t\t\tTree initialized for " << mySample->getName() << std::endl;
}


ewkvAnalyzer::~ewkvAnalyzer(){
  delete tree, tmvaTree, tmvaFile;
  delete vGenPart, vLeptons, vMET, vJets, vSoftTrackJets;
  delete histos;
}

void ewkvAnalyzer::loop(TString type, double testFraction){
  firstTMVAevent = true;
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
  double percentPoint = nEntries/100.;
  double nextPercentPoint = percentPoint;
  for(int i = 0; i < nEntries; ++i){
    if(i > nextPercentPoint){ std::cout << "="<< std::flush; nextPercentPoint += percentPoint;}
    tree->GetEntry(i);
    if(vType != theType) continue;
    histos->setBranch();
    cutflow->setBranch();
    if(theType == ZMUMU || theType == ZEE) analyze_Zjets();
    else analyze_Wjets();
  }
  if(makeTMVAtree && !firstTMVAevent){
    tmvaFile->cd();
    tmvaFile->WriteTObject(tmvaTree);
    tmvaFile->Close();
  }
  std::cout << std::endl;
  return;
}

void ewkvAnalyzer::fillTMVAtree(){
  if(!makeTMVAtree) return;
  if(firstTMVAevent){
    tmvaFile = new TFile(tmvaLocation + mySample->getName() + ".root","recreate");
    tmvaFile->cd();
    tmvaTree = new TTree("ewkv-TMVA-input","tree used for TMVA input");
    for(auto tmvaVariable = tmvaVariables.begin(); tmvaVariable != tmvaVariables.end(); ++tmvaVariable){
      tmvaTree->Branch(tmvaVariable->first, &tmvaVariables[tmvaVariable->first], tmvaVariable->first + "/D");
    }
    firstTMVAevent = false;
  }
  tmvaTree->Fill();
}

#endif
