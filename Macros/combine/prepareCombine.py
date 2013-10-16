#!/usr/bin/env python
from ROOT import TFile, TH1
 
base = "BDT"
tag = "20131015_Fast3"

def getTreeLocation():
  return "/user/tomc/public/merged/EWKV/2013-06-JetIDfix/"

def merge(sourceFile, plot, histList):
  histMerged = sourceFile.Get(plot + "_" + histList[0]).Clone()
  for hist in histList[1:]: 
    if sourceFile.FindKey(plot + "_" + hist): histMerged.Add(sourceFile.Get(plot + "_" + hist))
  return histMerged


combineFile = TFile("ewkZjj_8TeV.root","RECREATE")
for type in ["ZMUMU","ZEE"]: 
  print "prepare " + type
  sourceFile = TFile(getTreeLocation() + "outputs/rootfiles/" + type + "/" + tag + ".root")
  for category in ["","_100","_200"]:
    dir = combineFile.mkdir(type + category)
    dir.cd()

    data = sourceFile.Get(base + category + "_data")
    data.Write("data_obs")

    for systematic in ["","_JESUp","_JESDown"]:
      plot = base + category
      if systematic == "_JESUp": plot += "JES+"
      if systematic == "_JESDown": plot += "JES-"

      DY = merge(sourceFile, plot, ["DY0","DY1","DY2","DY3","DY4"])
      DY.Write("DY" + systematic)
      

      top = merge(sourceFile, plot, ["TTJetsSemiLept","TTJetsFullLept","TTJetsHadronic","T-W","Tbar-W","T-s","Tbar-s","T-t","Tbar-t"])
      top.Write("top" + systematic)

      VV = merge(sourceFile, plot, ["WW","WZ","ZZ"])
      VV.Write("VV" + systematic)

      WJets = sourceFile.Get(plot + "_WJets")
      WJets.Write("WJets" + systematic)

      EWKZjj = sourceFile.Get(plot + "_ZVBF")
      EWKZjj.Write("ewkZjj" + systematic)

      if systematic == "":
        with open("ewkZjj_8TeV"+category+"_" + type + ".dat", "wt") as card:
          with open("ewkZjj_template.dat", "rt") as template:
            for line in template:
              card.write(line.replace('$DIRECTORY', type + category).replace('$data', '%d'%round(data.Integral())).replace('$ewkZjj', '%d'%round(EWKZjj.Integral())).replace('$DY', '%d'%round(DY.Integral())).replace('$VV', '%d'%round(VV.Integral())).replace('$top', '%d'%round(top.Integral())).replace('$WJets', '%d'%round(WJets.Integral())))

exit()
