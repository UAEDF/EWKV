#!/usr/bin/env python
from ROOT import TFile, TH1
 
tag = "20130927_Full"

def getTreeLocation():
  return "/user/tomc/public/merged/EWKV/2013-06-JetIDfix/"

def merge(sourceFile, plot, histList):
  histMerged = sourceFile.Get(plot + "_" + histList[0]).Clone()
  for hist in histList[1:]: 
    if sourceFile.FindKey(plot + "_" + hist): histMerged.Add(sourceFile.Get(plot + "_" + hist))
  return histMerged 


for type in ["ZMUMU","ZEE"]:
  sourceFile = TFile(getTreeLocation() + "outputs/rootfiles/" + type + "/" + tag + ".root")
  for plot in ["eta","pt"]:
    data = sourceFile.Get("dilepton_" + plot + "_data").Clone()
    data.Add(merge(sourceFile, "dilepton_" + plot, ["TTJetsSemiLept","TTJetsFullLept","TTJetsHadronic","T-W","Tbar-W","T-s","Tbar-s","T-t","Tbar-t","WW","WZ","ZZ","WJets","ZVBF"]), -1)
    DY = merge(sourceFile, "dilepton_" + plot, ["DY0","DY1","DY2","DY3","DY4"])
    sumData = data.Integral()
    sumDY = DY.Integral()

    f = open(plot + 'Weigths_' + type + '.txt','w')
    f.write('min\tmax\tweight\n')
    for i in range(1, data.GetNbinsX() + 1):
      ratio = (data.GetBinContent(i)/sumData)/(DY.GetBinContent(i)/sumDY)
      f.write(str(data.GetBinLowEdge(i)) + '\t' + str(data.GetBinLowEdge(i+1)) + '\t' + str(ratio) + '\n')
    f.close()

exit()
