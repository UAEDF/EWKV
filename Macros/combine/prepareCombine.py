#!/usr/bin/env python
from ROOT import TFile, TH1
 
base = "BDT"
tag = "20131015_Fast3"
puUpTag = "20131017_66785_Fast"
puDownTag = "20131017_73815_Fast"

mcGroups = {'DY': ["DY0","DY1","DY2","DY3","DY4"],
            'TTJets': ["TTJetsSemiLept","TTJetsFullLept","TTJetsHadronic"],
            'T-s': ["T-s","Tbar-s"], 'T-W': ["T-W","Tbar-W"], 'T-t': ["T-t","Tbar-t"],
            'WW': ["WW"], 'WZ': ["WZ"], 'ZZ': ["ZZ"], 'WJets': ["WJets"],
            'ewkZjj': ["ZVBF"]}

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
  puUpFile = TFile(getTreeLocation() + "outputs/rootfiles/" + type + "/" + puUpTag + ".root")
  puDownFile = TFile(getTreeLocation() + "outputs/rootfiles/" + type + "/" + puDownTag + ".root")
  for category in ["","_100","_200"]:
    dir = combineFile.mkdir(type + category)
    dir.cd()

    data = sourceFile.Get(base + category + "_data")
    data.Write("data_obs")

    for systematic in ["","_JESUp","_JESDown","_PUUp","_PUDown"]:
      file = sourceFile
      if systematic == "_PUUp": file = puUpFile
      if systematic == "_PUDown": file = puDownFile

      plot = base + category
      if systematic == "_JESUp": plot += "JES+"
      if systematic == "_JESDown": plot += "JES-"

      expected = {}
      for name, mcs in mcGroups.iteritems():
        thisGroup = merge(file, plot, mcs)
        thisGroup.Write(name + systematic)
        expected[name] = ('%.3f' % thisGroup.Integral())
       
      if systematic == "":
        with open("ewkZjj_8TeV"+category+"_" + type + ".dat", "wt") as card:
          with open("ewkZjj_template.dat", "rt") as template:
            for line in template:
              line = line.replace('$DIRECTORY', type + category).replace('$data', '%d'%data.Integral())
              for name, n in expected.iteritems(): line = line.replace(('$'+name).ljust(16), n.ljust(16))
              card.write(line)

exit()
