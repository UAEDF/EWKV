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
#include "../QGtagging/HIG13011.h"
#include "ewkv.h"
#include "meanAndRMS.h"

// Constants
#define ZMASS 91.1876

// Cuts
#define JETPT_RADPAT 40
#define JET1PT 50 
#define JET2PT 30 
#define JET3PT 30 
#define JETETA 4.7

// Options
#define TMVATAG "20131015_InclusiveDY_ptZrew_BDT_zstar_ptj2"
#define TMVATYPE "BDT"
#define DYTYPE "composed"
#define OUTPUTTAG "20131015_Fast3"

/*****************
 * Main function *
 *****************/
int main(int argc, char *argv[]){
  std::vector<TString> types {"ZEE","ZMUMU"};								//If no type given as option, run both
  if(argc > 1 && ((TString) argv[1]) == "ZEE") types = {"ZEE"};
  if(argc > 1 && ((TString) argv[1]) == "ZMUMU") types = {"ZMUMU"};
  gROOT->SetBatch();

  for(TString type : types){
    TString samplesDir = getCMSSWBASE() + "src/EWKV/Macros/samples/";					//Set up list of samples
    TString mcConfig = samplesDir + (DYTYPE == "inclusive"? "mcInclusive.config" : "mc.config");
    if(DYTYPE == "powheg") mcConfig = samplesDir + "mcPowheg.config";
    TString dataConfig = samplesDir + "data_" + type + "_pixel.config";
    sampleList* samples = new sampleList();
    if(!samples->init(dataConfig, mcConfig)) return 1;

    TString outputDir = getTreeLocation() + "/outputs/rootfiles/" + type + "/";				//Set up output rootfile
    makeDirectory(outputDir);
    TFile *outFile = new TFile(outputDir + OUTPUTTAG + ".root", "RECREATE");

    cutFlowHandler* cutflows = new cutFlowHandler();
    for(sampleList::iterator it = samples->begin(); it != samples->end(); ++it){			//Loop over samples
      (*it)->useSkim(type, "20131010_Full");								//Use skimmed files to go faster
      ewkvAnalyzer *myAnalyzer = new ewkvAnalyzer(*it, outFile, OUTPUTTAG);				//Set up analyzer class for this sample
//      myAnalyzer->makeTMVAtree();									//Use if TMVA input trees has to be remade
//      myAnalyzer->makeSkimTree(); 									//Use if skimmed trees has to be remade
      myAnalyzer->loop(type);										//Loop over events in tree
      cutflows->add(myAnalyzer->getCutFlow());								//Get the cutflow
      delete myAnalyzer;
    }
    outFile->Close();

    TString cutFlowDir = getTreeLocation() + "cutflow/" + type + "/";
    makeDirectory(cutFlowDir);
    cutflows->toLatex(cutFlowDir + OUTPUTTAG + "_notmerged.tex");					//Save cutflow table (individual samples)
    cutflows->merge("DY-powheg",{"DYEE-powheg","DYMUMU-powheg","DYTAUTAU-powheg"});
    cutflows->merge("DY",{"DY0","DY1","DY2","DY3","DY4"});
    cutflows->merge("Diboson",{"WW","WZ","ZZ"});
    cutflows->merge("TTJets",{"TTJetsSemiLept","TTJetsFullLept","TTJetsHadronic"});
    cutflows->merge("Single top",{"T-s","T-t","T-W","Tbar-s","Tbar-t","Tbar-W"});
    cutflows->merge("QCD",{"QCD100","QCD250","QCD500","QCD1000"});
    cutflows->toLatex(cutFlowDir + OUTPUTTAG + ".tex");							//Save cutflow table (merged samples)

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

  // Get weight (lumi + pileUp) and muon efficiencies (ISO+ID, trigger not yet available)
  double weight;
  if(vType == ZMUMU) weight = mySample->getWeight(nPileUp)*mySample->muonEfficiency(&l1)*mySample->muonEfficiency(&l2);
  if(vType == ZEE) weight = mySample->getWeight(nPileUp);
  histos->setEventWeight(weight); cutflow->setEventWeight(weight);

  // Apply muScleFit corrections
  if(vType == ZMUMU){
    if(mySample->isData() && nRun > 203776){
      muScleFitCorrectorD->applyPtCorrection(l1,1);
      muScleFitCorrectorD->applyPtCorrection(l2,-1);
    } else {
      muScleFitCorrector->applyPtCorrection(l1,1);
      muScleFitCorrector->applyPtCorrection(l2,-1);
      if(!mySample->isData()){
        muScleFitCorrector->applyPtSmearing(l1,1,false);
        muScleFitCorrector->applyPtSmearing(l2,-1,false);
      }
    }
  }

  // Construct Z
  TLorentzVector Z 	= TLorentzVector(l1 + l2);
  TVector3 leptonPlane 	= l1.Vect().Cross(l2.Vect());

  histos->fillHist1D("nPriVtxs", 	nPriVtxs);
  histos->fillHist1D("nPileUp", 	nPileUp);

  if(l1.Pt() < 20 || l2.Pt() < 20) return;

  ptReweighting(Z.Pt());
//  etaReweighting(Z.Eta());

  histos->fillHist1D("lepton_pt", 	l1.Pt());
  histos->fillHist1D("lepton_eta", 	l1.Eta());
  histos->fillHist1D("lepton_phi", 	l1.Phi());
  histos->fillHist1D("lepton+_pt", 	l1.Pt());
  histos->fillHist1D("lepton+_eta", 	l1.Eta());
  histos->fillHist1D("lepton+_phi", 	l1.Phi());

  histos->fillHist1D("lepton_pt", 	l2.Pt());
  histos->fillHist1D("lepton_eta",	l2.Eta());
  histos->fillHist1D("lepton_phi",	l2.Phi());
  histos->fillHist1D("lepton-_pt", 	l2.Pt());
  histos->fillHist1D("lepton-_eta",	l2.Eta());
  histos->fillHist1D("lepton-_phi",	l2.Phi());

  // Select Z bosons
  if(fabs(Z.M() - ZMASS) > 40) return;
  histos->fillHist1D("dilepton_mass", 	Z.M());
  if(fabs(Z.M() - ZMASS) > 20) return;
  cutflow->track("$\\mid m_Z-m_{ll} \\mid < 20$ GeV"); 

  histos->fillHist1D("dilepton_pt", 	Z.Pt());
  histos->fillHist1D("dilepton_eta", 	Z.Eta());
  histos->fillHist1D("dilepton_phi", 	Z.Phi());
  checkRadiationPattern(Z.Rapidity());


  // Set up parallel branches in the histograms/cutflow for JES
  for(TString branch : {"JES-", "", "JES+"}){
    if(mySample->isData() && branch == "JES-") continue;
    if(mySample->isData() && branch == "JES+") continue;
    histos->restoreEventWeight();											// Go back to normal event weight
    histos->setBranch(branch); cutflow->setBranch(branch);

    // Sort leading jets again after jet energy corrections
    std::vector<int> jetOrder;
    std::vector<double> jetPt;
    for(int j=0; j < vJets->GetEntries(); ++j){
      TLorentzVector jet = *((TLorentzVector*) vJets->At(j));
      if(fabs(jet.Eta()) > JETETA) continue;
      if(branch == "JES-") jet *= (1-jetUncertainty[j]);
      if(branch == "JES+") jet *= (1+jetUncertainty[j]);
      int k = 0;
      while(k < jetOrder.size() && (jet.Pt() < jetPt.at(k))) ++k;
      jetOrder.insert(jetOrder.begin() + k, j);
      jetPt.insert(jetPt.begin() + k, jet.Pt());
    }
    if(jetOrder.size() < 2) continue;
    cutflow->track("2 jets, no $p_T$ cut");

    TLorentzVector j1 = *((TLorentzVector*) vJets->At(jetOrder.at(0)));
    TLorentzVector j2 = *((TLorentzVector*) vJets->At(jetOrder.at(1)));
    if(branch == "JES-") j1 *= (1-jetUncertainty[jetOrder.at(0)]);
    if(branch == "JES+") j1 *= (1+jetUncertainty[jetOrder.at(0)]);
    if(branch == "JES-") j2 *= (1-jetUncertainty[jetOrder.at(1)]);
    if(branch == "JES+") j2 *= (1+jetUncertainty[jetOrder.at(1)]);
    double etamin = (j1.Eta() < j2.Eta()? j1.Eta() : j2.Eta()) + 0.5;
    double etamax = (j1.Eta() < j2.Eta()? j2.Eta() : j1.Eta()) - 0.5;

    histos->fillHist1D("jet1_pt", 		j1.Pt());
    histos->fillHist1D("jet1_pt_log", 		j1.Pt());

    histos->fillHist1D("jet2_pt", 		j2.Pt());
    histos->fillHist1D("jet2_pt_log", 		j2.Pt());

    if(j1.Pt() < JET1PT) continue;
    if(j2.Pt() < JET2PT) continue;
    cutflow->track("2 jets"); 
    fillSkimTree();

    histos->fillHist1D("dilepton_pt_JS", 	Z.Pt());
    histos->fillHist1D("dilepton_eta_JS", 	Z.Eta());
    histos->fillHist1D("dilepton_phi_JS", 	Z.Phi());

    histos->fillHist1D("jet1_phi", 		j1.Phi());
    histos->fillHist1D("jet1_eta", 		j1.Eta());

    histos->fillHist1D("jet2_phi", 		j2.Phi());
    histos->fillHist1D("jet2_eta", 		j2.Eta());


    //Redo QG tagging (only HIG13011 at the moment)
    if(mySample->isData()){
      QGCorrections("j1", &j1, jetOrder);
      QGCorrections("j2", &j2, jetOrder);
    }

    TString app;
    for(TString product : {"qg","axis1","axis2","mult","ptD"}){
      app = (fabs(j1.Eta()) < 2.5 ? "_c" : "_f");
      histos->fillHist1D(product + "MLP_j1", jetQGvariables[product + "MLP"]->at(jetOrder.at(0)));
      histos->fillHist1D(product + "MLP_j1" + app, jetQGvariables[product + "MLP"]->at(jetOrder.at(0)));
      app = (fabs(j2.Eta()) < 2.5 ? "_c" : "_f");
      histos->fillHist1D(product + "MLP_j2", jetQGvariables[product + "MLP"]->at(jetOrder.at(1)));
      histos->fillHist1D(product + "MLP_j2" + app, jetQGvariables[product + "MLP"]->at(jetOrder.at(1)));
    }
    for(TString product : {"qg","axis2","mult","ptD"}){
      app = (fabs(j1.Eta()) < 2.5 ? "_c" : "_f");
      histos->fillHist1D(product + "Likelihood_j1", jetQGvariables[product + "Likelihood"]->at(jetOrder.at(0)));
      histos->fillHist1D(product + "Likelihood_j1" + app, jetQGvariables[product + "Likelihood"]->at(jetOrder.at(0)));
      app = (fabs(j2.Eta()) < 2.5 ? "_c" : "_f");
      histos->fillHist1D(product + "Likelihood_j2", jetQGvariables[product + "Likelihood"]->at(jetOrder.at(1)));
      histos->fillHist1D(product + "Likelihood_j2" + app, jetQGvariables[product + "Likelihood"]->at(jetOrder.at(1)));
    }
    for(TString product : {"qg","axis1","axis2","mult","R","pull"}){
      if(fabs(j1.Eta()) < 2) app = "_c";
      else if(fabs(j1.Eta()) < 3) app = "_t";
      else app = "_f";
      histos->fillHist1D(product + "HIG13011_j1", jetQGvariables[product + "HIG13011"]->at(jetOrder.at(0)));
      histos->fillHist1D(product + "HIG13011_j1" + app, jetQGvariables[product + "HIG13011"]->at(jetOrder.at(0)));
      if(fabs(j2.Eta() < 2)) app = "_c";
      else if(fabs(j2.Eta() < 3)) app = "_t";
      else app = "_f";
      histos->fillHist1D(product + "HIG13011_j2", jetQGvariables[product + "HIG13011"]->at(jetOrder.at(1)));
      histos->fillHist1D(product + "HIG13011_j2" + app, jetQGvariables[product + "HIG13011"]->at(jetOrder.at(1)));
    }


    TLorentzVector jj = TLorentzVector(j1 + j2);
    TLorentzVector all = TLorentzVector(l1 + l2 + j1 + j2);
    double dy = j1.Rapidity() - j2.Rapidity();
    double ystarZ = Z.Rapidity() - (j1.Rapidity() + j2.Rapidity())/2;
    double zstarZ = ystarZ/dy;

    // TMVA tree variables (variables for training tree, for the actually used variables, see the TMVA init function)
    tmvaVariables["pT_Z"] = 			Z.Pt();
    tmvaVariables["pT_j1"] = 			j1.Pt();
    tmvaVariables["pT_j2"] = 			j2.Pt();
    tmvaVariables["pT_jj"] = 			jj.Pt();
    tmvaVariables["eta_Z"] = 			Z.Eta();
    tmvaVariables["dPhi_j1"] = 			fabs(Z.DeltaPhi(j1));
    tmvaVariables["dPhi_j2"] = 			fabs(Z.DeltaPhi(j2));
    tmvaVariables["dPhi_jj"] = 			fabs(j1.DeltaPhi(j2)); 
    tmvaVariables["dEta_jj"] = 			fabs(j1.Eta() - j2.Eta());
    tmvaVariables["avEta_jj"] = 		fabs((j1.Eta() + j2.Eta())/2); 
    tmvaVariables["qgMLP_j1"] = 		jetQGvariables["qgMLP"]->at(jetOrder.at(0));
    tmvaVariables["qgMLP_j2"] = 		jetQGvariables["qgMLP"]->at(jetOrder.at(1));
    tmvaVariables["qgLikelihood_j1"] = 		jetQGvariables["qgLikelihood"]->at(jetOrder.at(0));
    tmvaVariables["qgLikelihood_j2"] = 		jetQGvariables["qgLikelihood"]->at(jetOrder.at(1));
    tmvaVariables["qgHIG13011_j1"] = 		jetQGvariables["qgHIG13011"]->at(jetOrder.at(0));
    tmvaVariables["qgHIG13011_j2"] = 		jetQGvariables["qgHIG13011"]->at(jetOrder.at(1));
    tmvaVariables["M_jj"] = 			jj.M();
    tmvaVariables["ystarZ"] = 			fabs(ystarZ);
    tmvaVariables["zstarZ"] = 			fabs(zstarZ);
    tmvaVariables["weight"] = 			weight;
    if(branch = "") fillTMVAtree();

    double mvaValue = tmvaReader->EvaluateMVA(TMVATYPE); 

//    histos->fillHist1D("dijet_mass_NR", 	jj.M());
//    histos->fillHist1D("dijet_dphi_NR", 	fabs(j1.DeltaPhi(j2)));
//    histos->fillHist1D("ystar_Z_NR", 		fabs(ystarZ));
//    histos->fillHist1D(TString(TMVATYPE)+"_NR", mvaValue);
    // From here on fill histograms with MCFM reweighted values
 //   mcfmReweighting(jj.M(), ystarZ);
    histos->fillHist1D(TMVATYPE, 		mvaValue);

    histos->fillHist1D("dijet_mass", 		jj.M());
    histos->fillHist1D("dijet_pt", 		jj.Pt());
    histos->fillHist1D("dijet_dphi", 		fabs(j1.DeltaPhi(j2)));
    histos->fillHist1D("dijet_deta", 		fabs(j1.Eta() - j2.Eta()));
    histos->fillHist1D("dijet_av_eta", 		(j1.Eta() + j2.Eta())/2);
    histos->fillHist1D("dijet_sum_pt", 		(j1.Pt() + j2.Pt()));

    histos->fillHist1D("jet1_Z_dphi", 		fabs(Z.DeltaPhi(j1)));
    histos->fillHist1D("jet2_Z_dphi", 		fabs(Z.DeltaPhi(j2)));


    if(jj.M() > 100){
      histos->fillHist1D("dijet_dphi_100",      fabs(j1.DeltaPhi(j2)));
      histos->fillHist1D("ystar_Z_100", 	fabs(ystarZ));
      histos->fillHist1D(TString(TMVATYPE)+"_100", 	mvaValue);
    }
    if(jj.M() > 200){
      histos->fillHist1D("dijet_dphi_200",      fabs(j1.DeltaPhi(j2)));
      histos->fillHist1D("ystar_Z_200", 	fabs(ystarZ));
      histos->fillHist1D(TString(TMVATYPE)+"_200", 	mvaValue);
    }

    histos->fillHist1D("ystar_Z", 		fabs(ystarZ));
    histos->fillHist1D("zstar_Z", 		fabs(zstarZ));

    // Zeppenfeld variables for the 3rd jet
    if(jetOrder.size() > 2){
      TLorentzVector j3 = *((TLorentzVector*) vJets->At(jetOrder.at(2)));
      if(branch == "JES-") j3 *= (1-jetUncertainty[jetOrder.at(2)]);
      if(branch == "JES+") j3 *= (1+jetUncertainty[jetOrder.at(2)]);
      double ystar3 = j3.Rapidity() - (j1.Rapidity() + j2.Rapidity())/2;
      histos->fillHist1D("ystar_3", 		fabs(ystar3));
      histos->fillHist1D("zstar_3", 		fabs(ystar3/fabs(dy)));
    }

    // Central activity with soft track jets
    int nStj = 0; double stjHT = 0, stjHT3 = 0;
    for(int t=0; t < vSoftTrackJets->GetEntries(); ++t){
      TLorentzVector softTrackJet = *((TLorentzVector*) vSoftTrackJets->At(t));
      if((softTrackJet.Pt() > 500) || (softTrackJet.Eta() > etamax) || (softTrackJet.Eta() < etamin)) continue;
      stjHT += softTrackJet.Pt();
      if(nStj < 2) stjHT3 += softTrackJet.Pt();
      if(nStj == 0) histos->fillHist1D("softTrackJet1_pt", softTrackJet.Pt());
      ++nStj;
    }
    histos->fillHist1D("nSoftTrackJets", 		nStj);
    histos->fillHist1D("stjHT3", 			stjHT3);
    histos->fillHist1D("totalSoftHT", 			totalSoftHT);
    histos->fillProfileHist("softHT3_vs_PV", 		nPriVtxs, 			stjHT3);
    histos->fillProfileHist("softHT3_vs_mjj", 		jj.M(), 			stjHT3);
    histos->fillProfileHist("softHT3_vs_detajj", 	fabs(j1.Eta() - j2.Eta()), 	stjHT3);
    histos->fillProfileHist("softHT_vs_PV", 		nPriVtxs, 			totalSoftHT);
    histos->fillProfileHist("softHT_vs_mjj", 		jj.M(), 			totalSoftHT);
    histos->fillProfileHist("softHT_vs_detajj", 	fabs(j1.Eta() - j2.Eta()), 	totalSoftHT);


    // Central activity with jets
    int nCentralJets = 0; double centralHT = 0;
    for(int j=0; j < vJets->GetEntries(); ++j){
      if(j == jetOrder.at(0) || j == jetOrder.at(1)) continue;
      TLorentzVector jet = *((TLorentzVector*) vJets->At(j));
      if((fabs(jet.Eta()) > etamax) || (fabs(jet.Eta()) < etamin)) continue;
      if(branch == "JES-") jet *= (1-jetUncertainty[j]);
      if(branch == "JES+") jet *= (1+jetUncertainty[j]);
      if(jet.Pt() < JET3PT) continue;
      if(nCentralJets == 0) histos->fillHist1D("centralJet1_pt", jet.Pt());
      centralHT += jet.Pt();
      ++nCentralJets;
    }
    histos->fillHist1D("nCentralJets", 			nCentralJets);
    if(nCentralJets > 0) histos->fillHist1D("centralHT",centralHT);


    // Additional cutflow
    if(jj.M() < 200) continue;				cutflow->track("$m_{jj} > 200$ GeV");
    if(fabs(ystarZ)> 1.2) continue;    			cutflow->track("$\\mid y^{*} \\mid < 1.2$");
    if(jj.M() < 600) continue;    			cutflow->track("$m_{jj} > 600$ GeV");
    if(fabs(j1.Eta() - j2.Eta()) < 3.5) continue;	cutflow->track("$\\Delta\\eta_{jj} > 3.5$");
  }
  return;
}


/*************************************
 * Initialization of the TMVA reader *
 *************************************/
void ewkvAnalyzer::initTMVAreader(){
  tmvaReader = new TMVA::Reader("Silent");

//  std::vector<TString> variables = {"pT_Z", "pT_j1", "pT_j2", "eta_Z", "dPhi_j1", "dPhi_j2", "dPhi_jj", "dEta_jj", "avEta_jj", "qgHIG13011_j1", "qgHIG13011_j2", "M_jj"};
  std::vector<TString> variables = {"pT_Z", "pT_j2", "eta_Z", "zstarZ", "avEta_jj", "qgHIG13011_j1", "qgHIG13011_j2", "M_jj"};
  for(TString variable : variables) tmvaReader->AddVariable( variable, &tmvaVariables[variable]);

  tmvaReader->BookMVA( TMVATYPE, getTreeLocation() + "tmvaWeights/" + type + "/" + TMVATAG + "/weights/TMVAClassification_" + TMVATYPE + ".weights.xml" );
}


/*****************************
 * Radiation pattern section *
 *****************************/
void ewkvAnalyzer::checkRadiationPattern(double zRapidity){
  int nJets = 0; double HT = 0, dEta = 0, cosDPhi = 0, mjj = 0, ystarZ = 0;
  std::vector<int> passedJets;
  for(int j=0; j < vJets->GetEntries(); ++j){										// Loop over all jets
    TLorentzVector jet = *((TLorentzVector*) vJets->At(j));
    if(jet.Pt() < JETPT_RADPAT || fabs(jet.Eta()) > JETETA) continue;							// Use radiation pattern selections
    ++nJets;
    HT += jet.Pt();
    for(int k : passedJets){												// Loop over already selected jets
      TLorentzVector otherJet = *((TLorentzVector*) vJets->At(k));
      double dEtaTest = fabs(otherJet.Eta() - jet.Eta());
      if(dEtaTest > dEta){
        dEta = dEtaTest;
        cosDPhi = TMath::Cos(fabs(jet.DeltaPhi(otherJet)));
        mjj = (TLorentzVector(jet + otherJet)).M();
        ystarZ = zRapidity - (jet.Rapidity() + otherJet.Rapidity())/2.;
      }
    }
    passedJets.push_back(j);
  }
  if(nJets > 0) histos->fillProfileHist("nJets_vs_HT", 	HT, 	nJets);
  if(nJets > 1){
    mcfmReweighting(mjj, ystarZ);											// Use MCFM reweighting
    histos->fillProfileHist("nJets_vs_detajj", 		dEta, 	nJets);
    histos->fillProfileHist("cosdPhi_vs_HT", 		HT, 	cosDPhi);
    histos->fillProfileHist("cosdPhi_vs_detajj", 	dEta, 	cosDPhi);
    histos->restoreEventWeight();											// Go back to normal event weight
  }
}


/***************************
 * MCFM NLO/LO reweighting *
 ***************************/
void ewkvAnalyzer::mcfmReweighting(double mjj, double ystarZ){
  return; //OFF
  std::vector<TString> needReweighting = {"DY","DY2","DY3","DY4"};							// Only selected samples need reweighting
  if(std::find(needReweighting.begin(), needReweighting.end(), mySample->getName()) == needReweighting.end()) return;
//double ystarZWeight = (8.76856e-01) + (1.15122e-01)*ystarZ; 	 							// MCFM NLO/madGraph gen
//  double ystarZWeight = (9.50782e-01) + (-5.23409e-03)*ystarZ + (3.01934e-02)*ystarZ*ystarZ;				// MCFM NLO/LO
  double ystarZWeight = 0.85+0.15*fabs(ystarZ);										// MCFM NLO/LO NEW
//double mjjWeight = (1.02289) + (-9.81406e-05)*mjj;									// MCFM NLO/madGraph gen
//  double mjjWeight = (1.04886e+00) + (-1.67724e-04)*mjj;								// MCFM NLO/LO
  double mjjWeight = 0.39+0.12*log(mjj)-0.00025*mjj;									// MCFM NLO/LO NEW
  histos->multiplyEventWeight(ystarZWeight*mjjWeight);
}

/*****************
 * Z reweighting *
 *****************/
void ewkvAnalyzer::readEtaWeights(){
  std::ifstream readFile;
  readFile.open("../reweightingZ/etaWeigths_" + type + ".txt");
  readFile.ignore(unsigned(-1), '\n');
  double min, max, ratio;
  while(readFile >> min >> max >> ratio){
    etaWeights.push_back(ratio);
    etaBins.push_back(min);
  }
  etaBins.push_back(max);
  readFile.close();
}

void ewkvAnalyzer::etaReweighting(double eta){
  if(etaBins.size() == 0 || eta < etaBins.front() || eta > etaBins.back()){ 
    histos->multiplyEventWeight(0.);
    histos->setEventWeight(histos->getWeight()); cutflow->setEventWeight(histos->getWeight());
    return;
  }
  std::vector<TString> needReweighting = {"DY","DY0","DY1","DY2","DY3","DY4"};						// Only selected samples need reweighting
  if(std::find(needReweighting.begin(), needReweighting.end(), mySample->getName()) == needReweighting.end()) return;
  int i = 0;
  while((eta > etaBins[i+1])) ++i;
  histos->multiplyEventWeight(etaWeights[i]);
  histos->setEventWeight(histos->getWeight()); cutflow->setEventWeight(histos->getWeight());
}

void ewkvAnalyzer::readPtWeights(){
  std::ifstream readFile;
  readFile.open("../reweightingZ/ptWeigths_" + type + ".txt");
  readFile.ignore(unsigned(-1), '\n');
  double min, max, ratio;
  while(readFile >> min >> max >> ratio){
    ptWeights.push_back(ratio);
    ptBins.push_back(min);
  }
  ptBins.push_back(max);
  readFile.close();
}

void ewkvAnalyzer::ptReweighting(double pt){
  if(ptBins.size() == 0 || pt < ptBins.front() || pt > ptBins.back()){ 
    histos->multiplyEventWeight(0.);
    histos->setEventWeight(histos->getWeight()); cutflow->setEventWeight(histos->getWeight());
    return;
  }
  std::vector<TString> needReweighting = {"DY","DY0","DY1","DY2","DY3","DY4"};						// Only selected samples need reweighting
  if(std::find(needReweighting.begin(), needReweighting.end(), mySample->getName()) == needReweighting.end()) return;
  int i = 0;
  while((pt > ptBins[i+1])) ++i;
  histos->multiplyEventWeight(ptWeights[i]);
  histos->setEventWeight(histos->getWeight()); cutflow->setEventWeight(histos->getWeight());
}

/***************************
 * Multiplicity correction *
 ***************************/
void ewkvAnalyzer::QGCorrections(TString jet, TLorentzVector *j, std::vector<int> jetOrder){
  TString region;
  int jetNumber;
  if(jet == "j1") jetNumber = 0;
  else jetNumber = 1;

  if(fabs(j->Eta()) < 2) region = "_c";
  else if(fabs(j->Eta()) < 3) region = "_t";
  else region = "_f";

  for(TString product : {"axis1","axis2","mult","R","pull"}){
    TString identifier = type + product + "HIG13011_" + jet + region;
    float mult = jetQGvariables[product + "HIG13011"]->at(jetOrder.at(jetNumber));
    jetQGvariables[product + "HIG13011"]->at(jetOrder.at(jetNumber)) = ((mult - meanData[identifier])*sigmaMC[identifier]/sigmaData[identifier] + meanMC[identifier]);
  }

  std::map<TString, float> mvaVariables;
  mvaVariables["axis1"] = jetQGvariables["axis1HIG13011"]->at(jetOrder.at(jetNumber));
  mvaVariables["axis2"] = jetQGvariables["axis2HIG13011"]->at(jetOrder.at(jetNumber));
  mvaVariables["JetPull"] = jetQGvariables["pullHIG13011"]->at(jetOrder.at(jetNumber));
  mvaVariables["JetR"] = jetQGvariables["RHIG13011"]->at(jetOrder.at(jetNumber));
  if(fabs(j->Eta()) < 2){
    float mult = jetQGvariables["multHIG13011"]->at(jetOrder.at(jetNumber));
    float multLow = floor(mult);
    float multHigh = ceil(mult);
    mvaVariables["Mult"] = multLow;
    float low = qgTagger->getMVA(j->Eta(), j->Pt(), rho, mvaVariables);
    mvaVariables["Mult"] = multHigh;
    float high = qgTagger->getMVA(j->Eta(), j->Pt(), rho, mvaVariables);
    jetQGvariables["qgHIG13011"]->at(jetOrder.at(jetNumber)) = (high-low)/(multHigh-multLow)*(mult-multLow)+low;
  } else {
    mvaVariables["Mult"] = jetQGvariables["multHIG13011"]->at(jetOrder.at(jetNumber));
    jetQGvariables["qgHIG13011"]->at(jetOrder.at(jetNumber)) = qgTagger->getMVA(j->Eta(), j->Pt(), rho, mvaVariables);
  }
}
