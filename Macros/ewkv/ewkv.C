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
#define TMVATAG "20131022_InclusiveDY_ptZrew_BDT_100_STEP0"
#define TMVATYPE "BDT"
#define DYTYPE "composed"
#define OUTPUTTAG "20131103_Fast_STEP0"

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
      (*it)->useSkim(type, "20131022_Full");								//Use skimmed files to go faster
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

  // Loop with shifted pile-up for systematics
  for(TString puMode : {"","PUUp","PUDown"}){
    if(mySample->isData() && puMode != "") continue;
    histos->setBranch(puMode); cutflow->setBranch(puMode);

    // Get weight (lumi + pileUp) and muon efficiencies (ISO+ID, trigger not yet available)
    if(vType == ZMUMU) setWeight(mySample->getWeight(nPileUp, puMode)*mySample->muonEfficiency(&l1)*mySample->muonEfficiency(&l2));
    if(vType == ZEE)   setWeight(mySample->getWeight(nPileUp, puMode));
  
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
  
    TLorentzVector Z 		= TLorentzVector(l1 + l2);
    TVector3 leptonPlane 	= l1.Vect().Cross(l2.Vect());
  
    histos->fillHist1D("nPriVtxs", 	nPriVtxs);
    histos->fillHist1D("nPileUp", 	nPileUp);
  
    if(l1.Pt() < 20 || l2.Pt() < 20 || Z.Pt() <= 0) continue;
  
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
    if(fabs(Z.M() - ZMASS) > 40) continue;
    histos->fillHist1D("dilepton_mass", 	Z.M());
    if(fabs(Z.M() - ZMASS) > 20) continue;
    cutflow->track("$\\mid m_Z-m_{ll} \\mid < 20$ GeV"); 
  
    histos->fillHist1D("dilepton_pt", 	Z.Pt());
    histos->fillHist1D("dilepton_eta", 	Z.Eta());
    histos->fillHist1D("dilepton_phi", 	Z.Phi());
    checkRadiationPattern(Z.Rapidity());
  
  
    // Set up parallel branches in the histograms/cutflow for JES and JER
    for(TString subBranch : {"", "JESUp", "JESDown", "JERUp", "JERDown"}){
      TString branch = puMode + subBranch;
      if((mySample->isData() || puMode != "") && subBranch != "") continue;
      histos->setBranch(branch); cutflow->setBranch(branch);

      int JESsign = 0, JERsign = 0;
      if(subBranch == "JESUp") JESsign = 1;
      if(subBranch == "JESDown") JESsign = -1;
      if(subBranch == "JERUp") JERsign = 1;
      if(subBranch == "JERDown") JERsign = -1;
  
      // Sort leading jets again after jet energy corrections
      std::vector<int> jetOrder;
      std::vector<double> jetPt;
      for(int j=0; j < vJets->GetEntries(); ++j){
        TLorentzVector jet = *((TLorentzVector*) vJets->At(j));
        jet *= (1+JESsign*jetUncertainty[j]);
        jet *= (1+JERsign*(jetSmearedPt[j]-jet.Pt())/jet.Pt());
        if(jet.Pt() <= 0 || fabs(jet.Eta()) > JETETA) continue;
        int k = 0;
        while(k < jetOrder.size() && (jet.Pt() < jetPt.at(k))) ++k;
        jetOrder.insert(jetOrder.begin() + k, j);
        jetPt.insert(jetPt.begin() + k, jet.Pt());
      }
      if(jetOrder.size() < 2) continue;
      cutflow->track("2 jets, no $p_T$ cut");
 
      TLorentzVector j1 = *((TLorentzVector*) vJets->At(jetOrder.at(0)));
      TLorentzVector j2 = *((TLorentzVector*) vJets->At(jetOrder.at(1)));
      j1 *= (1+JESsign*jetUncertainty[jetOrder.at(0)]);
      j1 *= (1+JERsign*(jetSmearedPt[jetOrder.at(0)]-j1.Pt())/j1.Pt());
      j2 *= (1+JESsign*jetUncertainty[jetOrder.at(1)]);
      j2 *= (1+JERsign*(jetSmearedPt[jetOrder.at(1)]-j2.Pt())/j2.Pt());
      double etamin = (j1.Eta() < j2.Eta()? j1.Eta() : j2.Eta()) + 0.5;
      double etamax = (j1.Eta() < j2.Eta()? j2.Eta() : j1.Eta()) - 0.5;
  
      histos->fillHist1D("jet1_pt", 		j1.Pt());
      histos->fillHist1D("jet1_pt_log", 	j1.Pt());
  
      histos->fillHist1D("jet2_pt", 		j2.Pt());
      histos->fillHist1D("jet2_pt_log", 	j2.Pt());

      if(j1.Pt() < JET1PT) continue;
      if(j2.Pt() < JET2PT) continue;
      cutflow->track("2 jets"); 
      fillSkimTree();

      TLorentzVector jj = TLorentzVector(j1 + j2);
      TLorentzVector all = TLorentzVector(l1 + l2 + j1 + j2);
      double dy = j1.Rapidity() - j2.Rapidity();
      double ystarZ = Z.Rapidity() - (j1.Rapidity() + j2.Rapidity())/2;
      double zstarZ = ystarZ/dy;
 
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

      for(TString product : {"qg","axis1","axis2","mult","ptD"}){
        histos->fillHist1D(product + "MLP_j1", jetQGvariables[product + "MLP"]->at(jetOrder.at(0)));
        histos->fillHist1D(product + "MLP_j2", jetQGvariables[product + "MLP"]->at(jetOrder.at(1)));
      }
      for(TString product : {"qg","axis2","mult","ptD"}){
      	histos->fillHist1D(product + "Likelihood_j1", jetQGvariables[product + "Likelihood"]->at(jetOrder.at(0)));
        histos->fillHist1D(product + "Likelihood_j2", jetQGvariables[product + "Likelihood"]->at(jetOrder.at(1)));
      }
      for(TString product : {"axis1","axis2","mult","R","pull"}){
        histos->fillHist1D(product + "HIG13011_j1", jetQGvariables[product+"HIG13011"]->at(jetOrder.at(0)));
        histos->fillHist1D(product + "HIG13011_j2", jetQGvariables[product+"HIG13011"]->at(jetOrder.at(1)));
      }

      // Add systematic branches for MCFM reweightig
      saveWeight();
      for(TString mcfmSyst : {"", "mjjUp", "ystarUp", "mcfmUp"}){
        if((mySample->isData() || (puMode + subBranch) != "") && mcfmSyst != "") continue;
        branch = puMode + subBranch + mcfmSyst;
        histos->setBranch(branch); cutflow->setBranch(branch);
        restoreWeight();										// Go back to normal event weight before next reweighting
        if(mcfmSyst == "mjjUp") 	mcfmReweighting(jj.M(), -1);
        if(mcfmSyst == "ystarUp")	mcfmReweighting(-1, ystarZ);
        if(mcfmSyst == "mcfmUp") 	mcfmReweighting(jj.M(), ystarZ);
  
        // Add systematic branch for QG smearing
        for(TString qgSyst : {"","QGUp"}){
          if((mySample->isData() || (puMode + subBranch + mcfmSyst) != "") && qgSyst != "") continue;
          branch = puMode + subBranch + mcfmSyst + qgSyst;
          histos->setBranch(branch); cutflow->setBranch(branch);
  
          float HIG13011_j1 = jetQGvariables["qgHIG13011"]->at(jetOrder.at(0)); 
          float HIG13011_j2 = jetQGvariables["qgHIG13011"]->at(jetOrder.at(1));
          if(qgSyst == "QGUp"){
            HIG13011_j1 = QGsmearing(&j1, HIG13011_j1);
            HIG13011_j2 = QGsmearing(&j2, HIG13011_j2);
          }
          histos->fillHist1D("qgHIG13011_j1", HIG13011_j1);
          histos->fillHist1D("qgHIG13011_j2", HIG13011_j2);
 
          // TMVA tree variables (variables for training tree, for the actually used variables, see the TMVA init function)
          tmvaVariables["pT_Z"] = 		Z.Pt();
          tmvaVariables["pT_j1"] = 		j1.Pt();
          tmvaVariables["pT_j2"] = 		j2.Pt();
          tmvaVariables["pT_jj"] = 		jj.Pt();
          tmvaVariables["eta_Z"] = 		Z.Eta();
          tmvaVariables["abs(eta_Z)"] = 	fabs(Z.Eta());
          tmvaVariables["dPhi_j1"] = 		fabs(Z.DeltaPhi(j1));
          tmvaVariables["dPhi_j2"] = 		fabs(Z.DeltaPhi(j2));
          tmvaVariables["dPhi_jj"] = 		fabs(j1.DeltaPhi(j2)); 
          tmvaVariables["dEta_jj"] = 		fabs(j1.Eta() - j2.Eta());
          tmvaVariables["avEta_jj"] = 		fabs((j1.Eta() + j2.Eta())/2); 
          tmvaVariables["qgMLP_j1"] = 		jetQGvariables["qgMLP"]->at(jetOrder.at(0));
          tmvaVariables["qgMLP_j2"] = 		jetQGvariables["qgMLP"]->at(jetOrder.at(1));
          tmvaVariables["qgLikelihood_j1"] = 	jetQGvariables["qgLikelihood"]->at(jetOrder.at(0));
          tmvaVariables["qgLikelihood_j2"] = 	jetQGvariables["qgLikelihood"]->at(jetOrder.at(1));
          tmvaVariables["qgHIG13011_j1"] = 	HIG13011_j1;
          tmvaVariables["qgHIG13011_j2"] = 	HIG13011_j2;
          tmvaVariables["M_jj"] = 		jj.M();
          tmvaVariables["ystarZ"] = 		fabs(ystarZ);
          tmvaVariables["zstarZ"] = 		fabs(zstarZ);
          tmvaVariables["weight"] = 		getWeight();
          if(branch == "") fillTMVAtree();
     
          double mvaValue = tmvaReader->EvaluateMVA(TMVATYPE); 
    
          histos->fillHist1D(TMVATYPE, 						mvaValue);
          if(jj.M() > 100) histos->fillHist1D(TString(TMVATYPE)+"_100", 	mvaValue);
          if(jj.M() > 200) histos->fillHist1D(TString(TMVATYPE)+"_200", 	mvaValue);
        }  
        branch = puMode + subBranch + mcfmSyst;
        histos->setBranch(branch); cutflow->setBranch(branch);
  
        histos->fillHist1D("dijet_mass", 		jj.M());
        histos->fillHist1D("dijet_pt", 			jj.Pt());
        histos->fillHist1D("dijet_dphi", 		fabs(j1.DeltaPhi(j2)));
        histos->fillHist1D("dijet_deta", 		fabs(j1.Eta() - j2.Eta()));
        histos->fillHist1D("dijet_av_eta", 		(j1.Eta() + j2.Eta())/2);
        histos->fillHist1D("dijet_sum_pt", 		(j1.Pt() + j2.Pt()));
    
        histos->fillHist1D("jet1_Z_dphi", 		fabs(Z.DeltaPhi(j1)));
        histos->fillHist1D("jet2_Z_dphi", 		fabs(Z.DeltaPhi(j2)));
    
        histos->fillHist1D("ystar_Z", 			fabs(ystarZ));
        histos->fillHist1D("zstar_Z", 			fabs(zstarZ));
    
        // Zeppenfeld variables for the 3rd jet
        if(jetOrder.size() > 2){
          TLorentzVector j3 = *((TLorentzVector*) vJets->At(jetOrder.at(2)));
          j3 *= (1+JESsign*jetUncertainty[jetOrder.at(2)]);
          j3 *= (1+JERsign*(jetSmearedPt[jetOrder.at(2)]-j3.Pt())/j3.Pt());
          if(j3.Pt() > JET3PT){
            double ystar3 = j3.Rapidity() - (j1.Rapidity() + j2.Rapidity())/2;
            histos->fillHist1D("ystar_3", 		fabs(ystar3));
            histos->fillHist1D("zstar_3", 		fabs(ystar3/fabs(dy)));
          }
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
        histos->fillHist1D("totalSoftHT", 		totalSoftHT);
        histos->fillProfileHist("softHT3_vs_PV", 	nPriVtxs, 			stjHT3);
        histos->fillProfileHist("softHT3_vs_mjj", 	jj.M(), 			stjHT3);
        histos->fillProfileHist("softHT3_vs_detajj", 	fabs(j1.Eta() - j2.Eta()), 	stjHT3);
        histos->fillProfileHist("softHT_vs_PV", 	nPriVtxs, 			totalSoftHT);
        histos->fillProfileHist("softHT_vs_mjj", 	jj.M(), 			totalSoftHT);
        histos->fillProfileHist("softHT_vs_detajj", 	fabs(j1.Eta() - j2.Eta()), 	totalSoftHT);
    
    
        // Central activity with jets
        int nCentralJets = 0; double centralHT = 0;
        for(int j=0; j < vJets->GetEntries(); ++j){
          if(j == jetOrder.at(0) || j == jetOrder.at(1)) continue;
          TLorentzVector jet = *((TLorentzVector*) vJets->At(j));
          if(jet.Pt() < JET3PT) continue;
          if((fabs(jet.Eta()) > etamax) || (fabs(jet.Eta()) < etamin)) continue;
          jet *= (1+JESsign*jetUncertainty[j]);
          jet *= (1+JERsign*(jetSmearedPt[j]-jet.Pt())/jet.Pt());
          if(nCentralJets == 0) histos->fillHist1D("centralJet1_pt", jet.Pt());
          centralHT += jet.Pt();
          ++nCentralJets;
        }
        histos->fillHist1D("nCentralJets", 			nCentralJets);
        if(nCentralJets > 0) histos->fillHist1D("centralHT",	centralHT);
    
    
        // Additional cutflow
        if(jj.M() < 200) continue;			cutflow->track("$m_{jj} > 200$ GeV");
        if(fabs(ystarZ)> 1.2) continue;    		cutflow->track("$\\mid y^{*} \\mid < 1.2$");
        if(jj.M() < 600) continue;    			cutflow->track("$m_{jj} > 600$ GeV");
        if(fabs(j1.Eta() - j2.Eta()) < 3.5) continue;	cutflow->track("$\\Delta\\eta_{jj} > 3.5$");
      }
      restoreWeight();
    }
  }
  return;
}  

