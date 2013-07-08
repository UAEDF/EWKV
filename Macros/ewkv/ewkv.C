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
#include <TH1F.h>
#include <TH2F.h>
#include <THStack.h>
#include <TLegend.h>
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
#include "../samples/sampleList.h"
#include "../samples/sample.h"
#include "../histos/histos.h"
#include "../environment.h"
#include "../cutFlow/cutFlow.h"
#include "../cutFlow/cutFlowHandler.h"
#include "ewkv.h"

// Constants
#define ZMASS 91.1876

// Cuts
#define JETPT_RADPAT 40
#define JET1PT 50 
#define JET2PT 30 
#define JETETA 4.7

// Options
#define TMVATAG "20130618_BDT50k" 
#define TMVATYPE "BDT"
#define DYTYPE "composed"
#define OUTPUTTAG "Full20130708"


/*****************
 * Main function *
 *****************/
int main(int argc, char *argv[]){
  std::vector<TString> types {"ZEE","ZMUMU"};
  if(argc > 1 && ((TString) argv[1]) == "ZEE") types = {"ZEE"};
  if(argc > 1 && ((TString) argv[1]) == "ZMUMU") types = {"ZMUMU"};
  gROOT->SetBatch();

  for(TString type : types){
    TString samplesDir = getCMSSWBASE() + "/src/EWKV/Macros/samples/";					//Set up list of samples
    TString mcConfig = samplesDir + (DYTYPE == "inclusive"? "mcInclusive.config" : "mc.config");
    TString dataConfig = samplesDir + "data_" + type + ".config";
    sampleList* samples = new sampleList();
    if(!samples->init(dataConfig, mcConfig)) return 1;

    TString outputDir = getTreeLocation() + "/outputs/rootfiles/" + type + "/";				//Set up output rootfile
    if(!exists(outputDir)) system("mkdir -p " + outputDir);
    TFile *outFile = new TFile(outputDir + OUTPUTTAG + ".root", "RECREATE");

    cutFlowHandler* cutflows = new cutFlowHandler();
    for(sampleList::iterator it = samples->begin(); it != samples->end(); ++it){			//loop over samples
//      (*it)->useSkim(type, "20130617c");								//Use skimmed files to go faster
      ewkvAnalyzer *myAnalyzer = new ewkvAnalyzer(*it, outFile, OUTPUTTAG);				//set up analyzer
      myAnalyzer->makeTMVAtree();									//Use if TMVA input trees has to be remade
      myAnalyzer->makeSkimTree(); 									//Use if skimmed trees has to be remade
      myAnalyzer->loop(type);										//loop over events in tree
      cutflows->add(myAnalyzer->getCutFlow());								//get the cutflow
      delete myAnalyzer;
    }
    outFile->Close();

    TString cutFlowDir = getTreeLocation() + "cutflow/" + type + "/";					//Handle cutflows
    if(!exists(cutFlowDir)) system("mkdir -p " + cutFlowDir);
    cutflows->toLatex(cutFlowDir + OUTPUTTAG + "_notmerged.tex");
    cutflows->merge("DY-powheg",{"DYEE-powheg","DYMUMU-powheg","DYTAUTAU-powheg"});
    cutflows->merge("DY",{"DY0","DY1","DY2","DY3","DY4"});
    cutflows->merge("Diboson",{"WW","WZ","ZZ"});
    cutflows->merge("TTJets",{"TTJetsSemiLept","TTJetsFullLept","TTJetsHadronic"});
    cutflows->merge("Single top",{"T-s","T-t","T-W","Tbar-s","Tbar-t","Tbar-W"});
    cutflows->merge("QCD",{"QCD100","QCD250","QCD500","QCD1000"});
    cutflows->toLatex(cutFlowDir + OUTPUTTAG + ".tex");

    delete cutflows, samples, outFile;
  }
  return 0;
}



/*******************************************
 * Function called for every event in tree *
 *******************************************/
