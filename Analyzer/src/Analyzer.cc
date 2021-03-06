/* Analyzer.cc
 * Package:	EWKV/Analyzer
 * Author:	Tom Cornelis, Paolo Azzurri, Alex Van Spilbeeck
 * Update:	2013/03/27
 * Based on:	http://cmssw.cvs.cern.ch/cgi-bin/cmssw.cgi/UserCode/PaoloA/VBFZ/Analyzer/src/Analyzer.cc?view=markup
 * 
 * Analyzer class for the EWKV analysis
 */

#include <memory>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h"
#include "SimDataFormats/GeneratorProducts/interface/LHEEventProduct.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "DataFormats/JetReco/interface/GenJetCollection.h"
#include "DataFormats/JetReco/interface/PFJet.h"
#include "DataFormats/JetReco/interface/PFJetCollection.h"
#include "DataFormats/METReco/interface/PFMETCollection.h"
#include "DataFormats/METReco/interface/PFMET.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "JetMETCorrections/Objects/interface/JetCorrector.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectionUncertainty.h"
#include "JetMETCorrections/Objects/interface/JetCorrectionsRecord.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectorParameters.h"
#include "DataFormats/JetReco/interface/TrackJetCollection.h"
#include "HLTrigger/HLTcore/interface/HLTConfigProvider.h"
#include "DataFormats/HLTReco/interface/HLTPrescaleTable.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include "FWCore/Common/interface/TriggerNames.h"
#include "CMGTools/External/interface/PileupJetIdentifier.h"

#include "TH1.h"
#include "TFile.h"
#include "TTree.h"
#include "TLorentzVector.h"
#include "TVector2.h"
#include "TClonesArray.h"

#include "../interface/Analyzer.h"


Analyzer::Analyzer(const edm::ParameterSet& iConfig) :
  fileName(			TString(iConfig.getUntrackedParameter<std::string>("fileName","ewkv.root"))),
  HLT_paths(            	iConfig.getParameter<std::vector<std::string> > ("HLT_paths")),
  HLT_process( 	        	iConfig.getParameter<std::string> ("HLT_process")),
  genJetsInputTag(    	     	iConfig.getParameter<edm::InputTag>("genJetsInputTag")),
  pfJetsNoVInputTag(    	iConfig.getParameter<edm::InputTag>("pfJetsNoVJetsInputTag")),
  pfLeptonsInputTag(    	iConfig.getParameter<edm::InputTag>("pfLeptonsInputTag")),
  metInputTag(            	iConfig.getParameter<edm::InputTag>("metInputTag")),
  metCorrInputTag(            	iConfig.getParameter<edm::InputTag>("metCorrInputTag")),
  metCorrNoVInputTag(           iConfig.getParameter<edm::InputTag>("metCorrNoVInputTag")),
  softTrackJetsInputTag(    	iConfig.getParameter<edm::InputTag>("softTrackJetsInputTag")),
  rhoInputTag(    	     	iConfig.getParameter<edm::InputTag>("rhoInputTag")),
  primaryVertexInputTag(  	iConfig.getParameter<edm::InputTag>("primaryVertexInputTag"))
{
}


void Analyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup){
  nRun 		= (int) iEvent.id().run();
  nEvent	= (int) iEvent.id().event();
  nLumi 	= (int) iEvent.id().luminosityBlock();

 /*********************
  * LHE event product *
  *********************/
  edm::Handle<LHEEventProduct> lheH; 
  iEvent.getByType(lheH);
  if(lheH.isValid()) nParticleEntries = lheH->hepeup().NUP; 
  else nParticleEntries = -1;

 /*********************************************************************************************
  * Get pile-up, the jet rho for L1 PU corrections and the number of offline primary vertices *
  *********************************************************************************************/
  edm::Handle<std::vector<PileupSummaryInfo>>  PupInfo;
  iEvent.getByLabel("addPileupInfo", PupInfo);

  if(PupInfo.isValid()){
    for(std::vector<PileupSummaryInfo>::const_iterator PVI = PupInfo->begin(); PVI != PupInfo->end(); ++PVI){
      if(PVI->getBunchCrossing() == 0){ 
	nPileUp = PVI->getPU_NumInteractions();
	nTrue = PVI->getTrueNumInteractions();
	continue;
      }
    }  
  } else {nPileUp = -1, nTrue = -1;}

  edm::Handle<double> rho;
  iEvent.getByLabel(rhoInputTag, rho);
  rhokt6PFJets = *rho;

  edm::Handle<reco::VertexCollection> vtxs;
  iEvent.getByLabel(primaryVertexInputTag , vtxs);
  nPriVtxs = vtxs->size(); 


 /*****************
  * GEN particles *
  *****************/
  edm::Handle<reco::GenParticleCollection> genParticles;
  iEvent.getByLabel("genParticles", genParticles);

  nGenPart = 0;
  vGenPart->Clear();
  if(genParticles.isValid()){
    for(reco::GenParticleCollection::const_iterator g = genParticles->begin(); g != genParticles->end() && nGenPart < maxGen; ++g){
      if(g->status()==3){
        bool final3genParticle = true;
        for(unsigned int i=0 ; i < g->numberOfDaughters(); ++i){
          if((g->daughter(i))->status() == 3){
            final3genParticle = false;
            break;	    
          }
        }
        if(final3genParticle){
          new((*vGenPart)[nGenPart]) TLorentzVector(g->px(),g->py(),g->pz(),g->energy());
          idGenPart[nGenPart] = g->pdgId();
	  nGenPart++;
	}
      }
    }
  }
 
 
 /****************
  * Get the type *
  ****************/
  edm::Handle<int> type;
  iEvent.getByLabel("PFCandidatesNoV","VType", type);
  vType = *type;


 /*********************************
  * Leptons from the vector boson *
  *********************************/
  edm::Handle<reco::PFCandidateCollection> pfLeptons;
  iEvent.getByLabel(pfLeptonsInputTag, pfLeptons);

  nLeptons = 0;
  vLeptons->Clear();
  for(reco::PFCandidateCollection::const_iterator lepton = pfLeptons->begin(); lepton != pfLeptons->end(); ++lepton, ++nLeptons){
    new((*vLeptons)[nLeptons]) TLorentzVector(lepton->px(), lepton->py(), lepton->pz(), lepton->energy());
    leptonCharge[nLeptons] = lepton->charge();
  }


 /**********
  * PF MET *
  **********/
  vMETCorrNoV->Clear();
  edm::Handle<reco::PFMETCollection> METCollectionCorrNoV;
  iEvent.getByLabel(metCorrNoVInputTag, METCollectionCorrNoV);
  const reco::PFMET *pfMetCorrNoV = &(METCollectionCorrNoV->front());
  new((*vMETCorrNoV)[0]) TLorentzVector(pfMetCorrNoV->px(), pfMetCorrNoV->py(), pfMetCorrNoV->pz(), pfMetCorrNoV->energy());
  metCorrNoV = pfMetCorrNoV->et();
  metPhiCorrNoV = pfMetCorrNoV->phi();
  metSigCorrNoV = pfMetCorrNoV->et()/pfMetCorrNoV->sumEt();

  vMETCorr->Clear();
  edm::Handle<reco::PFMETCollection> METCollectionCorr;
  iEvent.getByLabel(metCorrInputTag, METCollectionCorr);
  const reco::PFMET *pfMetCorr = &(METCollectionCorr->front());
  new((*vMETCorr)[0]) TLorentzVector(pfMetCorr->px(), pfMetCorr->py(), pfMetCorr->pz(), pfMetCorr->energy());
  metCorr = pfMetCorr->et();
  metPhiCorr = pfMetCorr->phi();
  metSigCorr = pfMetCorr->et()/pfMetCorr->sumEt();

  vMET->Clear();
  edm::Handle<reco::PFMETCollection> METCollection;
  iEvent.getByLabel(metInputTag, METCollection);
  const reco::PFMET *pfMet = &(METCollection->front());
  new((*vMET)[0]) TLorentzVector(pfMet->px(), pfMet->py(), pfMet->pz(), pfMet->energy());
  met = pfMet->et();
  metPhi = pfMet->phi();
  metSig = pfMet->et()/pfMet->sumEt();



 /*************************************************************************************
  * anti-kt 0.5 PFJets (no V), including corrections, genJets, QG tagging, PU ID, ... *
  *************************************************************************************/
  edm::ESHandle<JetCorrectorParametersCollection> JetCorrectionsCollection;
  iSetup.get<JetCorrectionsRecord>().get("AK5PF",JetCorrectionsCollection); 
  JetCorrectorParameters const & JetCorrections = (*JetCorrectionsCollection)["Uncertainty"];
  JetCorrectionUncertainty *jecUnc = new JetCorrectionUncertainty(JetCorrections);
  
  edm::Handle<reco::PFJetCollection> pfJets;
  iEvent.getByLabel(pfJetsNoVInputTag, pfJets);

  edm::Handle<reco::GenJetCollection> genJets;
  iEvent.getByLabel(genJetsInputTag, genJets);

  std::map<TString, edm::Handle<edm::ValueMap<float>>> QGTaggerHandle;
  for(TString product : {"qg","axis1","axis2","mult","ptD"}) 		iEvent.getByLabel("QGTagger",(product+"MLP").Data(), QGTaggerHandle[product+"MLP"]);
  for(TString product : {"qg","axis2","mult","ptD"}) 			iEvent.getByLabel("QGTagger",(product+"Likelihood").Data(), QGTaggerHandle[product+"Likelihood"]);
  for(TString product : {"qg","axis1","axis2","mult","R","pull"}) 	iEvent.getByLabel("QGTaggerHIG13011",(product+"HIG13011").Data(), QGTaggerHandle[product+"HIG13011"]);

  edm::Handle<edm::ValueMap<float>> puJetIdMVA;
  iEvent.getByLabel("jetPUMVA","fullDiscriminant", puJetIdMVA);

  edm::Handle<edm::ValueMap<int>> puJetIdFlag;
  iEvent.getByLabel("jetPUMVA","fullId", puJetIdFlag);

  nJets = 0;
  vJets->Clear();
  for(TString product : {"qg","axis1","axis2","mult","ptD"}) 		jetQGvariables[product + "MLP"].clear();
  for(TString product : {"qg","axis2","mult","ptD"}) 			jetQGvariables[product + "Likelihood"].clear();
  for(TString product : {"qg","axis1","axis2","mult","R","pull"}) 	jetQGvariables[product + "HIG13011"].clear();

  for(auto jet = pfJets->begin();  jet != pfJets->end() && nJets < maxJet; ++jet){
    if(!jetId(&(*jet))) continue;
    edm::RefToBase<reco::Jet> jetRef(edm::Ref<reco::PFJetCollection>(pfJets, jet - pfJets->begin()));
    if(!(puJetIdFlag.isValid() && PileupJetIdentifier::passJetId((*puJetIdFlag)[jetRef] , PileupJetIdentifier::kLoose))) continue;

    jecUnc->setJetEta(jet->eta());
    jecUnc->setJetPt(jet->pt());
    try { jetUncertainty[nJets] = jecUnc->getUncertainty(true); } 
    catch (...) { continue;}

    if(puJetIdMVA.isValid())  	jetPUIdMVA[nJets]	= (*puJetIdMVA)[jetRef];
    else 			jetPUIdMVA[nJets]	= -999;

    new((*vJets)[nJets]) TLorentzVector(jet->px(), jet->py(), jet->pz(), jet->energy());
    new((*vPull)[nJets]) TVector2(pull(jet, vtxs, 1));
    new((*vPull2)[nJets]) TVector2(pull(jet, vtxs, 2));

    std::vector<reco::PFCandidatePtr> jetParts = jet->getPFConstituents();
    ncJets[nJets] = jetParts.size();

    for(auto it = QGTaggerHandle.begin(); it != QGTaggerHandle.end(); ++it){
      if((it->second).isValid()) 	jetQGvariables[it->first].push_back((*(it->second))[jetRef]);
      else 				jetQGvariables[it->first].push_back(-996);
    }

    genJetPt[nJets] = -1;
    if(jet->pt()>5 && genJets.isValid()){
      float drmin = 0.3;
      for(reco::GenJetCollection::const_iterator genJet = genJets->begin(); genJet != genJets->end(); ++genJet){
        float dr = deltaR(*jet, *genJet);
	if(dr < drmin){
	  drmin = dr;
	  genJetPt[nJets] = genJet->pt();
	}
      }
    }

    float smearPt[5] = {1.052, 1.057, 1.096, 1.134, 1.288};  
    float smearEtaBins[6] = {0.0, 0.5, 1.1, 1.7, 2.3, 5.0};
    jetSmearedPt[nJets] = jet->pt();
    if(genJetPt[nJets] > 0){
      float eta = fabs(jet->eta());
      for(unsigned int k=0; k < 5; ++k){
	if(eta > smearEtaBins[k] && eta < smearEtaBins[k+1]){
	  jetSmearedPt[nJets] = std::max(0., genJetPt[nJets] + smearPt[k]*(jet->pt() - genJetPt[nJets]));
	}  
      }
    }

    ++nJets; 
  }

 /*******************
  * soft track-jets *
  *******************/
  edm::Handle<reco::TrackJetCollection> trackJets;
  iEvent.getByLabel(softTrackJetsInputTag, trackJets);

  nSoftTrackJets = 0;
  totalSoftHT = 0;
  vSoftTrackJets->Clear();
  if(trackJets.isValid()){
    for(reco::TrackJetCollection::const_iterator trackJet = trackJets->begin(); trackJet != trackJets->end(); ++trackJet, ++nSoftTrackJets){
      totalSoftHT += trackJet->pt();
      if(nSoftTrackJets < maxSTJ) new((*vSoftTrackJets)[nSoftTrackJets]) TLorentzVector(trackJet->px(), trackJet->py(), trackJet->pz(), trackJet->energy());
    }
  } 


 /***********
  * Trigger *
  ***********/ 
  edm::Handle<edm::TriggerResults> trigResults;
  edm::InputTag trigResultsTag("TriggerResults","",HLT_process);
  iEvent.getByLabel(trigResultsTag, trigResults);
  
  if(trigResults.isValid()){
    const edm::TriggerNames& trigNames = iEvent.triggerNames(*trigResults);
    std::vector<std::string> names =trigNames.triggerNames(); 
    std::vector<uint> triggerindexes;
    for(uint i=0; i < HLT_paths.size(); i++){
      std::string pathName= HLT_paths[i];
      std::string realPathName="";
      std::vector<std::pair<std::string,uint> > tmpNames;

      int found=0;
      for(uint k=0; k< names.size(); k++){
	bool notRecordedAlready=true;
	for(uint j=0; j<triggerindexes.size(); j++){
	  if(triggerindexes[j]==k) notRecordedAlready=false;
	}

	if (names[k].find(pathName) != std::string::npos && notRecordedAlready) {
	  found=fabs((names[k].length() - HLT_paths[i].length()));
	  tmpNames.push_back(make_pair(names[k],found));
	}
      }

      uint min=10000;
      uint index=666;

      for (uint k=0; k<tmpNames.size(); k++){
	if (tmpNames[k].second < min) {
	  min=tmpNames[k].second;
	  index=k;
	}
      }

      if (index!=666){
	realPathName=tmpNames[index].first;
	triggerindexes.push_back(trigNames.triggerIndex(realPathName));
      }
      //if (tmpNames.size()>0) cout << "Real name " << realPathName << "  in cfg " <<  HLT_paths[i] << endl;

      if (trigNames.triggerIndex(realPathName)!=trigNames.size()){
	trigRes[i]=(trigResults.product())->accept(trigNames.triggerIndex(realPathName));
	if (prescalersOK) trigPres[i]= hltConfig_.prescaleValue(iEvent, iSetup,realPathName);
        else trigPres[i]=666;
      }
      else {
	//	cout << HLT_paths[i] << " doesn't exist in TrigNames for current data" << endl; 
	trigRes[i]=0;
	trigPres[i]=-1;
      }	
    }
  }


 /*****************
  * Fill the tree *
  *****************/
  t_Analyzer->Fill();
}


