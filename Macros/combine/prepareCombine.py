#!/usr/bin/env python
from ROOT import TFile, TH1, TH1D, TAxis
 
tag7var = "20140414_Fast_7var"
tag13var = "20140414_Fast_13var"

plotCuts = ["_200","_ptHard_200","_ptHard_ystar_200"]

mcGroups = {'DY': ["DY0","DY1","DY2","DY3","DY4"],
            'Top': ["TTJetsSemiLept","TTJetsFullLept","TTJetsHadronic","T-s","Tbar-s","T-W","Tbar-W","T-t","Tbar-t"],
            'VV': ["WW","WZ","ZZ","WJets"],
            'EWKZ': ["EWKZ"]}

def getTreeLocation():
  return "/user/tomc/public/merged/EWKV/2013-12/"

def addErrorToHist(h, sigma, bin = 0):
  if sigma == 0: return h;
  h.SetBinContent(bin, h.GetBinContent(bin) + h.GetBinError(bin)*sigma);
  return h;

def getPlot(sourceFile, sample, plot, ignoreNonExist = False, addError = 0., bin = 0):
  hist = sourceFile.Get(sample + "/" + plot)
  if hist and hist.GetEntries() != 0: return addErrorToHist(hist.Clone(), addError, bin)
  if not ignoreNonExist: 
    print sample + "/" + plot + " not found!"
    exit(1)

def safeAdd(first, second):
  if first is None: return second
  if second is None: return first
  merged = first.Clone()
  merged.Add(second)
  return merged

def merge(sourceFile, plot, histList, addError = 0., bin = 0):
  histMerged = getPlot(sourceFile, histList[0], plot, False, addError, bin)
  for hist in histList[1:]: histMerged = safeAdd(histMerged, getPlot(sourceFile, hist, plot, True, addError, bin))
  return histMerged

expected = {}
expectedStr = {}
JESnorm = {}
combineFile = TFile("ewkZjj_8TeV.root","RECREATE")
for type in ["ZMUMU","ZEE"]: 
 for mcfm in ["","_mcfm"]:
  print "prepare " + type + mcfm
  sourceFile7var = TFile(getTreeLocation() + "outputs/rootfiles/" + type + "/" + tag7var + mcfm + ".root")
  sourceFile13var = TFile(getTreeLocation() + "outputs/rootfiles/" + type + "/" + tag13var + mcfm + ".root")
  for basePlot in ["mjj","BDT_7var","BDT_13var"]:
    sourceFile = sourceFile7var
    if basePlot == "BDT_13var": sourceFile = sourceFile13var
    basePlot_ = "BDT"
    if basePlot == "mjj":  basePlot_ = "dijet_mass"
    for plotCut in plotCuts:
      directoryName = type + "_" + basePlot + plotCut + mcfm;
      dir = combineFile.mkdir(directoryName)
      dir.cd()

      data = getPlot(sourceFile, "data", basePlot_ + plotCut)
      data.Write("data_obs")

      for systematic in ["","JESUp","JESDown","JERUp","JERDown","PUUp","PUDown","QGUp","QGDown","ptZUp","ptZDown"]:
        plot = basePlot_ + plotCut + systematic
        if systematic == "QGDown" or systematic == "JERDown" or systematic == "ptZDown": plot = basePlot_ + plotCut

        for name, mcs in mcGroups.iteritems():
          thisGroup = merge(sourceFile, plot, mcs)
          if systematic == "":
            thisGroup.Write(name)
            expectedStr[name] = ('%.3f' % thisGroup.Integral())
            expected[name] = thisGroup.Integral()
          else:
            thisGroup.Scale(thisGroup.Integral()/expected[name])
            thisGroup.Write(name + "_" + systematic)
            if systematic == "JESUp":
              JESnorm[name] = ('%.3f' % (thisGroup.Integral()/expected[name]))
              thisGroup.Write(name + "_JES_eUp")
            if systematic == "JESDown":
              thisGroup.Write(name + "_JES_eDown")

          if systematic == "" and name == "DY":
            for bin in range(1, data.GetNbinsX()+1):
              plotWithBinErrorUp = merge(sourceFile, plot, mcs, 1, bin)
              plotWithBinErrorUp.Write(name + "_b" + str(bin) + "Up")
              plotWithBinErrorDown = merge(sourceFile, plot, mcs, -1, bin)
              plotWithBinErrorDown.Write(name + "_b" + str(bin) + "Down")

            interferenceHist = merge(sourceFile, basePlot_ + plotCut + "interference", ["EWKZ"])
            interferenceHist.Scale(0.85)
            interferenceHist.Write("interference")
            expectedStr["interference"] = ('%.3f' % interferenceHist.Integral())
            expected["interference"] = interferenceHist.Integral()

      for intOption in ["","_interference"]:
        with open("cards/ewkZjj_8TeV_"+ basePlot + plotCut + intOption + mcfm +"_" + type + ".dat", "wt") as card:
          with open("ewkZjj_template" + intOption + ".dat", "rt") as template:
            for line in template:
              line = line.replace('$DIRECTORY', directoryName).replace('$data', '%d'%data.Integral())
              for name, n in expectedStr.iteritems(): line = line.replace(('$'+name).ljust(16), n.ljust(16))
              for name, n in JESnorm.iteritems(): line = line.replace(('$JES'+name).ljust(16), n.ljust(16))
              if basePlot == "mjj":
                if "b15" in line: continue
                if "b16" in line: continue
                if "b17" in line: continue
                if "b18" in line: continue
                if "b19" in line: continue
                if "b20" in line: continue
                if "b21" in line: continue
                if "b22" in line: continue
                if "b23" in line: continue
                if "b24" in line: continue
                if "b25" in line: continue
                if "b26" in line: continue
                if "b27" in line: continue
                if "b28" in line: continue
              card.write(line)

exit()
