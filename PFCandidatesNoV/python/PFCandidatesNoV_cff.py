from RecoJets.JetProducers.kt4PFJets_cfi import *
kt6PFJetsForIsolation = kt4PFJets.clone( rParam = 0.6, doRhoFastjet = True )
kt6PFJetsForIsolation.Rho_EtaMax = cms.double(2.5)

PFCandidatesNoVConfig = cms.EDFilter("PFCandidatesNoV",
    rhoIsoInputTag          = cms.InputTag("kt6PFJetsForIsolation", "rho"),
    pfCandidatesInputTag    = cms.InputTag("particleFlow"),
    metInputTag    	    = cms.InputTag("pfMet"),
    conversionsInputTag     = cms.InputTag("allConversions"),
    beamSpotInputTag        = cms.InputTag("offlineBeamSpot"),
    primaryVertexInputTag   = cms.InputTag("offlinePrimaryVertices"),
)

PFCandidatesNoV = cms.Sequence(kt6PFJetsForIsolation + PFCandidatesNoVConfig)
