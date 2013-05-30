import FWCore.ParameterSet.Config as cms

PFCandidatesNoV = cms.EDFilter("PFCandidatesNoV",
    pfCandidatesInputTag    = cms.InputTag("particleFlow"),
    metInputTag    	    = cms.InputTag("pfType1CorrectedMet"),
    conversionsInputTag     = cms.InputTag("allConversions"),
    beamSpotInputTag        = cms.InputTag("offlineBeamSpot"),
    primaryVertexInputTag   = cms.InputTag("goodOfflinePrimaryVertices"),
)
