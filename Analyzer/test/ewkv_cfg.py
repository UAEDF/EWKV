import FWCore.ParameterSet.Config as cms

MC = True

process = cms.Process("EWKV")

process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 100

process.load('Configuration.StandardSequences.Services_cff')
process.load('Configuration/StandardSequences/GeometryExtended_cff')
process.load('Configuration.StandardSequences.MagneticField_38T_cff')
process.load('Configuration/StandardSequences/Reconstruction_cff')
process.load('Configuration/StandardSequences/FrontierConditions_GlobalTag_cff')
process.load('Configuration/StandardSequences/Generator_cff')
process.load('GeneratorInterface.GenFilters.TotalKinematicsFilter_cfi')

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(200))

process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
      '/store/mc/Summer12_DR53X/DYJJ01JetsToLL_M-50_MJJ-200_TuneZ2Star_8TeV-madgraph_tauola/AODSIM/PU_S10_START53_V7A-v1/00000/FE987AF3-1E2A-E211-997F-008CFA002490.root'
    )
)

process.load("SimGeneral.HepPDTESSource.pythiapdt_cfi")
process.printTree = cms.EDAnalyzer("ParticleListDrawer",
  maxEventsToPrint = cms.untracked.int32(-1),
  printVertex = cms.untracked.bool(False),
  src = cms.InputTag("genParticles")
)

process.load('JetMETCorrections.Configuration.DefaultJEC_cff')

process.load('RecoJets.Configuration.RecoPFJets_cff')
process.kt6PFJets.doRhoFastjet = True
process.ak5PFJets.doAreaFastjet = True


if MC: process.GlobalTag.globaltag = 'START53_V19::All'  	# for MC
else : process.GlobalTag.globaltag = 'GR_R_53_V21::All'  	# for DATA

# PFCandidatesNoV filter/producer 
process.load('EWKV.PFCandidatesNoV.PFCandidatesNoV_cff')  

# jet producer
process.ak5PFJetsNoV = process.ak5PFJets.clone(
    src = cms.InputTag("PFCandidatesNoLL","pfCandidatesNoV")
)


# jet corrections
if MC: jetcorrection = 'ak5PFL1FastL2L3'			# for MC
else : jetcorrection = 'ak5PFL1FastL2L3Residual'	 	# for DATA
process.ak5PFJetsL1FastL2L3NoV   = cms.EDProducer('PFJetCorrectionProducer',
    src         = cms.InputTag('ak5PFJetsNoV'), 
    correctors  = cms.vstring(jetcorrection),
)

# gluon tag producer
process.load('QuarkGluonTagger.EightTeV.QGTagger_RecoJets_cff')  
process.QGTagger.srcJets = cms.InputTag('ak5PFJetsL1FastL2L3NoV')


# extra tracks producer (out of 2-lepton + 2-jets system)
process.extraTracks = cms.EDProducer('ExtraTracks')

#
process.ak5SoftTrackJets = process.ak5TrackJets.clone() 
#process.ak5SoftTrackJets.src  = cms.InputTag("outTracks")
process.ak5SoftTrackJets.src  = ('extraTracks')
process.ak5SoftTrackJets.jetPtMin = cms.double(1.0)

# our analyzer
process.lljets = cms.EDAnalyzer('LLJets',
	fileName 	= cms.untracked.string('lljets.root'),
        HLT_paths 	= cms.vstring("HLT_DoubleMu6","HLT_DoubleMu7","HLT_DoubleMu8","HLT_Mu13_Mu8","HLT_Mu17_Mu8","HLT_Mu17_TkMu8",
                                      "HLT_Ele17_CaloIdL_CaloIsoVL_Ele8_CaloIdL_CaloIsoVL",
                                      "HLT_Ele17_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL_Ele8_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL"),
        HLT_process 	= cms.string("HLT")
)

#process.p = cms.Path(process.kt6PFJets * process.PFCandidatesNoLL * process.ak5PFJetsNoLL * process.ak5PFJetsL1FastL2L3NoLL * process.QuarkGluonTagger * process.extraTracks *process.ak5SoftTrackJets * process.lljets)
process.p = cms.Path(process.kt6PFJets * process.PFCandidatesNoV)

