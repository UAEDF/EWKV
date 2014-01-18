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
#include <stdio.h>

#include "../environment.h"

int minBin = 0;  
int maxBin = 28;
std::vector<TString> selections {"","100","200","300","400","500","600","750","1250","ystar3_1","ystar3_2","ystar3_3","ystar3_4","ystar3_5","central3","noncentral3"};

int main(int argc, char *argv[]){
  gROOT->SetBatch();
  TString rootVersion = gROOT->GetVersion();
  if(((TString) rootVersion(0,4)).Atof() < 5.34){ std::cout << "fractionFitter.C\t!!!\t: outdated ROOT version could lead to infinite loops, upgrade to 5.34" << std::endl; exit(1);}
  TString tag = ((TString) argv[2]);

  for(TString type : typeSelector(argc, argv)){
    TFile *file = new TFile(getTreeLocation() + "outputs/rootfiles/" + type + "/" + tag + ".root");
    TH1D *fitResults = new TH1D("Fit results", "Fit results with TFractionFitter (" + type + ")", selections.size(), 0, selections.size());

    for(TString selection : selections){
      TString plot = "BDT";
      if(selection != "") plot = "BDT_" + selection;
 
      makeDirectory("results/"+ tag);
      remove("results/"+ tag + "/" + plot + ".txt");
      std::ofstream out("results/"+ tag + "/" + plot + ".txt", std::ios_base::app | std::ios_base::out);
//      std::streambuf *coutbuf = std::cout.rdbuf(); //save old buf
//      std::cout.rdbuf(out.rdbuf()); //redirect std::cout to out.txt!
 
      for(TString syst : {"","JESUp","JESDown","JERUp","JERDown","PUUp","PUDown","QGUp","ystarUp","mjjUp","mcfmUp"}){
        out << std::endl << type << "\t\t" << syst << std::endl;
        std::map<TString, TH1D*> histos;
        std::map<TString, double> lumi;
        std::map<TString, double> expected;
   
        lumi["data"] = 19711;
        lumi["EWKZ"] = 3.33264e+06;
        lumi["DY2"] = 100839;
   
        histos["data"] 					= getPlot(file, "data", plot);
        histos["EWKZ"] 					= getPlot(file, "EWKZ", plot + syst);
        histos["DY"] 					= nullptr;
        histos["other"] 				= nullptr;
        for(TString i : {"DY1","DY2","DY3","DY4"}) 	histos["DY"] = safeAdd(histos["DY"], getPlot(file, i, plot + syst));
        for(TString i : {"TTJetsSemiLept", "TTJetsFullLept","TTJetsHadronic","WW","WZ","ZZ","WJets","T-W","Tbar-W","T-s","Tbar-s","T-t","Tbar-t","DY0","QCD100","QCD250","QCD500","QCD1000"}){  
          histos["other"] = safeAdd(histos["other"], getPlot(file, i, plot + syst, true));
        }
 
        histos["total"] = (TH1D*) histos["DY"]->Clone();
        histos["total"] = safeAdd(histos["total"], histos["other"]);
        histos["total"] = safeAdd(histos["total"], histos["EWKZ"]);
        histos["dataNC"] = (TH1D*) histos["data"]->Clone();
        histos["data"]->Add(histos["other"], -1);
        for(int bin = 0; bin <= histos["data"]->GetNbinsX() + 1; ++bin){
          if(histos["data"]->GetBinContent(bin) < 0) histos["data"]->SetBinContent(bin, 0);
        }
  
        for(auto hist = histos.begin(); hist != histos.end(); ++hist) expected[hist->first] = hist->second->Integral(minBin, maxBin);
   
        //Scale back to number of generated events for TFractionFitter (take DY2 for DY)
        histos["EWKZ"]->Scale(lumi["EWKZ"]/lumi["data"]);
        histos["DY"]->Scale(lumi["DY2"]/lumi["data"]);
   
        std::vector<TString> parts {"EWKZ","DY"};
        TObjArray *mc = new TObjArray(parts.size());
        for(TString i : parts) mc->Add(histos[i]);

        TFractionFitter* fit = new TFractionFitter(histos["data"], mc, "Q");
        fit->SetRangeX(minBin,maxBin);
        if(fit->Fit() != 0) std::cout << "fractionFitter.C:\t!!!\tError in fit" << std::endl; 

        std::map<TString, double> fraction, fractionError, ratio, k, kError;
        int j = 0;
        for(TString i : parts){
          fit->GetResult( j++, fraction[i], fractionError[i]);
          ratio[i] = expected[i]/expected["data"];
          k[i] = fraction[i]/ratio[i];
          kError[i] = fractionError[i]/ratio[i];
        }
   
        out << "Chi²/NDF\t" << fit->GetChisquare()/fit->GetNDF() << std::endl << "\t\t\t";
        for(TString i : parts) out << i << "\t\t";
        out << std::endl << "expected\t\t";
        for(TString i : parts) out << expected[i] << "\t\t";
        out << expected["total"] << "\t\t" << expected["dataNC"];
        out << std::endl << "fraction\t\t";
        for(TString i : parts) out << fraction[i] << "\t";
        out << std::endl << "fractionError\t\t";
        for(TString i : parts) out << fractionError[i] << "\t";
        out << std::endl << "k\t\t\t";
        for(TString i : parts) out << k[i] << (k[i] < 1? "\t":"\t\t");
        out << std::endl << "kError\t\t\t";
        for(TString i : parts) out << kError[i] << "\t";
        out << std::endl;

        if(syst == ""){
          fitResults->SetBinError(fitResults->Fill(plot, k["EWKZ"]), kError["EWKZ"]);
        }
 
        if(syst == ""){
          out << "Correlation matrix:\t";
          double c00 = fit->GetFitter()->GetCovarianceMatrixElement(0, 0);
          double c01 = fit->GetFitter()->GetCovarianceMatrixElement(0, 1);
          double c10 = fit->GetFitter()->GetCovarianceMatrixElement(1, 0);
          double c11 = fit->GetFitter()->GetCovarianceMatrixElement(1, 1);
          out << c00/sqrt(c00*c00) << "\t\t";
          out << c01/sqrt(c00*c11) << std::endl << "\t\t\t";
          out << c10/sqrt(c11*c00) << "\t";
          out << c11/sqrt(c11*c11) << std::endl;
        }
      }
      out << std::endl << std::endl;
    }
    gStyle->SetOptStat(0);
    TCanvas *c = new TCanvas("Canvas", "Canvas", 1);
    fitResults->SetMinimum(0);
    fitResults->GetYaxis()->SetTitle("fit result");
    fitResults->Draw();
    c->SaveAs("BDT_" + type + ".pdf");
  }
  return 0; 
}