void Analyzer::beginJob(){
  vGenPart = new TClonesArray("TLorentzVector", maxGen);
  vLeptons = new TClonesArray("TLorentzVector", 2);
  vMET = new TClonesArray("TLorentzVector", 1);
  vMETCorr = new TClonesArray("TLorentzVector", 1);
  vMETCorrNoV = new TClonesArray("TLorentzVector", 1);
  vJets = new TClonesArray("TLorentzVector", maxJet);
  vPull = new TClonesArray("TVector2", maxJet);
  vPull2 = new TClonesArray("TVector2", maxJet);
  vSoftTrackJets = new TClonesArray("TLorentzVector", maxSTJ);

  f_Analyzer = new TFile(fileName, "RECREATE");
  t_Analyzer = new TTree("EWKV","EWK V+2 jets tree");

  t_Analyzer->Branch("run",			&nRun,			"run/I");
  t_Analyzer->Branch("event",			&nEvent,		"event/I");
  t_Analyzer->Branch("lumi",			&nLumi,			"lumi/I");

  t_Analyzer->Branch("nParticleEntries",	&nParticleEntries,	"nParticleEntries/I");

  t_Analyzer->Branch("nPileUp",			&nPileUp,		"nPileUp/I");
  t_Analyzer->Branch("nTrue",			&nTrue,			"nTrue/F");
  t_Analyzer->Branch("rhokt6PFJets",		&rhokt6PFJets,		"rhokt6PFJets/F");
  t_Analyzer->Branch("nPriVtxs",		&nPriVtxs,		"nPriVtxs/I");

  t_Analyzer->Branch("nGenPart",		&nGenPart,		"nGenPart/I");
  t_Analyzer->Branch("vGenPart","TClonesArray", &vGenPart, 		32000, 0);
  t_Analyzer->Branch("idGenPart", 		idGenPart, 		"idGenPart[nGenPart]/I");

  t_Analyzer->Branch("VType",			&vType,			"VType/I");
  t_Analyzer->Branch("nLeptons",		&nLeptons,		"nLeptons/I");
  t_Analyzer->Branch("vLeptons","TClonesArray", &vLeptons, 		32000, 0);
  t_Analyzer->Branch("leptonCharge", 		leptonCharge, 		"leptonCharge[nLeptons]/I");

  t_Analyzer->Branch("vMET","TClonesArray", 	&vMET, 			32000, 0);
  t_Analyzer->Branch("met",			&met ,			"met/F");
  t_Analyzer->Branch("metPhi",			&metPhi ,		"metPhi/F");
  t_Analyzer->Branch("metSig",			&metSig ,		"metSig/F");

  t_Analyzer->Branch("vMETCorr","TClonesArray",	&vMETCorr, 		32000, 0);
  t_Analyzer->Branch("metCorr",			&metCorr ,		"metCorr/F");
  t_Analyzer->Branch("metPhiCorr",		&metPhiCorr ,		"metPhiCorr/F");
  t_Analyzer->Branch("metSigCorr",		&metSigCorr ,		"metSigCorr/F");

  t_Analyzer->Branch("vMETCorrNoV","TClonesArray", &vMETCorrNoV, 	32000, 0);
  t_Analyzer->Branch("metCorrNoV",		&metCorrNoV ,		"metCorrNoV/F");
  t_Analyzer->Branch("metPhiCorrNoV",		&metPhiCorrNoV ,	"metPhiCorrNoV/F");
  t_Analyzer->Branch("metSigCorrNoV",		&metSigCorrNoV ,	"metSigCorrNoV/F");
 
  t_Analyzer->Branch("nJets",			&nJets,			"nJets/I");
  t_Analyzer->Branch("vJets","TClonesArray", 	&vJets, 		32000, 0);
  t_Analyzer->Branch("vPull","TClonesArray", 	&vPull, 		32000, 0);
  t_Analyzer->Branch("vPull2","TClonesArray", 	&vPull2, 		32000, 0);
  t_Analyzer->Branch("jetUncertainty",		jetUncertainty, 	"jetUncertainty[nJets]/D");
  t_Analyzer->Branch("jetPUIdMVA",		jetPUIdMVA,		"jetPUIdMVA[nJets]/F");
  t_Analyzer->Branch("genJetPt",		genJetPt, 		"jenGenPt[nJets]/F");
  t_Analyzer->Branch("jetSmearedPt",		jetSmearedPt, 		"jetSmearedPt[nJets]/F");
  t_Analyzer->Branch("ncJets", 			ncJets, 		"ncJets[nJets]/I");

  for(TString product : {"qg","axis1","axis2","mult","ptD"}) 		t_Analyzer->Branch(product + "MLP", &jetQGvariables[product + "MLP"]);
  for(TString product : {"qg","axis2","mult","ptD"}) 			t_Analyzer->Branch(product + "Likelihood", &jetQGvariables[product + "Likelihood"]);
  for(TString product : {"qg","axis1","axis2","mult","R","pull"}) 	t_Analyzer->Branch(product + "HIG13011", &jetQGvariables[product + "HIG13011"]);

  t_Analyzer->Branch("nSoftTrackJets", 		&nSoftTrackJets,	"nSoftTrackJets/I");
  t_Analyzer->Branch("vSoftTrackJets","TClonesArray", &vSoftTrackJets, 	32000, 0);
  t_Analyzer->Branch("totalSoftHT",		&totalSoftHT,		"totalSoftHT/F");

  for (uint i=0; i< HLT_paths.size(); ++i){
    t_Analyzer->Branch(HLT_paths[i].c_str(),  &trigRes[i],"trigRes/O");
    std::string bname=HLT_paths[i]+"_prescales";
    t_Analyzer->Branch(bname.c_str(),  &trigPres[i],"trigPres/D");
  }

}