void ewkvAnalyzer::analyze_Zjets(){
  // Trigger 
  if((vType == ZMUMU) && !(Mu17_Mu8 || Mu17_TkMu8)) return;
  if((vType == ZEE) && !Ele17T_Ele8T) return;

  // Get lorentzvectors (l+ in l1 and l- in l2) + construct Z boson
  TLorentzVector l1 	= *((TLorentzVector*) vLeptons->At(leptonCharge[0] == 1? 0 : 1));
  TLorentzVector l2 	= *((TLorentzVector*) vLeptons->At(leptonCharge[0] == 1? 1 : 0));
  TLorentzVector Z 	= TLorentzVector(l1 + l2);
  TVector3 leptonPlane = l1.Vect().Cross(l2.Vect());

  histos->setFillWeight(mySample->getWeight(nPileUp));		//TO DO: update sample::getWeight function with lepton efficiencies
  cutflow->setFillWeight(mySample->getWeight(nPileUp));

  histos->fillHist1D("nPriVtxs", nPriVtxs);
  histos->fillHist1D("nPileUp", nPileUp);

  histos->fillHist1D("lepton_pt", l1.Pt());
  histos->fillHist1D("lepton_eta", l1.Eta());
  histos->fillHist1D("lepton_phi", l1.Phi());

  histos->fillHist1D("lepton_pt", l2.Pt());
  histos->fillHist1D("lepton_eta", l2.Eta());
  histos->fillHist1D("lepton_phi", l2.Phi());

  // Select Z window
  if(fabs(Z.M() - ZMASS) > 40) return;
  histos->fillHist1D("dilepton_mass", Z.M());
  if(fabs(Z.M() - ZMASS) > 20) return;
  cutflow->track("$\\mid m_Z-m_{ll} \\mid < 20$ GeV"); 

  histos->fillHist1D("dilepton_pt", Z.Pt());
  histos->fillHist1D("dilepton_eta", Z.Eta());
  histos->fillHist1D("dilepton_phi", Z.Phi());


  // Set up parallel branches for JES
  for(TString branch : {"JES-", "", "JES+"}){
    histos->setBranch(branch);
    cutflow->setBranch(branch);

    if(branch == "") checkRadiationPattern(Z.Rapidity());

    // Find leading jets (at least 3) and sort
    std::vector<int> jetOrder;
    std::vector<double> jetPt;
    for(int j=0; j < vJets->GetEntries(); ++j){
      TLorentzVector jet = *((TLorentzVector*) vJets->At(j));
      if(branch == "JES-") jet *= (1-jetUncertainty[j]);
      if(branch == "JES+") jet *= (1+jetUncertainty[j]);

      if(fabs(jet.Eta()) > JETETA) continue;

      int k = 0;
      while(k < jetOrder.size() && (jet.Pt() < jetPt.at(k))) ++k;
      jetOrder.insert(jetOrder.begin() + k, j);
      jetPt.insert(jetPt.begin() + k, jet.Pt());
    }
    if(jetOrder.size() < 2) continue;
    TLorentzVector j1 = *((TLorentzVector*) vJets->At(jetOrder.at(0)));
    TLorentzVector j2 = *((TLorentzVector*) vJets->At(jetOrder.at(1)));
    if(branch == "JES-") j1 *= (1-jetUncertainty[jetOrder.at(0)]);
    if(branch == "JES+") j1 *= (1+jetUncertainty[jetOrder.at(0)]);
    if(branch == "JES-") j2 *= (1-jetUncertainty[jetOrder.at(1)]);
    if(branch == "JES+") j2 *= (1+jetUncertainty[jetOrder.at(1)]);

    histos->fillHist1D("jet1_pt", j1.Pt());
    histos->fillHist1D("jet1_pt_log", j1.Pt());
    histos->fillHist1D("jet2_pt", j2.Pt());
    histos->fillHist1D("jet2_pt_log", j2.Pt());

    cutflow->track("2 jets, no $p_T$ cut");
    if(j1.Pt() < JET1PT) continue;
    if(j2.Pt() < JET2PT) continue;
    cutflow->track("2 jets"); 
    fillSkimTree();

    TLorentzVector jj = TLorentzVector(j1 + j2);
    TLorentzVector all = TLorentzVector(l1 + l2 + j1 + j2);
    double mjj = jj.M();
    double ystar = Z.Rapidity() - (j1.Rapidity() + j2.Rapidity())/2;

    // TMVA tree variables
    tmvaVariables["pT_Z"] = Z.Pt();
    tmvaVariables["pT_j1"] = j1.Pt();
    tmvaVariables["pT_j2"] = j2.Pt();
    tmvaVariables["eta_Z"] = Z.Eta();
    tmvaVariables["dPhi_j1"] = fabs(Z.DeltaPhi(j1));
    tmvaVariables["dPhi_j2"] = fabs(Z.DeltaPhi(j2));
    tmvaVariables["dPhi_jj"] = fabs(j1.DeltaPhi(j2)); 
    tmvaVariables["dEta_jj"] = fabs(j1.Eta() - j2.Eta());
    tmvaVariables["avEta_jj"] = fabs((j1.Eta() + j2.Eta())/2); 
    tmvaVariables["qgMLP_j1"] = jetQGvariables["qgMLP"]->at(jetOrder.at(0));
    tmvaVariables["qgMLP_j2"] = jetQGvariables["qgMLP"]->at(jetOrder.at(1));
    tmvaVariables["qgLikelihood_j1"] = jetQGvariables["qgLikelihood"]->at(jetOrder.at(0));
    tmvaVariables["qgLikelihood_j2"] = jetQGvariables["qgLikelihood"]->at(jetOrder.at(0));
    tmvaVariables["qgHIG13011_j1"] = jetQGvariables["qgHIG13011"]->at(jetOrder.at(0));
    tmvaVariables["qgHIG13011_j2"] = jetQGvariables["qgHIG13011"]->at(jetOrder.at(0));
    tmvaVariables["M_jj"] = mjj;
    tmvaVariables["ystar"] = fabs(ystar);
    tmvaVariables["weight"] = mySample->getWeight(nPileUp);
    if(branch = "") fillTMVAtree();

    double mvaValue = tmvaReader->EvaluateMVA(TMVATYPE); 
    histos->fillHist1D(TMVATYPE, mvaValue);

    // From here on fill histograms with reweighted values
    mcfmReweighting(mjj, ystar);

    histos->fillHist1D("jet1_eta", j1.Eta());
    histos->fillHist1D("jet1_phi", j1.Phi());

    histos->fillHist1D("jet2_eta", j2.Eta());
    histos->fillHist1D("jet2_phi", j2.Phi());

    histos->fillHist1D("dijet_mass", mjj);
    histos->fillHist1D("dijet_pt", jj.Pt());
    histos->fillHist1D("dijet_dphi", fabs(j1.DeltaPhi(j2)));
    histos->fillHist1D("dijet_deta", fabs(j1.Eta() - j2.Eta()));

    histos->fillHist1D("qgMLP_j1", jetQGvariables["qgMLP"]->at(jetOrder.at(0)));
    histos->fillHist1D("qgMLP_j2", jetQGvariables["qgMLP"]->at(jetOrder.at(1)));
    histos->fillHist1D("qgLikelihood_j1", jetQGvariables["qgLikelihood"]->at(jetOrder.at(0)));
    histos->fillHist1D("qgLikelihood_j2", jetQGvariables["qgLikelihood"]->at(jetOrder.at(1)));
    histos->fillHist1D("qgHIG13011_j1", jetQGvariables["qgHIG13011"]->at(jetOrder.at(0)));
    histos->fillHist1D("qgHIG13011_j2", jetQGvariables["qgHIG13011"]->at(jetOrder.at(1)));

    histos->fillHist1D("ystar", fabs(ystar));


    // Soft track jets
    double etamin = (j1.Eta() < j2.Eta()? j1.Eta() : j2.Eta());
    double etamax = (j1.Eta() < j2.Eta()? j2.Eta() : j1.Eta());

    int nStj = 0; double stjHT = 0, stjHT3 = 0;
    for(int t=0; t < vSoftTrackJets->GetEntries(); ++t){
      TLorentzVector softTrackJet = *((TLorentzVector*) vSoftTrackJets->At(t));
      if((softTrackJet.Pt() > 500) || (softTrackJet.Eta() > (etamax - 0.5)) || (softTrackJet.Eta() < (etamin + 0.5))) continue;
      stjHT += softTrackJet.Pt();
      if(nStj < 2) stjHT3 += softTrackJet.Pt();
      if(nStj == 0) histos->fillHist1D("softTrackJet1_pt", softTrackJet.Pt());
      ++nStj;
    }
    if(nStj != 0){
      histos->fillHist1D("stjHT3", stjHT3);
      histos->fillProfileHist("softHT3_vs_PV", nPriVtxs, stjHT3);
      histos->fillProfileHist("softHT3_vs_mjj", jj.M(), stjHT3);
      histos->fillProfileHist("softHT3_vs_detajj", fabs(j1.Eta() - j2.Eta()) , stjHT3);
    }
    histos->fillHist1D("totalSoftHT", totalSoftHT);
    histos->fillProfileHist("softHT_vs_PV", nPriVtxs, totalSoftHT);
    histos->fillProfileHist("softHT_vs_mjj", jj.M(), totalSoftHT);
    histos->fillProfileHist("softHT_vs_detajj", fabs(j1.Eta() - j2.Eta()) , totalSoftHT);

    // Additional cutflow
    if(jj.M() < 200) continue;
    cutflow->track("$m_{jj} > 200$ GeV");

    if(fabs(ystar)> 1.2) continue;
    cutflow->track("$\\mid y^{*} \\mid < 1.2$");

    if(jj.M() < 600) continue;
    cutflow->track("$m_{jj} > 600$ GeV");

    if(fabs(j1.Eta() - j2.Eta()) < 3.5) continue;
    cutflow->track("$\\Delta\\eta_{jj} > 3.5$");
  }
  return;
}


