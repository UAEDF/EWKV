#!/usr/bin/python
import sys
import shutil
import os
import fileinput

samples = {
	   # Single muon
	   'SingleMuonA' : 	['data',	'/SingleMu/Run2012A-13Jul2012-v1/AOD', 			'Cert_190456-196531_8TeV_13Jul2012ReReco_Collisions12_JSON_v2.txt'],
	   'SingleMuonA2' : 	['data',	'/SingleMu/Run2012A-recover-06Aug2012-v1/AOD', 		'Cert_190782-190949_8TeV_06Aug2012ReReco_Collisions12_JSON.txt'],
	   'SingleMuonB' : 	['data',	'/SingleMu/Run2012B-13Jul2012-v1/AOD', 			'Cert_190456-196531_8TeV_13Jul2012ReReco_Collisions12_JSON_v2.txt'],
	   'SingleMuonC' : 	['data',	'/SingleMu/Run2012C-24Aug2012-v1/AOD', 			'Cert_198022-198523_8TeV_24Aug2012ReReco_Collisions12_JSON.txt'],
	   'SingleMuonC2' : 	['data',	'/SingleMu/Run2012C-PromptReco-v2/AOD',			'Cert_190456-203002_8TeV_PromptReco_Collisions12_JSON_v2.txt'],
	   'SingleMuonC3' : 	['data',	'/SingleMu/Run2012C-EcalRecover_11Dec2012-v1/AOD',	'Cert_201191-201191_8TeV_11Dec2012ReReco-recover_Collisions12_JSON.txt'],
	   'SingleMuonD' : 	['data',	'/SingleMu/Run2012D-PromptReco-v1/AOD', 		'Cert_190456-208686_8TeV_PromptReco_Collisions12_JSON_PixAlignRecovery.txt'],
	   'SingleMuonD2' : 	['data',	'', 							'Cert_207883-208307_8TeV_16Jan2013ReReco_Collisions12_JSON.txt'],

	   # Single electron
           'SingleElectronA' : 	['data',	'/SingleElectron/Run2012A-13Jul2012-v1/AOD', 		'Cert_190456-196531_8TeV_13Jul2012ReReco_Collisions12_JSON_v2.txt'],
	   'SingleElectronA2' :	['data',	'/SingleElectron/Run2012A-recover-06Aug2012-v1/AOD', 	'Cert_190782-190949_8TeV_06Aug2012ReReco_Collisions12_JSON.txt'],
	   'SingleElectronB' : 	['data',	'/SingleElectron/Run2012B-13Jul2012-v1/AOD', 		'Cert_190456-196531_8TeV_13Jul2012ReReco_Collisions12_JSON_v2.txt'],
	   'SingleElectronC' : 	['data',	'/SingleElectron/Run2012C-24Aug2012-v1/AOD', 		'Cert_198022-198523_8TeV_24Aug2012ReReco_Collisions12_JSON.txt'],
	   'SingleElectronC2' :	['data',	'/SingleElectron/Run2012C-PromptReco-v2/AOD', 		'Cert_190456-203002_8TeV_PromptReco_Collisions12_JSON_v2.txt'],
	   'SingleElectronC3' :	['data',	'/SingleElectron/Run2012C-EcalRecover_11Dec2012-v1/AOD','Cert_201191-201191_8TeV_11Dec2012ReReco-recover_Collisions12_JSON.txt'],
	   'SingleElectronD' : 	['data',	'/SingleElectron/Run2012D-PromptReco-v1/AOD', 		'Cert_190456-208686_8TeV_PromptReco_Collisions12_JSON_PixAlignRecovery.txt'],
	   'SingleElectronD2' :	['data',	'', 							'Cert_207883-208307_8TeV_16Jan2013ReReco_Collisions12_JSON.txt'],

	   # Double muon
	   'DoubleMuonA' : 	['data',	'/DoubleMu/Run2012A-13Jul2012-v1/AOD', 			'Cert_190456-196531_8TeV_13Jul2012ReReco_Collisions12_JSON_v2.txt'],
	   'DoubleMuonA2' : 	['data',	'/DoubleMu/Run2012A-recover-06Aug2012-v1/AOD', 		'Cert_190782-190949_8TeV_06Aug2012ReReco_Collisions12_JSON.txt'],
	   'DoubleMuonB' : 	['data',	'/DoubleMu/Run2012B-13Jul2012-v4/AOD', 			'Cert_190456-196531_8TeV_13Jul2012ReReco_Collisions12_JSON_v2.txt'],
	   'DoubleMuonC' : 	['data',	'/DoubleMu/Run2012C-24Aug2012-v1/AOD', 			'Cert_198022-198523_8TeV_24Aug2012ReReco_Collisions12_JSON.txt'],
	   'DoubleMuonC2' : 	['data',	'/DoubleMu/Run2012C-PromptReco-v2/AOD', 		'Cert_190456-203002_8TeV_PromptReco_Collisions12_JSON_v2.txt'],
	   'DoubleMuonC3' : 	['data',	'/DoubleMu/Run2012C-EcalRecover_11Dec2012-v1/AOD',	'Cert_201191-201191_8TeV_11Dec2012ReReco-recover_Collisions12_JSON.txt'],
	   'DoubleMuonD' : 	['data',	'/DoubleMu/Run2012D-PromptReco-v1/AOD', 		'Cert_190456-208686_8TeV_PromptReco_Collisions12_JSON_PixAlignRecovery.txt'],
	   'DoubleMuonD2' : 	['data',	'/DoubleMu/Run2012D-16Jan2013-v2/AOD', 			'Cert_207883-208307_8TeV_16Jan2013ReReco_Collisions12_JSON.txt'],

	   # Double electron
           'DoubleElectronA' : 	['data',	'/DoubleElectron/Run2012A-13Jul2012-v1/AOD', 		'Cert_190456-196531_8TeV_13Jul2012ReReco_Collisions12_JSON_v2.txt'],
	   'DoubleElectronA2' :	['data',	'/DoubleElectron/Run2012A-recover-06Aug2012-v1/AOD', 	'Cert_190782-190949_8TeV_06Aug2012ReReco_Collisions12_JSON.txt'],
	   'DoubleElectronB' : 	['data',	'/DoubleElectron/Run2012B-13Jul2012-v1/AOD', 		'Cert_190456-196531_8TeV_13Jul2012ReReco_Collisions12_JSON_v2.txt'],
	   'DoubleElectronC' : 	['data',	'/DoubleElectron/Run2012C-24Aug2012-v1/AOD', 		'Cert_198022-198523_8TeV_24Aug2012ReReco_Collisions12_JSON.txt'],
	   'DoubleElectronC2' :	['data',	'/DoubleElectron/Run2012C-PromptReco-v2/AOD', 		'Cert_190456-203002_8TeV_PromptReco_Collisions12_JSON_v2.txt'],
	   'DoubleElectronC3' : ['data',	'/DoubleElectron/Run2012C-EcalRecover_11Dec2012-v1/AOD','Cert_201191-201191_8TeV_11Dec2012ReReco-recover_Collisions12_JSON.txt'],
	   'DoubleElectronD' : 	['data',	'/DoubleElectron/Run2012D-PromptReco-v1/AOD', 		'Cert_190456-208686_8TeV_PromptReco_Collisions12_JSON_PixAlignRecovery.txt'],
	   'DoubleElectronD2' :	['data',	'/DoubleElectron/Run2012D-16Jan2013-v1/AOD', 		'Cert_207883-208307_8TeV_16Jan2013ReReco_Collisions12_JSON.txt'],

	   # Signal
	   'signal' :		['MC',		'/DYJJ01JetsToLL_M-50_MJJ-200_TuneZ2Star_8TeV-madgraph_tauola/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],

           # DY backgrounds
	   'DY' :		['MC',		'/DYJetsToLL_M-50_TuneZ2Star_8TeV-madgraph-tarball/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'DY1' :		['MC',		'/DY1JetsToLL_M-50_TuneZ2Star_8TeV-madgraph/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'DY2' :		['MC',		'/DY2JetsToLL_M-50_TuneZ2Star_8TeV-madgraph/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'DY3' :		['MC',		'/DY3JetsToLL_M-50_TuneZ2Star_8TeV-madgraph/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'DY4' :		['MC',		'/DY4JetsToLL_M-50_TuneZ2Star_8TeV-madgraph/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],

	   # WJets backgrounds
	   'WJets':		['MC',		'/WJetsToLNu_TuneZ2Star_8TeV-madgraph-tarball/Summer12_DR53X-PU_S10_START53_V7A-v2/AODSIM'],
	   'W1Jets':		['MC',		'/W1JetsToLNu_TuneZ2Star_8TeV-madgraph/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'W2Jets':		['MC',		'/W2JetsToLNu_TuneZ2Star_8TeV-madgraph/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'W3Jets':		['MC',		'/W3JetsToLNu_TuneZ2Star_8TeV-madgraph/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'W4Jets':		['MC',		'/W4JetsToLNu_TuneZ2Star_8TeV-madgraph/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],

	   # Diboson backgrounds
	   'WW' :		['MC',		'/WW_TuneZ2star_8TeV_pythia6_tauola/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'WZ' :		['MC',		'/WZ_TuneZ2star_8TeV_pythia6_tauola/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'ZZ' :		['MC',		'/ZZ_TuneZ2star_8TeV_pythia6_tauola/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],

	   # QCD backgrounds
	   'QCD100' : 		['MC',		'/QCD_HT-100To250_TuneZ2star_8TeV-madgraph-pythia/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'QCD250' : 		['MC',		'/QCD_HT-250To500_TuneZ2star_8TeV-madgraph-pythia6/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'QCD500' : 		['MC',		'/QCD_HT-500To1000_TuneZ2star_8TeV-madgraph-pythia6/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'QCD1000' : 		['MC',		'/QCD_HT-1000ToInf_TuneZ2star_8TeV-madgraph-pythia6/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],

	   # Top backgrounds
	   'TTJets' : 		['MC',		'/TTJets_MassiveBinDECAY_TuneZ2star_8TeV-madgraph-tauola/Summer12_DR53X-PU_S10_START53_V7A-v2/AODSIM'],
	   'T-W' :		['MC',		'/T_tW-channel-DR_TuneZ2star_8TeV-powheg-tauola/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'T-s' :		['MC',		'/T_s-channel_TuneZ2star_8TeV-powheg-tauola/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'T-t' :		['MC',		'/T_t-channel_TuneZ2star_8TeV-powheg-tauola/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'Tbar-W' :		['MC',		'/Tbar_tW-channel-DR_TuneZ2star_8TeV-powheg-tauola/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'Tbar-s' : 		['MC',		'/Tbar_s-channel_TuneZ2star_8TeV-powheg-tauola/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM'],
	   'Tbar-t' : 		['MC',		'/Tbar_t-channel_TuneZ2star_8TeV-powheg-tauola/Summer12_DR53X-PU_S10_START53_V7A-v1/AODSIM']
           }



sample = sys.argv[1]
sampleInfo = samples[sample]

if os.path.isdir(sample):
  print sample + ' already exists!'
  sys.exit(0)
if sampleInfo[1] == '':
  print 'No dataset for ' + sample
  sys.exit(0)

print sample
print '\tType:\t\t' + sampleInfo[0]
print '\tDataset:\t' + sampleInfo[1]
if sampleInfo[0] == 'data': print '\tJson:\t\t' + sampleInfo[2]

os.mkdir(sample)

shutil.copyfile(os.environ['CMSSW_BASE'] + '/src/EWKV/Analyzer/test/ewkv_cfg.py',sample + '/ewkv_cfg.py') 
for line in fileinput.input(sample + '/ewkv_cfg.py', inplace=1):
  if sampleInfo[0] == 'MC': print line.replace('MC = False','MC = True'),
  else: print line.replace('MC = True','MC = False'),

shutil.copyfile('crabTemplate.cfg',sample + '/crab.cfg')
for line in fileinput.input(sample + '/crab.cfg', inplace=1):
  print line.replace('DATASET', sampleInfo[1]).replace('SAMPLE', sample),
  if '[CMSSW]' in line: 
    if sampleInfo[0] == 'data': print 'lumi_mask               = ' + sampleInfo[2]
  if '[USER]' in line:
    if sampleInfo[0] == 'data': print 'additional_input_files  = ' + sampleInfo[2]

if sampleInfo[0] == 'data': shutil.copyfile('JSON/' + sampleInfo[2], sample + '/' + sampleInfo[2])
