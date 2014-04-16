#!/bin/zsh

cd ../../../../../
export SCRAM_ARCH=slc5_amd64_gcc472 
[[ -d CMSSW_6_1_2 ]] || scram p CMSSW CMSSW_6_1_2
cd CMSSW_6_1_2/src 
eval `scram runtime -sh`
if [[ -d HiggsAnalysis ]]; then
  cd HiggsAnalysis/CombinedLimit
else
  git clone https://github.com/cms-analysis/HiggsAnalysis-CombinedLimit.git HiggsAnalysis/CombinedLimit
  cd HiggsAnalysis/CombinedLimit
  git pull origin master
  git checkout V03-05-00
  scramv1 b
fi
cd ../../combineTool
cp ../../../CMSSW_5_3_9_patch1/src/EWKV/Macros/combine/ewkZjj_8TeV.root .
cp -r ../../../CMSSW_5_3_9_patch1/src/EWKV/Macros/combine/cards .