/*************************************
 * Initialization of the TMVA reader *
 *************************************/
void ewkvAnalyzer::initTMVAreader(TString type){
  tmvaReader = new TMVA::Reader("Silent");

  std::vector<TString> variables = {"pT_Z", "pT_j1", "pT_j2", "eta_Z", "dPhi_j1", "dPhi_j2", "dPhi_jj", "dEta_jj", "avEta_jj", "qgHIG13011_j1", "qgHIG13011_j2", "M_jj"};
  for(TString variable : variables) tmvaReader->AddVariable( variable, &tmvaVariables[variable]);

  tmvaReader->BookMVA( TMVATYPE, getTreeLocation + "tmvaWeights/" + type + "/" + TMVATAG + "/weights/TMVAClassification_" + TMVATYPE + ".weights.xml" );
}


/******************************
 * Raddiation pattern section *
 ******************************/
void ewkvAnalyzer::checkRadiationPattern(double zRapidity){
  int nJets = 0; double HT = 0, dEta = 0, cosDPhi = 0, mjj = 0, ystar = 0;
  std::vector<int> passedJets;
  for(int j=0; j < vJets->GetEntries(); ++j){
    TLorentzVector jet = *((TLorentzVector*) vJets->At(j));
    if(jet.Pt() > JETPT_RADPAT && fabs(jet.Eta()) < JETETA){
      ++nJets;
      HT += jet.Pt();
      for(int k : passedJets){
        TLorentzVector otherJet = *((TLorentzVector*) vJets->At(k));
        double dEtaTest = fabs(otherJet.Eta() - jet.Eta());
        if(dEtaTest > dEta){
          dEta = dEtaTest;
          cosDPhi = TMath::Cos(fabs(jet.DeltaPhi(otherJet)));
          mjj = (TLorentzVector(jet + otherJet)).M();
          ystar = zRapidity - (jet.Rapidity() + otherJet.Rapidity())/2.;
        }
      }
      passedJets.push_back(j);
    }
  }
  if(nJets > 0) histos->fillProfileHist("nJets_vs_HT", HT , nJets);
  if(nJets > 1){
    mcfmReweighting(mjj, ystar);
    histos->fillProfileHist("nJets_vs_detajj", dEta , nJets);
    histos->fillProfileHist("cosdPhi_vs_HT", HT , cosDPhi);
    histos->fillProfileHist("cosdPhi_vs_detajj", dEta , cosDPhi);
    histos->setFillWeight(mySample->getWeight(nPileUp));
  }
}


/***************************
 * MCFM NLO/LO reweighting *
 ***************************/
void ewkvAnalyzer::mcfmReweighting(double mjj, double ystar){
  std::vector<TString> needReweighting = {"DY","DY2","DY3","DY4"};
  if(std::find(needReweighting.begin(), needReweighting.end(), mySample->getName()) == needReweighting.end()) return;
  double ystarWeight = (8.76856e-01) + (1.15122e-01)*ystar;
  double mjjWeight = (1.02289) + (-9.81406e-05)*mjj;
  histos->setFillWeight(mySample->getWeight(nPileUp)*ystarWeight*mjjWeight);
}
