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

// Constants
#define ZMASS 91.1876

// Our header-files
#include "../samples/sampleList.h"
#include "../samples/sample.h"
#include "../histos/histos.h"
#include "../CMSSWBASE.h"
#include "../cutFlow/cutFlow.h"
#include "../cutFlow/cutFlowHandler.h"
#include "ewkv.h"


/*****************
 * Main function *
 *****************/
int main(){
  gROOT->SetBatch();
  TString outputTag = "20130502";
  for(TString type : {"ZMUMU"}){

    sampleList* samples = new sampleList();
    TString samplesDir = getCMSSWBASE() + "/src/EWKV/Macros/samples/";
    if(type == "ZMUMU" && !samples->init(samplesDir + "data.config", samplesDir + "mc.config")) return 1;			//Set up list of samples
    if(type == "ZEE" && !samples->init(samplesDir + "dataDoubleElectron.config", samplesDir + "mc.config")) return 1;

    TString outputDir = getCMSSWBASE() + "/src/EWKV/Macros/outputs/";
    TFile *outFile = new TFile(outputDir + "rootfiles/" + type + "/" + outputTag + ".root", "RECREATE");
    cutFlowHandler* cutflows = new cutFlowHandler();
    for(sampleList::iterator it = samples->begin(); it != samples->end(); ++it){			//loop over samples
      ewkvAnalyzer *myAnalyzer = new ewkvAnalyzer(*it, outFile);					//set up analyzer
//    myAnalyzer->setMakeTMVAtree("/afs/cern.ch/work/t/tomc/public/EWKV/2013-03/tmva-input/"+type+"/"); //Use if TMVA input trees has to be remade
      myAnalyzer->loop(type);										//loop over events in tree
      myAnalyzer->getHistoCollection()->toFile();							//write all the histograms to file				
      cutflows->add(myAnalyzer->getCutFlow());								//get the cutflow
      delete myAnalyzer;
    }
    outFile->Close();
    cutflows->toLatex(outputDir + "cutflow/" + type + "/" + outputTag + "_notmerged.tex");
    cutflows->merge("Diboson",{"WW","WZ","ZZ"});
    cutflows->merge("Single top",{"T-s","T-t","T-W","Tbar-s","Tbar-t","Tbar-W"});
    cutflows->merge("QCD",{"QCD100","QCD250","QCD500","QCD1000"});
    cutflows->toLatex(outputDir + "cutflow/" + type + "/" + outputTag + ".tex");
    delete cutflows;
    delete samples;
    delete outFile;
  }

  return 0;
}



/*******************************************
 * Function called for every event in tree *
 *******************************************/
