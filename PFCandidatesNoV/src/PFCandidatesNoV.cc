/* PFCandidatesNoV.cc
 * Package:	EWKV/PFCandidatesNoV
 * Author:	Tom Cornelis
 * Based on:	http://cmssw.cvs.cern.ch/cgi-bin/cmssw.cgi/UserCode/PaoloA/VBFZ/PFCandidatesNoV/src/PFCandidatesNoV.cc?view=markup
 *
 * Class to select and extract the lepton(s) from Z or W from the PFCandidates collection
 */

#include <memory>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDFilter.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h"
#include "SimDataFormats/GeneratorProducts/interface/LHEEventProduct.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/MuonReco/interface/Muon.h" 
#include "DataFormats/EgammaCandidates/interface/GsfElectron.h"
#include "DataFormats/METReco/interface/PFMETCollection.h"
#include "DataFormats/METReco/interface/PFMET.h"

#include "TH1.h"
#include "TFile.h"

#include "../interface/PFCandidatesNoV.h"


PFCandidatesNoV::PFCandidatesNoV(const edm::ParameterSet& iConfig):
  fileName(			TString(iConfig.getUntrackedParameter<std::string>("fileName","pileUp.root"))),
  dilepton_mass_min(		iConfig.getUntrackedParameter<double>("dilepton_mass_min", 50.)),
  transverse_Wmass_min(		iConfig.getUntrackedParameter<double>("transverse_Wmass_min", 50.)),
  muon_pt_min(			iConfig.getUntrackedParameter<double>("muon_pt_min", 20.)),
  veto_muon_pt_min(		iConfig.getUntrackedParameter<double>("veto_muon_pt_min", 20.)),
  muon_eta_max(			iConfig.getUntrackedParameter<double>("muon_eta_max", 2.4)),
  electron_pt_min(		iConfig.getUntrackedParameter<double>("electron_pt_min", 20.)),
  veto_electron_pt_min(		iConfig.getUntrackedParameter<double>("veto_electron_pt_min", 20.)),
  electron_eta_max(		iConfig.getUntrackedParameter<double>("electron_eta_max", 2.4)),
  pfCandidatesInputTag(   	iConfig.getParameter<edm::InputTag>("pfCandidatesInputTag")),
  metInputTag(            	iConfig.getParameter<edm::InputTag>("metInputTag")),
  conversionsInputTag(    	iConfig.getParameter<edm::InputTag>("conversionsInputTag")),
  beamSpotInputTag(       	iConfig.getParameter<edm::InputTag>("beamSpotInputTag")),
  primaryVertexInputTag(  	iConfig.getParameter<edm::InputTag>("primaryVertexInputTag"))
{
  produces<reco::PFCandidateCollection>("pfCandidatesNoV");
  produces<reco::PFCandidateCollection>("pfLeptons");
  produces<int>("VType");
}