void Analyzer::endJob(){
  delete vGenPart;
  delete vLeptons;
  delete vMET;
  delete vMETCorr;
  delete vMETCorrNoV;
  delete vJets;
  delete vSoftTrackJets;

  f_Analyzer->WriteTObject(t_Analyzer);
  delete t_Analyzer;

  f_Analyzer->Close();
  delete f_Analyzer;
}


void Analyzer::beginRun(edm::Run const& iRun, edm::EventSetup const& iSetup){
  prescalersOK=false;
  bool changed=true;
  if (hltConfig_.init(iRun,iSetup,HLT_process,changed)) prescalersOK=true;
}



bool Analyzer::jetId(const reco::PFJet *jet){
  //jetID taken from https://twiki.cern.ch/twiki/bin/view/CMS/JetID#Recommendations_for_7_TeV_data_a (no twiki for 8TeV available)
  double jetEnergyUncorrected = jet->chargedHadronEnergy() + jet->neutralHadronEnergy() + jet->photonEnergy() + 
				jet->electronEnergy() + jet->muonEnergy() + jet->HFHadronEnergy() + jet->HFEMEnergy();
  double neutralHadronEnergyFraction = (jet->neutralHadronEnergy() + jet->HFHadronEnergy())/jetEnergyUncorrected;
  double neutralEmEnergyFraction = (jet->neutralEmEnergy())/jetEnergyUncorrected;
  double chargedHadronEnergyFraction = (jet->chargedHadronEnergy())/jetEnergyUncorrected;
  double chargedEmEnergyFraction = (jet->chargedEmEnergy())/jetEnergyUncorrected;

  if(! (neutralHadronEnergyFraction < .99) ) 					return false;
  if(! (neutralEmEnergyFraction < .99) ) 					return false;
  if(! ((jet->chargedMultiplicity() + jet->neutralMultiplicity()) > 1) ) 	return false;
  if(fabs(jet->eta()) < 2.4){
    if(! (chargedHadronEnergyFraction > 0) ) 					return false;
    if(! (chargedEmEnergyFraction < .99) ) 					return false;
    if(! (jet->chargedMultiplicity() > 0) ) 					return false;
  }
  return true;
}


