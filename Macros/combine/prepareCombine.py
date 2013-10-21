#!/usr/bin/env python
from ROOT import TFile, TH1
 
base = "BDT"
tag = "20131021_Fast"

mcGroups = {'DY': ["DY0","DY1","DY2","DY3","DY4"],
            'TTJets': ["TTJetsSemiLept","TTJetsFullLept","TTJetsHadronic"],
            'T-s': ["T-s","Tbar-s"], 'T-W': ["T-W","Tbar-W"], 'T-t': ["T-t","Tbar-t"],
            'WW': ["WW"], 'WZ': ["WZ"], 'ZZ': ["ZZ"], 'WJets': ["WJets"],
            'ewkZjj': ["ZVBF"]}

def getTreeLocation():
  return "/user/tomc/public/merged/EWKV/2013-06-JetIDfix/"

def getPlot(sourceFile, sample, plot, ignoreNonExist = False):
  if sourceFile.FindKey(sample + "/" + plot): return sourceFile.Get(sample + "/" + plot).Clone()
  if sourceFile.FindKey(plot + "_" + sample): return sourceFile.Get(plot + "_" + sample).Clone()
  if not ignoreNonExist: 
    print sample + "/" + plot + " not found!"
    exit(1)

def merge(sourceFile, plot, histList):
  histMerged = getPlot(sourceFile, histList[0], plot)
  for hist in histList[1:]: histMerged.Add(getPlot(sourceFile, hist, plot, True))
  return histMerged

combineFile = TFile("ewkZjj_8TeV.root","RECREATE")
for type in ["ZMUMU","ZEE"]: 
  print "prepare " + type
  sourceFile = TFile(getTreeLocation() + "outputs/rootfiles/" + type + "/" + tag + ".root")
  for category in ["","_100","_200"]:
    dir = combineFile.mkdir(type + category)
    dir.cd()

    getPlot(sourceFile, "data", base + category).Write("data_obs")

    for systematic in ["","JESUp","JESDown","JERUp","JERDown","PUUp","PUDown","QGUp","QGDown","mcfmUp","mcfmDown"]:
      plot = base + category + systematic
      if systematic == "QGDown" or systematic == "mcfmDown": plot = base + category

      expected = {}
      for name, mcs in mcGroups.iteritems():
        thisGroup = merge(sourceFile, plot, mcs)
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