/*************************************
 * Initialization of the TMVA reader *
 *************************************/
void ewkvAnalyzer::initTMVAreader(){
  tmvaReader = new TMVA::Reader("Silent");
  TString weightsFile = getTreeLocation() + "tmvaWeights/" + type + "/" + TMVATAG + "/weights/TMVAClassification_" + TMVATYPE + ".weights.xml";

  ifstream xmlFile;
  xmlFile.open(weightsFile.Data());
  if(xmlFile.is_open()){
    std::string line;
    while(getline(xmlFile,line)){
      std::string::size_type start = line.find("Expression=\"");
      if(start != std::string::npos){
        TString variable = TString(line.substr(start + 12, line.find("\" Label") - start - 12));
        tmvaReader->AddVariable( variable, &tmvaVariables[variable]);
      }
    }
  } else std::cout << "ewkv.C:\t\t!!!\tTMVA XML file not found!" << std::endl;

  tmvaReader->BookMVA( TMVATYPE, weightsFile);
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
    saveWeight();
    mcfmReweighting(mjj, ystarZ);											// Use MCFM reweighting
    histos->fillProfileHist("nJets_vs_detajj", 		dEta, 	nJets);
    histos->fillProfileHist("cosdPhi_vs_HT", 		HT, 	cosDPhi);
    histos->fillProfileHist("cosdPhi_vs_detajj", 	dEta, 	cosDPhi);
    restoreWeight();												// Go back to normal event weight
  }
}


