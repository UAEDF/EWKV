/* LeptonSelections.cc
 * Package:	EWKV/PFCandidatesNoV
 * Author:	Tom Cornelis
 * Update:	2013/03/18
 * Based on:	http://cmssw.cvs.cern.ch/cgi-bin/cmssw.cgi/UserCode/PaoloA/VBFZ/PFCandidatesNoV/src/PFCandidatesNoV.cc?view=markup
 *
 * Muon and electron selection functions
 * The MuonID recommendations are listed at https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideMuonId#Tight_Muon
 * The ElectronID recommendations are listed at https://twiki.cern.ch/twiki/bin/view/CMS/EgammaCutBasedIdentification
 */

#include <memory>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDFilter.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/MuonReco/interface/Muon.h" 
#include "DataFormats/MuonReco/interface/MuonSelectors.h"
#include "DataFormats/EgammaCandidates/interface/GsfElectron.h"
#include "EGamma/EGammaAnalysisTools/interface/EGammaCutBasedEleId.h"

#include "../interface/PFCandidatesNoV.h"

bool PFCandidatesNoV::muonSelection(const reco::MuonRef mu, edm::Handle<reco::VertexCollection> vtxs){
  if(mu->pt() < muon_pt_min)	  			return false;
  if(fabs(mu->eta()) > muon_eta_max) 			return false;
  if(!muon::isTightMuon(*(mu.get()), *(vtxs->begin())))	return false;

  double isoTrack = mu->isolationR03().sumPt/mu->pt();
  if(isoTrack > 0.1) 					return false;
  return true;
}


bool PFCandidatesNoV::electronSelection(const reco::GsfElectronRef e, edm::Handle<reco::PFCandidateCollection> pfCandidates, edm::Handle<reco::VertexCollection> vtxs, 
                                        edm::Handle<reco::ConversionCollection> conversions, edm::Handle<reco::BeamSpot> beamspot){
  if(e->pt() < electron_pt_min) 			return false;
  if(fabs(e->eta()) > electron_eta_max) 		return false;
  if(fabs(e->eta()) < 1.566 && fabs(e->eta()) > 1.4442)	return false;

  float trackIso = e->dr03TkSumPt()/e->pt();
  if(trackIso > 0.1) return false;

  return EgammaCutBasedEleId::PassWP( EgammaCutBasedEleId::LOOSE, e, conversions, *(beamspot.product()), vtxs, 0, 0, 0, 0);
}


bool PFCandidatesNoV::muonSelectionVeto(const reco::MuonRef mu){
  if(mu->pt() < veto_muon_pt_min)	  	return false;
  if(fabs(mu->eta()) > muon_eta_max) 		return false;
  if(!mu->isPFMuon())				return false;
  if(!mu->isGlobalMuon())			return false;
  if(!mu->isTrackerMuon())			return false;

  double isoTrack = mu->isolationR03().sumPt/mu->pt();
  if(isoTrack > 0.1) 				return false;
  return true;
}


bool PFCandidatesNoV::electronSelectionVeto(const reco::GsfElectronRef e, edm::Handle<reco::PFCandidateCollection> pfCandidates, edm::Handle<reco::VertexCollection> vtxs, 
                                            edm::Handle<reco::ConversionCollection> conversions, edm::Handle<reco::BeamSpot> beamspot){
  if(e->pt() < veto_electron_pt_min) 			return false;
  if(fabs(e->eta()) > electron_eta_max) 		return false;
  if(fabs(e->eta()) < 1.566 && fabs(e->eta()) > 1.4442)	return false;

  float trackIso = e->dr03TkSumPt()/e->pt();
  if(trackIso > 0.1) return false;

  return EgammaCutBasedEleId::PassWP( EgammaCutBasedEleId::LOOSE, e, conversions, *(beamspot.product()), vtxs, 0, 0, 0, 0);
}
