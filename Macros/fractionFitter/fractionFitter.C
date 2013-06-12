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

#include "../shellVariables.h"

using namespace std;

TString plot = "BDTD";
TString tag = "20130612e";


int main(){
  gROOT->SetBatch();

  for(TString type : {"ZEE","ZMUMU"}){
    TFile *file = new TFile(getCMSSWBASE() + "/src/EWKV/Macros/outputs/rootfiles/" + type + "/" + tag + ".root");
    for(TString jes : {"","JES+","JES-"}){
      std::map<TString, TH1D*> histos;
      histos["data"] = 		(TH1D*) file->Get(plot + "_data");
      histos["DY"] = 		(TH1D*) file->Get(plot + jes + "_DY0");
      for(TString i : {"1","2","3","4"}){
        histos["DY"]->Add((TH1D*) file->Get(plot + jes + "_DY" + i));
      }
      histos["signal"] = 	(TH1D*) file->Get(plot + jes + "_ZVBF");

      TObjArray *mc = new TObjArray(2);
      mc->Add(histos["DY"]);
      mc->Add(histos["signal"]);
      TFractionFitter* fit = new TFractionFitter(histos["data"], mc, "Q");

      fit->Constrain(0,0.0,0.2);               // constrain fraction 1 to be between 0 and 1
      fit->Constrain(1,0.2,1.0);               // constrain fraction 1 to be between 0 and 1
      if(fit->Fit() == 0){                      
        double fractionDY, fractionSignal, errorFractionDY, errorFractionSignal;
        fit->GetResult( 0, fractionDY, errorFractionDY);
        fit->GetResult( 1, fractionSignal, errorFractionSignal);

        double Ns = histos["signal"]->Integral();
        double Nb = histos["DY"]->Integral();
        double Nd = histos["data"]->Integral();

        double Rs = Ns/Nd;
        double Rb = Nb/Nd;

        double kb = fractionDY/Rb;
        double eb = errorFractionDY/Rb;
        double ks = fractionSignal/Rs;
        double es = errorFractionSignal/Rs;

        std::cout << std::endl << std::endl << type << "\t" << jes << std::endl;
        std::cout << "Chi²\t" << fit->GetChisquare() << std::endl;
        std::cout << "DY:\t" << fractionDY << "\t" << errorFractionDY << std::endl;
        std::cout << "signal:\t" << fractionSignal << "\t" << errorFractionSignal << std::endl;
        std::cout << "kb:\t" << kb << "\t+/-\t" << eb << std::endl;
        std::cout << "ks:\t" << ks << "\t+/-\t" << es << std::endl << std::endl;;
      } else {std::cout << "Error in fit" << std::endl;}
    }
  }

  return 0; 
}
