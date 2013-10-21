#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <TROOT.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TCanvas.h>
#include <TMarker.h>
#include <TFile.h>
#include <TTree.h>
#include <TChain.h>
#include <TH1F.h>
#include <THStack.h>
#include <TMath.h>
#include <Math/VectorUtil.h>
#include <TF1.h>
#include <TLegend.h>
#include <TLine.h>
#include <TColor.h>
#include <TObjArray.h>
#include <TFractionFitter.h>
#include <TText.h>

#include "../environment.h"

#include "../samples/sampleList.h" 
#include "../samples/sample.h" 

using namespace std;

TString plot = "BDT";
TString tag = "20131021_Fast";

int main(){
  gROOT->SetBatch();

  for(TString type : {"ZEE","ZMUMU"}){
    TFile *file = new TFile(getTreeLocation() + "outputs/rootfiles/" + type + "/" + tag + ".root");
    for(TString syst : {"","JESUp","JESDown","JERUp","JERDown","PUUp","PUDown","QGUp","ystarUp","mjjUp","mcfmUp"}){
      std::cout << std::endl << type << "\t\t" << syst << std::endl;
      std::map<TString, TH1D*> histos;
      std::map<TString, double> lumi;
      std::map<TString, double> expected;

      lumi["data"] = 19788;
      lumi["ZVBF"] = 835070;
      lumi["DY0"] = 8693.5; 
      lumi["DY1"] = 36087.7; 
      lumi["DY2"] = 101652; 
      lumi["DY3"] = 181503; 
      lumi["DY4"] = 234021;
      lumi["WJets"] = 1434.75;
      lumi["TTJetsSemiLept"] = 242267;
      lumi["TTJetsFullLept"] = 488269;
      lumi["TTJetsHadronic"] = 99409.8;
      lumi["T-W"] = 44834.1;
      lumi["Tbar-W"] = 44455.9;
      lumi["T-s"] = 68591.3;
      lumi["Tbar-s"] = 79530.7;
      lumi["T-t"] = 66635.2;
      lumi["Tbar-t"] = 63031.7;
      lumi["WW"] = 182363;
      lumi["WZ"] = 301123;
      lumi["ZZ"] = 555110;

      histos["data"] 				= getPlot(file, "data", plot);
      histos["ZVBF"] 				= getPlot(file, "ZVBF", plot + syst);
      histos["DY"] 				= getPlot(file, "DY1", plot + syst);
      for(TString i : {"DY2","DY3","DY4"}) 	histos["DY"]->Add(getPlot(file, i, plot + syst));
      histos["other"] 				= getPlot(file, "TTJetsSemiLept", plot + syst);
      for(TString i : {"TTJetsFullLept","TTJetsHadronic","WW","WZ","ZZ","WJets","T-W","Tbar-W","T-s","Tbar-s","T-t","Tbar-t","DY0"}){  
        safeAdd(histos["other"], getPlot(file, i, plot + syst, true));
      }
      histos["data"]->Add(histos["other"], -1);

      for(auto hist = histos.begin(); hist != histos.end(); ++hist) expected[hist->first] = hist->second->Integral();

      //Scale back to number of generated events for TFractionFitter (take DY1 for DY)
      histos["ZVBF"]->Scale(lumi["ZVBF"]/lumi["data"]);
      histos["DY"]->Scale(lumi["DY2"]/lumi["data"]);

      std::vector<TString> parts {"ZVBF","DY"};
      TObjArray *mc = new TObjArray(parts.size());
      for(TString i : parts) mc->Add(histos[i]);

      TFractionFitter* fit = new TFractionFitter(histos["data"], mc, "Q");
//      fit->Constrain(1,0,0.02);
//      fit->Constrain(2,0,1);
//      fit->SetRangeX(16,22);
      if(fit->Fit() == 0){
        std::map<TString, double> fraction;
        std::map<TString, double> fractionError;
        std::map<TString, double> ratio;
        std::map<TString, double> k;
        std::map<TString, double> kError;
        int j = 0;
        for(TString i : parts){
          fit->GetResult( j++, fraction[i], fractionError[i]);
          ratio[i] = expected[i]/expected["data"];
          k[i] = fraction[i]/ratio[i];
          kError[i] = fractionError[i]/ratio[i];
        }

        std::cout << "Chi²/NDF\t" << fit->GetChisquare()/fit->GetNDF() << std::endl << "\t\t\t";
        for(TString i : parts) std::cout << i << "\t\t";
        std::cout << std::endl << "fraction\t\t";
        for(TString i : parts) std::cout << fraction[i] << "\t";
        std::cout << std::endl << "fractionError\t\t";
        for(TString i : parts) std::cout << fractionError[i] << "\t";
        std::cout << std::endl << "k\t\t\t";
        for(TString i : parts) std::cout << k[i] << (k[i] < 1? "\t":"\t\t");
        std::cout << std::endl << "kError\t\t\t";
        for(TString i : parts) std::cout << kError[i] << "\t";
        std::cout << std::endl;
      } else {std::cout << "Error in fit" << std::endl;}
    }
    std::cout << std::endl << std::endl;
  }

  return 0; 
}
