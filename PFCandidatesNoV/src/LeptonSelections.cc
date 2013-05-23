/* LeptonSelections.cc
 * Package:	EWKV/PFCandidatesNoV
 * Author:	Tom Cornelis
 * Update:	2013/03/18
 * Based on:	http://cmssw.cvs.cern.ch/cgi-bin/cmssw.cgi/UserCode/PaoloA/VBFZ/PFCandidatesNoV/src/PFCandidatesNoV.cc?view=markup
 *
 * Muon and electron selection functions
 * The MuonID recommendations are listed at https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideMuonId#Tight_Muon
 * The ElectronID recommendations are listed at https://twiki.cern.ch/twiki/bin/view/CMS/EgammaCutBasedIdentification
 * The ElectronID includes isolation (effective area used as seen in http://cmssw.cvs.cern.ch/cgi-bin/cmssw.cgi/UserCode/EGamma/EGammaAnalysisTools/src/EGammaCutBasedEleId.cc?view=markup)
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
#include "EGamma/EGammaAnalysisTools/interface/PFIsolationEstimator.h"

#include "../interface/PFCandidatesNoV.h"

bool PFCandidatesNoV::muonSelection(const reco::MuonRef mu, edm::Handle<reco::VertexCollection> vtxs){
  count("muons");
  if(mu->pt() < muon_pt_min)	  			return false;
  if(fabs(mu->eta()) > muon_eta_max) 			return false;
  if(!muon::isTightMuon(*(mu.get()), *(vtxs->begin())))	return false;

  // Isolation cut (used in 7TeV analysis)
  double isoTrack = mu->isolationR03().sumPt/mu->pt();
  if(isoTrack > 0.1) 			return false;/*
  //ALTERNATIVE: PFisolation with deltaBeta corrections used in almost all other 2012 Vjets analyses
  double chargedHadronPt = mu->isolationR04().sumChargedHadronPt;
  double neutralHadronPt = mu->isolationR04().sumNeutralHadronPt;
  double photonPt 	 = mu->isolationR04().sumPhotonPt;
  double PUPt 	 	 = mu->isolationR04().sumPUPt;
  double iso = (chargedHadronPt + max(0., neutralHadronPt + photonPt - 0.5*sumPUPt))/mu->pt();
  if(iso > 0.2)				return false;  //or 0.12 (tight) https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideMuonId#Muon_Isolation_AN1
*/
  count("mouns selected");
  return true;
}


bool PFCandidatesNoV::electronSelection(const reco::GsfElectronRef e, edm::Handle<reco::PFCandidateCollection> pfCandidates, edm::Handle<reco::VertexCollection> vtxs, 
                                        edm::Handle<reco::ConversionCollection> conversions, edm::Handle<reco::BeamSpot> beamspot, edm::Handle<double> rhoIso){
  count("electrons");
  if(e->pt() < electron_pt_min) 			return false;
  if(fabs(e->eta()) > electron_eta_max) 		return false;
  if(fabs(e->eta()) < 1.566 && fabs(e->eta()) > 1.4442)	return false;

  isolator.fGetIsolation(e.get(), &(*pfCandidates), VertexRef(vtxs, 0), vtxs);
  double iso_ch = isolator.getIsolationCharged();
  double iso_em = isolator.getIsolationPhoton();
  double iso_nh = isolator.getIsolationNeutral();

  if(!EgammaCutBasedEleId::PassWP( EgammaCutBasedEleId::LOOSE, e, conversions, *(beamspot.product()), vtxs, iso_ch, iso_em, iso_nh, *(rhoIso.product()))) return false;
  count("electrons selected");
  return true;
//return EgammaCutBasedEleId::PassWP( EgammaCutBasedEleId::MEDIUM, e, conversions, *(beamspot.product()), vtxs, iso_ch, iso_em, iso_nh, *(rhoIso.product()));
}


bool PFCandidatesNoV::muonSelectionVeto(const reco::MuonRef mu){
  if(mu->pt() < veto_muon_pt_min)	  	return false;
  if(fabs(mu->eta()) > muon_eta_max) 		return false;
  if(!mu->isPFMuon())				return false;
  if(!mu->isGlobalMuon())			return false;
  if(!mu->isTrackerMuon())			return false;

  // Isolation cut (used in 7TeV analysis)
  double isoTrack = mu->isolationR03().sumPt/mu->pt();
  if(isoTrack > 0.1) 			return false;/*
  //ALTERNATIVE: PFisolation with deltaBeta corrections used in almost all other 2012 Vjets analyses
  double chargedHadronPt = mu->isolationR04().sumChargedHadronPt;
  double neutralHadronPt = mu->isolationR04().sumNeutralHadronPt;
  double photonPt 	 = mu->isolationR04().sumPhotonPt;
  double PUPt 	 	 = mu->isolationR04().sumPUPt;
  double iso = (chargedHadronPt + max(0., neutralHadronPt + photonPt - 0.5*sumPUPt))/mu->pt();
  if(iso > 0.2)				return false;  //or 0.12 (tight) https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideMuonId#Muon_Isolation_AN1
*/
  return true;
}


bool PFCandidatesNoV::electronSelectionVeto(const reco::GsfElectronRef e, edm::Handle<reco::PFCandidateCollection> pfCandidates, edm::Handle<reco::VertexCollection> vtxs, 
                                            edm::Handle<reco::ConversionCollection> conversions, edm::Handle<reco::BeamSpot> beamspot, edm::Handle<double> rhoIso){
  if(e->pt() < veto_electron_pt_min) 			return false;
  if(fabs(e->eta()) > electron_eta_max) 		return false;
  if(fabs(e->eta()) < 1.566 && fabs(e->eta()) > 1.4442)	return false;

  isolator.fGetIsolation(e.get(), &(*pfCandidates), VertexRef(vtxs, 0), vtxs);
  double iso_ch = isolator.getIsolationCharged();
  double iso_em = isolator.getIsolationPhoton();
  double iso_nh = isolator.getIsolationNeutral();

  return EgammaCutBasedEleId::PassWP( EgammaCutBasedEleId::VETO, e, conversions, *(beamspot.product()), vtxs, iso_ch, iso_em, iso_nh, *(rhoIso.product()));
}
