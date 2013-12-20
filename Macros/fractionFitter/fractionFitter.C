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
#include "../samples/sampleList.h" 
#include "../samples/sample.h" 

using namespace std;

TString basePlot = "BDT";
int minBin = 0;  
int maxbin = 28;

int main(int argc, char *argv[]){
   gROOT->SetBatch();
   TString rootVersion = gROOT->GetVersion();
   if(((TString) rootVersion(0,4)).Atof() < 5.34){ std::cout << "fractionFitter.C\t!!!\t: outdated ROOT version could lead to infinite loops, upgrade to 5.34" << std::endl; exit(1);}
   TString tag = ((TString) argv[2]);

   TH1D *fitResultsZEE = new TH1D("fit results for ee", "fit results for ee", 16, -25, 775);
   TH1D *fitResultsZMUMU = new TH1D("fit results for mumu", "fit results for mumu", 16, -25, 775);

   for(TString mjjCut : {"","_100","_150","_200","_250","_300","_350","_400","_450","_500","_550","_600","_650","_700"}){
     TString plot = basePlot + mjjCut;
 
     remove("results/"+ tag + mjjCut + "_" + TString::Format("%d",minBin) + ".txt");
     std::ofstream out("results/"+ tag + mjjCut + "_" + TString::Format("%d",minBin) + ".txt", std::ios_base::app | std::ios_base::out);
     std::streambuf *coutbuf = std::cout.rdbuf(); //save old buf
     std::cout.rdbuf(out.rdbuf()); //redirect std::cout to out.txt!
 
     for(TString type : typeSelector(argc, argv)){
       TFile *file = new TFile(getTreeLocation() + "outputs/rootfiles/" + type + "/" + tag + ".root");
       for(TString syst : {"","JESUp","JESDown","JERUp","JERDown","PUUp","PUDown","QGUp","ystarUp","mjjUp","mcfmUp"}){
         std::cout << std::endl << type << "\t\t" << syst << std::endl;
         std::map<TString, TH1D*> histos;
         std::map<TString, double> lumi;
         std::map<TString, double> expected;
   
         lumi["data"] = 19788;
         lumi["ZVBF"] = 3.33264e+06;
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
         histos["DY"] 					= getPlot(file, "DY1", plot + syst);
         for(TString i : {"DY2","DY3","DY4"}) 		histos["DY"]->Add(getPlot(file, i, plot + syst));
         histos["other"] 				= getPlot(file, "TTJetsSemiLept", plot + syst);
         for(TString i : {"TTJetsFullLept","TTJetsHadronic","WW","WZ","ZZ","WJets","T-W","Tbar-W","T-s","Tbar-s","T-t","Tbar-t","DY0"}){  
           histos["other"] = safeAdd(histos["other"], getPlot(file, i, plot + syst, true));
         }
 
         histos["total"] = (TH1D*) histos["DY"]->Clone();
         histos["total"] = safeAdd(histos["total"], histos["other"]);
         histos["total"] = safeAdd(histos["total"], histos["ZVBF"]);
         histos["dataNC"] = (TH1D*) histos["data"]->Clone();
         histos["data"]->Add(histos["other"], -1);
  
         for(auto hist = histos.begin(); hist != histos.end(); ++hist) expected[hist->first] = hist->second->Integral(minBin, maxBin);
   
         //Scale back to number of generated events for TFractionFitter (take DY2 for DY)
         histos["ZVBF"]->Scale(lumi["ZVBF"]/lumi["data"]);
         histos["DY"]->Scale(lumi["DY2"]/lumi["data"]);
   
         std::vector<TString> parts {"ZVBF","DY"};
         TObjArray *mc = new TObjArray(parts.size());
         for(TString i : parts) mc->Add(histos[i]);
   
         TFractionFitter* fit = new TFractionFitter(histos["data"], mc, "Q");
         fit->SetRangeX(minBin,maxBin);
         if(fit->Fit() == 0){
           std::map<TString, double> fraction, fractionError, ratio, k, kError;
           int j = 0;
           for(TString i : parts){
             fit->GetResult( j++, fraction[i], fractionError[i]);
             ratio[i] = expected[i]/expected["data"];
             k[i] = fraction[i]/ratio[i];
             kError[i] = fractionError[i]/ratio[i];
           }
   
           std::cout << "Chi²/NDF\t" << fit->GetChisquare()/fit->GetNDF() << std::endl << "\t\t\t";
           for(TString i : parts) std::cout << i << "\t\t";
           std::cout << std::endl << "expected\t\t";
           for(TString i : parts) std::cout << expected[i] << "\t\t";
           std::cout << expected["total"] << "\t\t" << expected["dataNC"];
           std::cout << std::endl << "fraction\t\t";
           for(TString i : parts) std::cout << fraction[i] << "\t";
           std::cout << std::endl << "fractionError\t\t";
           for(TString i : parts) std::cout << fractionError[i] << "\t";
           std::cout << std::endl << "k\t\t\t";
           for(TString i : parts) std::cout << k[i] << (k[i] < 1? "\t":"\t\t");
           std::cout << std::endl << "kError\t\t\t";
           for(TString i : parts) std::cout << kError[i] << "\t";
           std::cout << std::endl;

           if(syst == "mjjUp"){
             int mjjCut_ = 0;
             if(mjjCut != "") mjjCut_ = ((TString)mjjCut.Strip(TString::kLeading, '_')).Atoi();
             if(type == "ZEE"){
               fitResultsZEE->SetBinContent(fitResultsZEE->FindBin(mjjCut_), k["ZVBF"]);
               fitResultsZEE->SetBinError(fitResultsZEE->FindBin(mjjCut_), kError["ZVBF"]);
             } else {
               fitResultsZMUMU->SetBinContent(fitResultsZMUMU->FindBin(mjjCut_), k["ZVBF"]);
               fitResultsZMUMU->SetBinError(fitResultsZMUMU->FindBin(mjjCut_), kError["ZVBF"]);
             } 
           }
 
           if(syst == ""){
             std::cout << "Correlation matrix:\t";
             double c00 = fit->GetFitter()->GetCovarianceMatrixElement(0, 0);
             double c01 = fit->GetFitter()->GetCovarianceMatrixElement(0, 1);
             double c10 = fit->GetFitter()->GetCovarianceMatrixElement(1, 0);
             double c11 = fit->GetFitter()->GetCovarianceMatrixElement(1, 1);
             std::cout << c00/sqrt(c00*c00) << "\t\t";
             std::cout << c01/sqrt(c00*c11) << std::endl << "\t\t\t";
             std::cout << c10/sqrt(c11*c00) << "\t";
             std::cout << c11/sqrt(c11*c11) << std::endl;
           }
         } else { std::cout << "fractionFitter.C:\t!!!\tError in fit" << std::endl;}
       }
       std::cout << std::endl << std::endl;
     }
   }
   gStyle->SetOptStat(0);
   TCanvas *cZEE = new TCanvas("Canvas", "Canvas", 1);
   fitResultsZEE->SetMinimum(0);
   fitResultsZEE->GetXaxis()->SetTitle("m_{jj} cut (GeV)");
   fitResultsZEE->GetYaxis()->SetTitle("fit result");
   fitResultsZEE->Draw();
   cZEE->SaveAs(basePlot + "_mjjUp_ZEE.pdf");
   TCanvas *cZMUMU = new TCanvas("Canvas2", "Canvas2", 1);
   fitResultsZMUMU->SetMinimum(0);
   fitResultsZMUMU->GetXaxis()->SetTitle("m_{jj} cut (GeV)");
   fitResultsZMUMU->GetYaxis()->SetTitle("fit result");
   fitResultsZMUMU->Draw();
   cZMUMU->SaveAs(basePlot + "_mjjUp_ZMUMU.pdf");
   return 0; 
}
