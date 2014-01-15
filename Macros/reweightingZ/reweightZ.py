#!/usr/bin/env python
from ROOT import TFile, TH1
 
tag = "20140115_Full"

def getTreeLocation():
  return "/user/tomc/public/merged/EWKV/2013-12/"

def getPlot(sourceFile, sample, plot, ignoreNonExist = False):
  hist = sourceFile.Get(sample + "/" + plot)
  if hist: return hist.Clone()
  hist = sourceFile.Get(plot + "_" + sample)
  if hist: return hist.Clone()
  if not ignoreNonExist: 
    print sample + "/" + plot + " not found!"
    exit(1)

def safeAdd(first, second):
  if first is None: return second
  if second is None: return first
  first.Add(second)
  return first

def merge(sourceFile, plot, histList):
  histMerged = getPlot(sourceFile, histList[0], plot)
  for hist in histList[1:]: histMerged = safeAdd(histMerged, getPlot(sourceFile, hist, plot, True))
  return histMerged


for type in ["ZMUMU","ZEE"]:
  sourceFile = TFile(getTreeLocation() + "outputs/rootfiles/" + type + "/" + tag + ".root")
  for plot in ["eta","pt"]:
    data = sourceFile.Get("data/dilepton_" + plot).Clone()
    data.Add(merge(sourceFile, "dilepton_" + plot, ["TTJetsSemiLept","TTJetsFullLept","TTJetsHadronic","T-W","Tbar-W","T-s","Tbar-s","T-t","Tbar-t","WW","WZ","ZZ","WJets","ZVBF","QCD100","QCD250","QCD500","QCD1000"]), -1)
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