void ewkvAnalyzer::analyze_Zjets(){
  // Trigger 
  if((vType == ZMUMU) && !(Mu17_Mu8 || Mu17_TkMu8)) return;
  if((vType == ZEE) && !(Ele17_Ele8 || Ele17T_Ele8T)) return;

  histos->fillHist1D("nPriVtxs", nPriVtxs);

  // Get lorentzvectors (l+ in l1 and l- in l2) + construct Z boson
  TLorentzVector l1 	= *((TLorentzVector*) vLeptons->At(leptonCharge[0] == 1? 0 : 1));
  TLorentzVector l2 	= *((TLorentzVector*) vLeptons->At(leptonCharge[0] == 1? 1 : 0));
  TLorentzVector Z 	= TLorentzVector(l1 + l2);
  TVector3 leptonPlane = l1.Vect().Cross(l2.Vect());

  histos->setFillWeight(mySample->getWeight(nPileUp));		//TO DO: update sample::getWeight function with lepton efficiencies
  cutflow->setFillWeight(mySample->getWeight(nPileUp));

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

/*
    // Radiation patterns section
    int nJets = 0;
    double jetHT = 0, dEta = 0, cosDPhi = 0;
    for(int j=0; j < vJets->GetEntries(); ++j){
      TLorentzVector jet = *((TLorentzVector*) vJets->At(j));
      if(jet.Pt() > 40 && fabs(jet.Eta()) < 3.6){
        ++nJets;
        jetHT += jet.Pt();
      }
      for(int k=0; k < j; ++k){
        TLorentzVector otherjet = *((TLorentzVector*) vPF5Jets->At(k));
        if(jet.Pt() > 40 && fabs(jet.Eta()) < 3.6){
          if(fabs(otherjet.Eta() - jet.Eta()) > dEta){
            dEta = fabs(otherjet.Eta() - jet.Eta());
            cosDPhi = TMath::Cos(fabs(jet.DeltaPhi(otherjet)));
          }
        }
      }
    }
    if(nJets > 0) histos->fillProfileHist("nJets_vs_HT", HT , nJets);
    if(nJets > 1){
        histos->fillProfileHist("nJets_vs_dEta_jj", dEta , nJets);
        histos->fillProfileHist("cosDPhi_vs_HT", HT , cosDPhi);
    }*/

    //Generator level cuts
    if(mySample->getName() == "signal"){
      int nGenLeptons = 0, nGenQJets = 0;
      for(int g = 0; g < nGenPart; ++g){
        TLorentzVector genParticle = *((TLorentzVector*) vGenPart->At(g));
        if(nGenLeptons < 2 && ((fabs(idGenPart[g]) == 11) || (fabs(idGenPart[g]) == 13))){
          if(fabs(genParticle.Eta()) > 2.4) continue;
          if(genParticle.Pt() < 20) continue;
	  ++nGenLeptons;
        }
        if(nGenQJets < 2 && (fabs(idGenPart[g]) < 6)){
          if(fabs(genParticle.Eta()) > 3.6) continue;
          if((nGenQJets == 0) && (fabs(genParticle.Pt()) < 65)) continue;
          if((nGenQJets == 1) && (fabs(genParticle.Pt()) < 40)) continue;
	  ++nGenQJets;
        }
      }
      if(nGenLeptons < 2 && nGenQJets < 2){
        if(branch != "") continue;
        histos->setBranch("fake");
        cutflow->setBranch("fake");
      } else cutflow->track("GEN level cuts");
    } else cutflow->track("GEN level cuts");

 

    // Find leading jets (at least 3) and sort
    std::vector<int> leadingJets;
    for(int j=0; j < vJets->GetEntries(); ++j){
      if(jetUncertainty[j] > .5) continue;
      TLorentzVector jet = *((TLorentzVector*) vJets->At(j));
      if(branch == "JES-") jet *= (1-jetUncertainty[j]);
      if(branch == "JES+") jet *= (1+jetUncertainty[j]);

      if(fabs(jet.Eta()) > 3.6) continue;
      for(auto k = leadingJets.begin(); k != leadingJets.end(); ++k){
        TLorentzVector leadingJet = *((TLorentzVector*) vJets->At(*k));
        if(jet.Pt() > leadingJet.Pt()){
          leadingJets.insert(k, j);
          break;
        }
      }
      if(leadingJets.size() < 3) leadingJets.push_back(j);
    }
    if(leadingJets.size() < 2) continue;
    TLorentzVector j1 = *((TLorentzVector*) vJets->At(leadingJets.at(0)));
    TLorentzVector j2 = *((TLorentzVector*) vJets->At(leadingJets.at(1)));
    if(branch == "JES-") j1 *= (1-jetUncertainty[leadingJets.at(0)]);
    if(branch == "JES+") j1 *= (1+jetUncertainty[leadingJets.at(0)]);
    if(branch == "JES-") j2 *= (1-jetUncertainty[leadingJets.at(1)]);
    if(branch == "JES+") j2 *= (1+jetUncertainty[leadingJets.at(1)]);
    if(j1.Pt() < 65) continue;
    if(j2.Pt() < 40) continue;
    cutflow->track("2 jets"); 


    //Analyze jets
    TLorentzVector jj = TLorentzVector(j1 + j2);
    TLorentzVector all = TLorentzVector(l1 + l2 + j1 + j2);

    histos->fillHist1D("jet1_pt", j1.Pt());
    histos->fillHist1D("jet1_pt_log", j1.Pt());
    histos->fillHist1D("jet1_eta", j1.Eta());
    histos->fillHist1D("jet1_phi", j1.Phi());

    histos->fillHist1D("jet2_pt", j2.Pt());
    histos->fillHist1D("jet2_pt_log", j2.Pt());
    histos->fillHist1D("jet2_eta", j2.Eta());
    histos->fillHist1D("jet2_phi", j2.Phi());

    histos->fillHist1D("dijet_mass", jj.M());
    histos->fillHist1D("dijet_pt", jj.Pt());
    histos->fillHist1D("dijet_dphi", fabs(j1.DeltaPhi(j2)));
    histos->fillHist1D("dijet_deta", fabs(j1.Eta() - j2.Eta()));

    histos->fillHist1D("QGMLP_j1", jetQGMLP[leadingJets.at(0)]);
    histos->fillHist1D("QGMLP_j2", jetQGMLP[leadingJets.at(1)]);
    histos->fillHist1D("QGLikelihood_j1", jetQGLikelihood[leadingJets.at(0)]);
    histos->fillHist1D("QGLikelihood_j2", jetQGLikelihood[leadingJets.at(1)]);

    // Zeppenfeld variable
    double Zeppenfeld = Z.Rapidity() - (j1.Rapidity() + j2.Rapidity())/2;
    histos->fillHist1D("ystar", fabs(Zeppenfeld));


    // Soft track jets
    double etamin = (j1.Eta() < j2.Eta()? j1.Eta() : j2.Eta());
    double etamax = (j1.Eta() < j2.Eta()? j2.Eta() : j1.Eta());

    int nStj = 0; 
    double stjHT = 0, stjHT3 = 0;
    for(int t=0; t < vSoftTrackJets->GetEntries(); ++t){
      TLorentzVector softTrackJet = *((TLorentzVector*) vSoftTrackJets->At(t));
      if((softTrackJet.Pt() > 500) || (softTrackJet.Eta() > (etamax - 0.5)) || (softTrackJet.Eta() < (etamin + 0.5))) continue;
      stjHT += softTrackJet.Pt();
      if(nStj < 2) stjHT3 += softTrackJet.Pt();
      if(nStj == 0) histos->fillHist1D("softTrackJet1_pt", softTrackJet.Pt());
      ++nStj;
    }
    histos->fillHist1D("stjHT3", stjHT3);
/*
    histos->fillProfileHist("HT_vs_PV", nPriVtxs, sumPt3);
    histos->fillProfileHist("HT_vs_Mjj", jj.M(), sumPt3);
    histos->fillProfileHist("HT_vs_dEta_jj", dEta_jj , sumPt3);
*/

    // Additional cutflow
    if(jj.M() < 200) continue;
    cutflow->track("$m_{jj} > 200$ GeV");

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
    tmvaVariables["qgMLP_j1"] = jetQGMLP[leadingJets.at(0)];
    tmvaVariables["qgMLP_j2"] = jetQGMLP[leadingJets.at(1)];
    tmvaVariables["qgLikelihood_j1"] = jetQGLikelihood[leadingJets.at(0)];
    tmvaVariables["qgLikelihood_j2"] = jetQGLikelihood[leadingJets.at(1)];
    tmvaVariables["M_jj"] = jj.M();
    tmvaVariables["weight"] = mySample->getWeight(nPileUp);
    fillTMVAtree();

  //  histos->fillHist1D("BDTD", tmvaReader->EvaluateMVA("BDTD"));


    if(fabs(Zeppenfeld)> 1.2) continue;
    cutflow->track("$\\mid y^{*} \\mid < 1.2$");

    if(jj.M() < 600) continue;
    cutflow->track("$m_{jj} > 600$ GeV");

    if(fabs(j1.Eta() - j2.Eta()) < 3.5) continue;
    cutflow->track("$\\Delta\\eta_{jj} > 3.5$");
  }
  return;
}


