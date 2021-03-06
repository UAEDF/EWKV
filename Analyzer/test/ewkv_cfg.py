MC = True

import FWCore.ParameterSet.Config as cms
process = cms.Process("EWKV")

# CMSSW services, configurations, ...
process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 100

process.load('Configuration.StandardSequences.Services_cff')
process.load('Configuration.StandardSequences.GeometryExtended_cff')
process.load('Configuration.StandardSequences.MagneticField_38T_cff')
process.load('Configuration.StandardSequences.Reconstruction_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
if MC: process.GlobalTag.globaltag = 'START53_V27::All'  	# for MC
else : process.GlobalTag.globaltag = 'FT53_V21A_AN6::All'  	# for DATA
process.load('Configuration.StandardSequences.Generator_cff')
process.load('GeneratorInterface.GenFilters.TotalKinematicsFilter_cfi')

# Signal and number of events for test runs
process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(500))
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring('/store/mc/Summer12_DR53X/DYJJ01JetsToLL_M-50_MJJ-200_TuneZ2Star_8TeV-madgraph_tauola/AODSIM/PU_S10_START53_V7A-v1/00000/FE987AF3-1E2A-E211-997F-008CFA002490.root')
)

# good offline primary vertices
from PhysicsTools.SelectorUtils.pvSelector_cfi import pvSelector
process.goodOfflinePrimaryVertices = cms.EDFilter("PrimaryVertexObjectFilter",
    filterParams = pvSelector.clone( minNdof = cms.double(4.0), maxZ = cms.double(24.0) ),
    src          = cms.InputTag('offlinePrimaryVertices')
)


# GEN Particles 
process.load("SimGeneral.HepPDTESSource.pythiapdt_cfi")
process.printTree = cms.EDAnalyzer("ParticleListDrawer",
  maxEventsToPrint = cms.untracked.int32(-1),
  printVertex = cms.untracked.bool(False),
  src = cms.InputTag("genParticles")
)

# PFCandidatesNoV filter/producer 
process.load('EWKV.PFCandidatesNoV.PFCandidatesNoV_cff')  

# jet producer and corrections
process.load('RecoJets.Configuration.RecoPFJets_cff')
process.kt6PFJets.doRhoFastjet = True
process.ak5PFJets.doAreaFastjet = True
process.ak5PFJetsNoV = process.ak5PFJets.clone(
    src = cms.InputTag('PFCandidatesNoV','pfCandidatesNoV')
)

process.load('JetMETCorrections.Configuration.DefaultJEC_cff')
if MC: jetcorrection = 'ak5PFL1FastL2L3'			# for MC
else : jetcorrection = 'ak5PFL1FastL2L3Residual'	 	# for DATA

process.ak5PFJetsL1FastL2L3 = cms.EDProducer('PFJetCorrectionProducer',
    src         = cms.InputTag('ak5PFJets'), 
    correctors  = cms.vstring(jetcorrection)
)
process.ak5PFJetsL1FastL2L3NoV = cms.EDProducer('PFJetCorrectionProducer',
    src         = cms.InputTag('ak5PFJetsNoV'), 
    correctors  = cms.vstring(jetcorrection)
)

#PU jet ID
from CMGTools.External.pujetidsequence_cff import puJetId, puJetMva

process.jetPUId = puJetId.clone(
   jets = cms.InputTag("ak5PFJetsL1FastL2L3NoV"),
   applyJec = cms.bool(False),
   inputIsCorrected = cms.bool(True),                
)

process.jetPUMVA = puJetMva.clone(
   jets = cms.InputTag("ak5PFJetsL1FastL2L3NoV"),
   jetids = cms.InputTag("jetPUId"),
   applyJec = cms.bool(False),
   inputIsCorrected = cms.bool(True),               
)

process.jetPUIdSequence = cms.Sequence(process.jetPUId * process.jetPUMVA)


# MET corrections (type I)
process.load("EWKV.Analyzer.pfMETCorrections_cff")
process.pfJetMETcorr.src = cms.InputTag('ak5PFJetsL1FastL2L3')
process.pfJetMETcorrNoV.src = cms.InputTag('ak5PFJetsL1FastL2L3NoV')
process.pfJetMETcorr.jetCorrLabel = cms.string(jetcorrection)
process.pfJetMETcorrNoV.jetCorrLabel = cms.string(jetcorrection)


# QuarkGluonTagger
process.load('QuarkGluonTagger.EightTeV.QGTagger_RecoJets_cff')  
process.QGTagger.srcJets = cms.InputTag('ak5PFJetsL1FastL2L3NoV')

# HIG13011-like QuarkGluonTagger (FSQ12-019 at 8Tev)
process.QGTaggerHIG13011 = cms.EDProducer('QGTaggerHIG13011',
    src             = cms.untracked.InputTag('ak5PFJetsL1FastL2L3NoV'),
    mva	            = cms.untracked.string('Likelihood'),
    xmldir          = cms.untracked.string('EWKV/QGTaggerHIG13011/data/'),
    useProbValue    = cms.untracked.bool(False),
)


# extra tracks producer (out of V + 2-jets system)
process.load('EWKV.ExtraTracks.SoftTrackJets_cff')  

# our analyzer
process.ewkv = cms.EDAnalyzer('Analyzer',
	fileName 		= cms.untracked.string('ewkv.root'),
        HLT_paths 		= cms.vstring("HLT_Mu13_Mu8","HLT_Mu17_Mu8","HLT_Mu17_TkMu8",
                                      "HLT_Ele17_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL_Ele8_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL",
 				      "HLT_Mu15","HLT_Mu15_eta2p1","HLT_IsoMu24","HLT_IsoMu24_eta2p1",
				      "HLT_Ele17_CaloIdL_CaloIsoVL","HLT_Ele22_CaloIdL_CaloIsoVL","HLT_Ele27_WP80"),
        HLT_process 		= cms.string("HLT"),
	genJetsInputTag		= cms.InputTag('ak5GenJets'),
	pfJetsNoVJetsInputTag	= cms.InputTag('ak5PFJetsL1FastL2L3NoV'),
	pfLeptonsInputTag	= cms.InputTag('PFCandidatesNoV', 'pfLeptons'),
	metInputTag		= cms.InputTag('pfMet'),
	metCorrInputTag		= cms.InputTag('pfType1CorrectedMet'),
	metCorrNoVInputTag	= cms.InputTag('pfType1CorrectedMetNoV'),
	softTrackJetsInputTag	= cms.InputTag('ak5SoftTrackJets'),
	rhoInputTag		= cms.InputTag('kt6PFJets','rho'),
        primaryVertexInputTag	= cms.InputTag('goodOfflinePrimaryVertices')
)

process.p = cms.Path(process.goodOfflinePrimaryVertices * process.kt6PFJets * 
                     process.ak5PFJets * process.ak5PFJetsL1FastL2L3 * 
                     process.producePFMETCorrections *
                     process.PFCandidatesNoV * 
		     process.ak5PFJetsNoV * process.ak5PFJetsL1FastL2L3NoV * 
                     process.producePFMETCorrectionsNoV *
		     process.jetPUIdSequence * 
		     process.QuarkGluonTagger * process.QGTaggerHIG13011 *
                     process.seqSoftTrackJets * process.ewkv)

