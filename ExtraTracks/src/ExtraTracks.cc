/* ExtraTracks.cc
 * Package:	EWKV/ExtraTracks
 * Author:	Paolo Azzurri, Tom Cornelis
 * Update:	2013/03/19
 * Based on:	http://cmssw.cvs.cern.ch/cgi-bin/cmssw.cgi/UserCode/PaoloA/VBFZ/ExtraTracks/src/ExtraTracks.cc?view=markup
 *
 * Class to produce the extra track collection
 *
 * TO DO: 	- Let it work in the EWKV package
 *	  	- Clean up code
 */


#include <memory>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/RecoCandidate/interface/RecoChargedRefCandidate.h"
#include "DataFormats/RecoCandidate/interface/RecoChargedRefCandidateFwd.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/JetReco/interface/PFJetCollection.h"
#include "DataFormats/JetReco/interface/PFJet.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"

#include "../interface/ExtraTracks.h"


ExtraTracks::ExtraTracks(const edm::ParameterSet& iConfig):
  pfJetsNoVInputTag( 	  	iConfig.getParameter<edm::InputTag>("pfJetsNoVInputTag")),
  pfLeptonsInputTag(   		iConfig.getParameter<edm::InputTag>("pfLeptonsInputTag")),
  primaryVertexInputTag(  	iConfig.getParameter<edm::InputTag>("primaryVertexInputTag")),
  tracksInputTag(  		iConfig.getParameter<edm::InputTag>("tracksInputTag"))
{
  produces<reco::RecoChargedRefCandidateCollection>();
}


ExtraTracks::~ExtraTracks(){}

void ExtraTracks::produce(edm::Event& iEvent, const edm::EventSetup& iSetup){
  //TrackCollection extraTracks;
  std::auto_ptr<reco::RecoChargedRefCandidateCollection> extraTracks(new reco::RecoChargedRefCandidateCollection);
  reco::TrackRefVector V2jetsTracks;
  V2jetsTracks.clear();

  // Remove tracks from V-leptons
  edm::Handle<reco::PFCandidateCollection> pfLeptons;
  iEvent.getByLabel(pfLeptonsInputTag, pfLeptons);
  for(reco::PFCandidateCollection::const_iterator lepton = pfLeptons->begin(); lepton != pfLeptons->end(); ++lepton){
    reco::TrackRef track = lepton->trackRef();
    if(track.isNonnull()) V2jetsTracks.push_back(track);
  }

  // Remove tracks from 2 leading jets
  edm::Handle<reco::PFJetCollection> pfJets;
  iEvent.getByLabel(pfJetsNoVInputTag, pfJets);
  unsigned int iJet = 0;
  for(reco::PFJetCollection::const_iterator jet = pfJets->begin(); jet != pfJets->end() && iJet < 2; ++jet, ++iJet){
    reco::TrackRefVector pfTracks = jet->getTrackRefs();   
    for(reco::TrackRefVector::const_iterator track = pfTracks.begin(); track != pfTracks.end(); ++track) V2jetsTracks.push_back(*track);
  }

  // Get the primary vertices points
  std::vector<math::XYZPoint> PVpoints;
  edm::Handle<reco::VertexCollection> vtxs;
  iEvent.getByLabel(primaryVertexInputTag, vtxs);
  for (reco::VertexCollection::const_iterator vtx = vtxs->begin(); vtx != vtxs->end(); ++vtx) PVpoints.push_back(vtx->position());       

  // Loop over tracks
  edm::Handle<reco::TrackCollection> tracks;
  iEvent.getByLabel(tracksInputTag, tracks);
  unsigned int iTrack = 0;
  for(reco::TrackCollection::const_iterator track = tracks->begin(); track != tracks->end(); ++track, ++iTrack){
    if(!(track->quality(reco::TrackBase::highPurity) && track->pt() > 0.3)) continue;	//Select high purity and pT > 300 MeV

    // minimum z-distance of track to the first PV : 2mm && 3sigma
    float dzPV0 = track->dz(PVpoints[0]);
    float dzErr = track->dzError();
    if(fabs(dzPV0) > 0.2 || fabs(dzPV0/dzErr) > 3) continue;

    // loop over secondary (softer) primary Vertices and exclude tracks more compatible with those       
    bool otherVertex = false;
    for(std::vector<math::XYZPoint>::const_iterator PVpoint = PVpoints.begin() + 1; PVpoint != PVpoints.end(); ++PVpoint){
      float dz =  track->dz(*PVpoint);
      if(fabs(dz) < fabs(dzPV0)) otherVertex = true;
    }
    if(otherVertex) continue;

    // exclude tracks belonging to the leptons or jets
    bool exclude = false;
    for(reco::TrackRefVector::const_iterator V2jetsTrack = V2jetsTracks.begin(); V2jetsTrack != V2jetsTracks.end(); ++V2jetsTrack){
      if(&*track == V2jetsTrack->get()) exclude = true;
    }
    if(exclude) continue;      

    // assign the pi+ mass to the candidate
    reco::RecoChargedRefCandidate refCand = reco::RecoChargedRefCandidate(reco::TrackRef(tracks, iTrack), 0.139);
    extraTracks->push_back(refCand);
  }

  iEvent.put(extraTracks);
}


void ExtraTracks::fillDescriptions(edm::ConfigurationDescriptions& descriptions){
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("pfJetsNoVInputTag");
  desc.add<edm::InputTag>("pfLeptonsInputTag");
  desc.add<edm::InputTag>("primaryVertexInputTag");
  desc.add<edm::InputTag>("tracksInputTag");
  descriptions.add("ExtraTracks", desc);
}

DEFINE_FWK_MODULE(ExtraTracks);