void ewkvAnalyzer::initTMVAreader(TString type){
//  tmvaReader = new TMVA::Reader("");

//  std::vector<TString> variables = {"pT_Z", "pT_j1", "pT_j2", "eta_Z", "dPhi_j1", "dPhi_j2", "dPhi_jj", "dEta_jj", "avEta_jj", "qgMLP_j1", "qgMLP_j2", "M_jj"};
//  for(TString variable : variables) tmvaReader->AddVariable( variable, &tmvaVariables[variable]);

//  tmvaReader->BookMVA( "BDTD", "../TMVAtraining/"+type+"/weights/TMVAClassification_BDT.weights.xml" );
}



 /**************************************************************************
  * Everything below is from the 7TeV analysis, still to add in this macro *
  **************************************************************************/

/* 
  TFile *MCFMreweightingFile = new TFile("~tomc/public/Rat_MCFM_NLO_Madgraph.root");
  TH1F *MCFMreweighting = (TH1F*) MCFMreweightingFile->Get("hMjj_ratnlolo");
*/

/*



      //MCFM reweighting
      Int_t bin = MCFMreweighting->FindBin(M_jj);
      Float_t MjjWeight = MCFMreweighting->GetBinContent(bin);
      histos->setFillWeight(samples->getWeight(inputfile, nPileUp, l1.Eta(), l2.Eta())*MjjWeight);	//multiply with Mjj weight
      histos->fillHist1D("TMVA_BDTD_MCFMreweighted", mvaValue_BDTD);
      histos->setFillWeight(samples->getWeight(inputfile, nPileUp, l1.Eta(), l2.Eta()));		//restore to original weight


      //distort gluonLike
      gluonLike_j1 = (1.1-0.2*gluonLike_j1)*gluonLike_j1;
      gluonLike_j2 = (1.1-0.2*gluonLike_j2)*gluonLike_j2;

      Float_t mvaValue_BDTD_dist = reader->EvaluateMVA("BDTD");

      //use pt smeared
      TLorentzVector j1smeared = j1;
      TLorentzVector j2smeared = j2;
      j1smeared *= (PF5JetSmearedPt[0]/j1.Pt());
      j2smeared *= (PF5JetSmearedPt[1]/j2.Pt());
      TLorentzVector jjsmeared = TLorentzVector(j1smeared + j2smeared);

      pT_j1 = j1.Pt();
      pT_j2 = j2.Pt();
      dPhi_j1 = fabs(Z.DeltaPhi(j1smeared));
      dPhi_j2 = fabs(Z.DeltaPhi(j2smeared));
      dPhi_jj = fabs(j1smeared.DeltaPhi(j2smeared));
      dEta_jj = fabs(j1smeared.Eta()-j2smeared.Eta());
      avEta_jj = fabs((j1smeared.Eta()+j2smeared.Eta())/2);
      gluonLike_j1 = PF5JetGluonLike[restrictedJets[0]];
      gluonLike_j2 = PF5JetGluonLike[restrictedJets[1]];
      M_jj = jjsmeared.M();


      Float_t mvaValue_BDTD_smearedPt = reader->EvaluateMVA("BDTD");

         
//    if(inputfile == 1) histos->setFillWeight(samples->getWeight(inputfile, nPileUp, l1.Eta(), l2.Eta())*1.17);
//    else if(inputfile != 0) histos->setFillWeight(samples->getWeight(inputfile, nPileUp, l1.Eta(), l2.Eta())*0.957);
      histos->fillHist1D("TMVA_BDTD", mvaValue_BDTD);
//    histos->setFillWeight(samples->getWeight(inputfile, nPileUp, l1.Eta(), l2.Eta()));		//restore to original weight
      histos->fillHist1D("TMVA_BDTD_dist", mvaValue_BDTD_dist);
      histos->fillHist1D("TMVA_BDTD_smearedPt", mvaValue_BDTD_smearedPt);

      //Cuts
      if(mvaValue_BDTD > 0.25) cutFlow->track("BDTD $>$ .25");
      if(mvaValue_BDTD > 0.28) cutFlow->track("BDTD $>$ .28");
      if(mvaValue_BDTD > 0.31) cutFlow->track("BDTD $>$ .31");
      if(mvaValue_BDTD > 0.34) cutFlow->track("BDTD $>$ .34");
      if(mvaValue_BDTD > 0.37) cutFlow->track("BDTD $>$ .37");
      if(mvaValue_BDTD > 0.40) cutFlow->track("BDTD $>$ .40");


  if(FROMSCRATCH && MAKESKIM) skimmedFile->WriteTObject(skimmedTree);
}*/