TVector2 Analyzer::pull(reco::PFJetCollection::const_iterator jet, edm::Handle<reco::VertexCollection> vC, double power){
  reco::VertexCollection::const_iterator vtxLead = vC->begin();

  float sum_weight(0.);
  std::vector<float> weights; 
  TVector2 jetCenter = TVector2(jet->eta(), jet->phi());
  TVector2 jetCenterNoPU = TVector2(0, 0);
  std::vector<TVector2> rVectors; 

  // Loop over the jet constituents
  std::vector<reco::PFCandidatePtr> constituents = jet->getPFConstituents();
  for(unsigned i = 0; i < constituents.size(); ++i){
    reco::PFCandidatePtr part = jet->getPFConstituent(i);      
    if(!part.isNonnull()) continue;

    // Filter pu tracks
    reco::TrackRef itrk = part->trackRef();
    if(itrk.isNonnull()){
      reco::VertexCollection::const_iterator vtxClose = vC->begin();
      for(reco::VertexCollection::const_iterator vtx = vC->begin(); vtx != vC->end(); ++vtx){
        if(fabs(itrk->dz(vtx->position())) < fabs(itrk->dz(vtxClose->position()))) vtxClose = vtx;
      }
      if(vtxClose != vtxLead) continue;

      float dz = itrk->dz(vtxClose->position());
      float dz_sigma = sqrt(pow(itrk->dzError(),2) + pow(vtxClose->zError(),2));
      if(!(itrk->quality(reco::TrackBase::qualityByName("highPurity")) && fabs(dz/dz_sigma) < 5.)) continue;
    }

    float weight = pow(part->pt(), power);
    TVector2 r = TVector2(part->eta(), part->phi()) - jetCenter;
    r.Set(r.X(), 2*atan(tan(r.Y()/2)));

    jetCenterNoPU += r*weight;
    sum_weight += weight;

    weights.push_back(weight);				//Store for next loop
    rVectors.push_back(r);
  }

  if(sum_weight <= 0) return TVector2(0,0);
  jetCenterNoPU /= sum_weight;
      
  TVector2 pull = TVector2(0, 0);
  for(unsigned int i=0; i < rVectors.size(); ++i){
    TVector2 ri = rVectors[i] - jetCenterNoPU; 
    ri.Set(ri.X(), 2*atan(tan(ri.Y()/2)));
    pull += ri*sqrt(ri*ri)*weights[i];
  } 
  pull /= sum_weight;

  return pull;
}



void Analyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions){
  edm::ParameterSetDescription desc;
  desc.addUntracked<std::string>("fileName","ewkv.root.root");
  desc.add<std::vector<std::string> >("HLT_paths");
  desc.add<std::string>("HLT_process");
  desc.add<edm::InputTag>("genJetsInputTag");
  desc.add<edm::InputTag>("pfJetsNoVJetsInputTag");
  desc.add<edm::InputTag>("pfLeptonsInputTag");
  desc.add<edm::InputTag>("metInputTag");
  desc.add<edm::InputTag>("metCorrInputTag");
  desc.add<edm::InputTag>("metCorrNoVInputTag");
  desc.add<edm::InputTag>("softTrackJetsInputTag");
  desc.add<edm::InputTag>("rhoInputTag");
  desc.add<edm::InputTag>("primaryVertexInputTag");
  descriptions.add("Analyzer",desc);
}


DEFINE_FWK_MODULE(Analyzer);