bool PFCandidatesNoV::filter(edm::Event& iEvent, const edm::EventSetup& iSetup){
  fillPU(iEvent);
 
  type = UNDEFINED; 
  lepton1 = NULL, lepton2 = NULL;
  bool vetoLepton = false;

  //Get primary vertex
  edm::Handle<reco::VertexCollection> vtxs;
  iEvent.getByLabel(primaryVertexInputTag, vtxs);
  if(!vtxs.isValid() || (*vtxs.product()).size() == 0) return false;

  // Get PF candidates
  edm::Handle<reco::PFCandidateCollection> pfCandidates;
  iEvent.getByLabel(pfCandidatesInputTag, pfCandidates);
 
  // Get PF MET
  edm::Handle<reco::PFMETCollection> METCollection;
  iEvent.getByLabel(metInputTag, METCollection);
  const reco::PFMET *met = &(METCollection->front());

  std::vector<const reco::PFCandidate*> selectedMuons;
  std::vector<const reco::PFCandidate*> selectedElectrons;

  // Try to find a V
  for(std::vector<reco::PFCandidate>::const_iterator pfCandidate = pfCandidates->begin(); pfCandidate != pfCandidates->end(); ++pfCandidate){
    if(reco::PFCandidate::ParticleType(pfCandidate->particleId()) != reco::PFCandidate::mu) continue;
    reco::MuonRef muref = pfCandidate->muonRef();
    if(!(muref.isNonnull() && muonSelection(muref, vtxs))){
      if(muonSelectionVeto(muref)) vetoLepton = true;
      continue;
    }
    if(TryZ(&(*pfCandidate), selectedMuons)) break; 							//Vectors are pt-oredered: select firt pair which combine to Z
    if(type == UNDEFINED){
      if(!TryW(&(*pfCandidate), met)) vetoLepton = true; 						//Select first W, but keep looking for a Z
    } else vetoLepton = true;										//Second visit ==> there is a veto lepton
    selectedMuons.push_back(&(*pfCandidate));								//Remember the lepton for next iterations
  }


  if(type != ZMUMU){											//If no Z found using muons, try electrons
    //Stuff needed for the electron ID
    edm::Handle<reco::ConversionCollection> conversions; 	iEvent.getByLabel(conversionsInputTag, conversions);
    edm::Handle<reco::BeamSpot> beamspot;			iEvent.getByLabel(beamSpotInputTag, beamspot);

    // find PF electrons
    for(std::vector<reco::PFCandidate>::const_iterator pfCandidate = pfCandidates->begin(); pfCandidate != pfCandidates->end(); ++pfCandidate){
      if(reco::PFCandidate::ParticleType(pfCandidate->particleId()) != reco::PFCandidate::e) continue;
      reco::GsfElectronRef eref = pfCandidate->gsfElectronRef();
      if(!(eref.isNonnull() && electronSelection(eref, pfCandidates, vtxs, conversions, beamspot))){
        if(electronSelectionVeto(eref, pfCandidates, vtxs, conversions, beamspot)) vetoLepton = true;
        continue;
      }
      if(TryZ(&(*pfCandidate), selectedElectrons)) break;						//Vectors are pt-ordered: select first pair which combine to Z, stop looking
      if(type == UNDEFINED){
        if(!TryW(&(*pfCandidate), met)) vetoLepton = true;						//Select first W, but keep looking for a Z
      } else vetoLepton = true;										//Second visit ==> there is a veto lepton
      selectedElectrons.push_back(&(*pfCandidate));							//Remember the lepton for next iterations
    }
  }

  if(type == UNDEFINED) return false;

  // Apply lepton veto in case of W events
  if(((type == WMUNU) || (type == WENU)) && vetoLepton) return false;
  
  // Split the V leptons from the other candidates
  std::auto_ptr<reco::PFCandidateCollection> pfCandidatesNoV(new reco::PFCandidateCollection());
  std::auto_ptr<reco::PFCandidateCollection> pfLeptons(new reco::PFCandidateCollection());
  for(std::vector<reco::PFCandidate>::const_iterator pfCandidate = pfCandidates->begin(); pfCandidate != pfCandidates->end(); ++pfCandidate){
    if(&(*pfCandidate) == lepton1 || &(*pfCandidate) == lepton2) pfLeptons->push_back(*pfCandidate);
    else pfCandidatesNoV->push_back(*pfCandidate);
  }

  // Put VType and splitted pfCandidates in the event
  std::auto_ptr<int> theType(new int(type));
  iEvent.put(theType, "VType");
  iEvent.put(pfCandidatesNoV, "pfCandidatesNoV");
  iEvent.put(pfLeptons, "pfLeptons");
  return true;
}


bool PFCandidatesNoV::TryW(const reco::PFCandidate *lepton, const reco::PFMET *met){
  reco::Candidate::LorentzVector WCandidate = lepton->p4() + met->p4();
  double eT = lepton->pt() + met->pt();
  double px = WCandidate.px();
  double py = WCandidate.py();
  double massT = eT*eT - px*px - py*py;
  if(massT < transverse_Wmass_min) return false;
  lepton1 = lepton;
  if(reco::PFCandidate::ParticleType(lepton->particleId()) == reco::PFCandidate::mu) type = WMUNU;
  else type = WENU;
  return true;
}


