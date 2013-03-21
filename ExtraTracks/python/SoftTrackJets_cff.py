import FWCore.ParameterSet.Config as cms

# Produce extra tracks
extraTracks = cms.EDProducer('ExtraTracks',
   pfJetsNoVInputTag		= cms.InputTag("ak5PFJetsL1FastL2L3NoV"), 
   pfLeptonsInputTag		= cms.InputTag("PFCandidatesNoV", "pfLeptons"), 
   primaryVertexInputTag	= cms.InputTag("offlinePrimaryVertices"),
   tracksInputTag		= cms.InputTag("generalTracks")
)

# Cluster to trackjets
from RecoJets.Configuration.RecoTrackJets_cff import ak5TrackJets
ak5SoftTrackJets = ak5TrackJets.clone( 
   src  = ('extraTracks'),
   jetPtMin = cms.double(1.0)
)

seqSoftTrackJets = cms.Sequence(extraTracks + ak5SoftTrackJets)