/***************************
 * MCFM NLO/LO reweighting *
 ***************************/
void ewkvAnalyzer::mcfmReweighting(double mjj, double ystarZ){
  std::vector<TString> needReweighting = {"DY","DY2","DY3","DY4"};							// Only selected samples need reweighting
  if(std::find(needReweighting.begin(), needReweighting.end(), mySample->getName()) == needReweighting.end()) return;
  double ystarZWeight = 0.85+0.15*fabs(ystarZ);										// MCFM NLO/LO NEW
  double mjjWeight = 0.39+0.12*log(mjj)-0.00025*mjj;									// MCFM NLO/LO NEW
  if(mjj < 200 || mjj == -1) mjjWeight = 1.;
  if(ystarZ == -1) ystarZWeight = 1.;
  multiplyWeight(ystarZWeight*mjjWeight);
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
  if(etaBins.size() == 0 || eta < etaBins.front() || eta > etaBins.back()){ multiplyWeight(0.); return;}
  std::vector<TString> needReweighting = {"DY","DY0","DY1","DY2","DY3","DY4"};						// Only selected samples need reweighting
  if(std::find(needReweighting.begin(), needReweighting.end(), mySample->getName()) == needReweighting.end()) return;
  int i = 0;
  while((eta > etaBins[i+1])) ++i;
  multiplyWeight(etaWeights[i]);
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
  if(ptBins.size() == 0 || pt < ptBins.front() || pt > ptBins.back()){ multiplyWeight(0.); return;}
  std::vector<TString> needReweighting = {"DY","DY0","DY1","DY2","DY3","DY4"};						// Only selected samples need reweighting
  if(std::find(needReweighting.begin(), needReweighting.end(), mySample->getName()) == needReweighting.end()) return;
  int i = 0;
  while((pt > ptBins[i+1])) ++i;
  multiplyWeight(ptWeights[i]);
}

/******************
 * QG corrections *
 ******************/
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


/***************
 * QG smearing *
 ***************/
float ewkvAnalyzer::QGsmearing(TLorentzVector *j, float input){
  float dRmin = .3; int partonID = 0;
  for(int i = 0; i < nGenPart; ++i){
    double dR = j->DeltaR(*((TLorentzVector*) (vGenPart->At(i))));
    if(dR < dRmin){
      dRmin = dR;
      partonID = idGenPart[i];
    }
  }
  bool quark;
  if(partonID != 0 && fabs(partonID) < 6) quark = true;
  else if(partonID == 21) quark = false;
  else return input;

  float a, b;
  if(fabs(j->Eta()) < 2.5){
    a = (quark? 1.020 : 1.030);
    b = (quark? -0.100 : 0.204);
  } else {
    if(j->Pt() < 40){
      a = (quark? 1.000 : 1.030);
      b = (quark? -0.000 : 0.200);
    } else {
      a = (quark? 0.990 : 0.980);
      b = (quark? -0.200 : 0.010);
    }
  } 
  float output = TMath::TanH(a*TMath::ATanH(2.*input-1)+b)/2+.5;
  if(output < 0) output = 0;
  if(output > 1) output = 1;
  return output;
}
