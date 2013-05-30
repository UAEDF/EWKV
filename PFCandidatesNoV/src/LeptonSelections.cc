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
                                        edm::Handle<reco::ConversionCollection> conversions, edm::Handle<reco::BeamSpot> beamspot, edm::Handle<double> rhoIso){
  if(e->pt() < electron_pt_min) 			return false;
  if(fabs(e->eta()) > electron_eta_max) 		return false;
  if(fabs(e->eta()) < 1.566 && fabs(e->eta()) > 1.4442)	return false;

  if(e->isEB()){
    if(e->deltaEtaSuperClusterTrackAtVtx() > 0.007) 		return false;
    if(e->deltaPhiSuperClusterTrackAtVtx() > 0.15) 		return false;
    if(e->sigmaIetaIeta() > 0.01) 				return false;
    if(e->hadronicOverEm() > 0.12) 				return false;
  } else if(e->isEE()){
    if(e->deltaEtaSuperClusterTrackAtVtx() > 0.009) 		return false;
    if(e->deltaPhiSuperClusterTrackAtVtx() > 0.10) 		return false;
    if(e->sigmaIetaIeta() > 0.03) 				return false;
    if(e->hadronicOverEm() > 0.10) 				return false;
  } else return false;

  float trackIso = e->dr03TkSumPt()/e->pt();
  if(trackIso > 0.1) return false;


  double d0vtx, dzvtx;
  if(vtxs->size() > 0){
    reco::VertexRef vtx(vtxs, 0);    
    d0vtx = e->gsfTrack()->dxy(vtx->position());
    dzvtx = e->gsfTrack()->dz(vtx->position());
  } else {
    d0vtx = e->gsfTrack()->dxy();
    dzvtx = e->gsfTrack()->dz();
  }
  if(d0vtx > 0.02)						return false;
  if(dzvtx > 0.2)						return false;

  double ooemoop = (1.0/e->ecalEnergy() - e->eSuperClusterOverP()/e->ecalEnergy());
  if(fabs(ooemoop) > 0.05)					return false;

  float mHits = e->gsfTrack()->trackerExpectedHitsInner().numberOfHits(); 
  if(mHits > 1)							return false;

  return true;
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
                                            edm::Handle<reco::ConversionCollection> conversions, edm::Handle<reco::BeamSpot> beamspot, edm::Handle<double> rhoIso){
  if(e->pt() < veto_electron_pt_min) 			return false;
  if(fabs(e->eta()) > electron_eta_max) 		return false;
  if(fabs(e->eta()) < 1.566 && fabs(e->eta()) > 1.4442)	return false;

  if(e->isEB()){
    if(e->deltaEtaSuperClusterTrackAtVtx() > 0.007) 		return false;
    if(e->deltaPhiSuperClusterTrackAtVtx() > 0.8) 		return false;
    if(e->sigmaIetaIeta() > 0.01) 				return false;
    if(e->hadronicOverEm() > 0.15) 				return false;
  } else if(e->isEE()){
    if(e->deltaEtaSuperClusterTrackAtVtx() > 0.009) 		return false;
    if(e->deltaPhiSuperClusterTrackAtVtx() > 0.7) 		return false;
    if(e->sigmaIetaIeta() > 0.03) 				return false;
  } else return false;

  float trackIso = e->dr03TkSumPt()/e->pt();
  if(trackIso > 0.1) return false;


  double d0vtx, dzvtx;
  if(vtxs->size() > 0){
    reco::VertexRef vtx(vtxs, 0);    
    d0vtx = e->gsfTrack()->dxy(vtx->position());
    dzvtx = e->gsfTrack()->dz(vtx->position());
  } else {
    d0vtx = e->gsfTrack()->dxy();
    dzvtx = e->gsfTrack()->dz();
  }
  if(d0vtx > 0.04)						return false;
  if(dzvtx > 0.2)						return false;

  return true;
}