bool PFCandidatesNoV::TryZ(const reco::PFCandidate *lepton, std::vector<const reco::PFCandidate*> selectedLeptons){
  for(std::vector<const reco::PFCandidate*>::const_iterator secondLepton = selectedLeptons.begin(); secondLepton != selectedLeptons.end(); ++secondLepton){
    if(lepton->charge() == (*secondLepton)->charge()) continue;
    reco::Candidate::LorentzVector ZCandidate = lepton->p4() + (*secondLepton)->p4();
    if(sqrt(ZCandidate.M2()) < dilepton_mass_min) continue;
    lepton1 = lepton; lepton2 = *secondLepton;
    if(reco::PFCandidate::ParticleType(lepton->particleId()) == reco::PFCandidate::mu) type = ZMUMU;
    else type = ZEE;
    return true;
  }
  return false;
}


void PFCandidatesNoV::fillPU(edm::Event& iEvent){
  nPileUp = -1;
  nTrue = -1;
  nParticleEntries = -1;

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
  }
  h_pileUp["pileUp"]->Fill(nPileUp);
  h_true["true"]->Fill(nTrue);

  edm::Handle<LHEEventProduct> lheH; 
  iEvent.getByType(lheH);
  if(lheH.isValid()){
    nParticleEntries = lheH->hepeup().NUP;
    if(nParticleEntries > 4 && nParticleEntries < 10) h_pileUp["pileUp"+TString::Format("%d", nParticleEntries)]->Fill(nPileUp);
    if(nParticleEntries > 4 && nParticleEntries < 10) h_true["true"+TString::Format("%d", nParticleEntries)]->Fill(nTrue);
  }
  t_pileUp->Fill();
}


void PFCandidatesNoV::beginJob(){
  f_pileUp = new TFile(fileName,"RECREATE");
  t_pileUp = new TTree("pileUpTree","pileUp tree");

  t_pileUp->Branch("nPileUp",		&nPileUp,		"nPileUp/I");
  t_pileUp->Branch("nTrue",		&nTrue,			"nTrue/F");
  t_pileUp->Branch("nParticleEntries",	&nParticleEntries,	"nParticleEntries/I");

  for(TString i : {"","5","6","7","8","9"}) h_pileUp["pileUp"+i] = new TH1I("pileUp"+i, "pileUp"+i, 101, -.5, 100.5);
  for(TString i : {"","5","6","7","8","9"}) h_true["true"+i] = new TH1D("true"+i, "true"+i, 10000, 0, 100);
}


void PFCandidatesNoV::endJob(){
  for(auto hist = h_pileUp.begin(); hist != h_pileUp.end(); ++hist) f_pileUp->WriteTObject(hist->second);
  for(auto hist = h_true.begin(); hist != h_true.end(); ++hist) f_pileUp->WriteTObject(hist->second);
  f_pileUp->WriteTObject(t_pileUp);
  f_pileUp->WriteTObject(t_true);
  f_pileUp->Close();

  for(auto hist = h_pileUp.begin(); hist != h_pileUp.end(); ++hist) delete hist->second;
  for(auto hist = h_true.begin(); hist != h_true.end(); ++hist) delete hist->second;
  delete t_pileUp;
  delete t_true;
  delete f_pileUp;
}


void PFCandidatesNoV::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.addUntracked<std::string>("fileName","pileUp.root");
  desc.addUntracked<double>("dilepton_mass_min", 50.);
  desc.addUntracked<double>("transverse_Wmass_min", 50.);
  desc.addUntracked<double>("muon_pt_min", 20.);
  desc.addUntracked<double>("veto_muon_pt_min", 20.);
  desc.addUntracked<double>("muon_eta_max", 2.4);
  desc.addUntracked<double>("electron_pt_min", 20.);
  desc.addUntracked<double>("veto_electron_pt_min", 20.);
  desc.addUntracked<double>("electron_eta_max", 2.4);
  desc.add<edm::InputTag>("pfCandidatesInputTag");
  desc.add<edm::InputTag>("metInputTag");
  desc.add<edm::InputTag>("conversionsInputTag");
  desc.add<edm::InputTag>("beamSpotInputTag");
  desc.add<edm::InputTag>("primaryVertexInputTag");
  desc.add<edm::InputTag>("primaryVertexInputTag");
  descriptions.add("PFCandidatesNoV", desc);
  descriptions.add("PFCandidatesNoV", desc);
}

DEFINE_FWK_MODULE(PFCandidatesNoV);
