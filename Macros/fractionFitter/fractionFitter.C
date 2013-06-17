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
TString tag = "20130614h";
bool powheg = false;

int main(){
  gROOT->SetBatch();


  for(TString type : {"ZEE","ZMUMU"}){
    TFile *file = new TFile(getCMSSWBASE() + "/src/EWKV/Macros/outputs/rootfiles/" + type + "/" + tag + ".root");
    for(TString jes : {"","JES+","JES-"}){
      std::cout << std::endl << std::endl << type << "\t" << jes << std::endl;
      std::map<TString, TH1D*> histos;
      histos["data"] 				= (TH1D*) file->Get(plot + "_data");
      histos["signal"] 				= (TH1D*) file->Get(plot + jes + "_ZVBF");
      if(powheg){
        if(type == "ZMUMU") histos["DY"] 	= (TH1D*) file->Get(plot + jes + "_DYMUMU-powheg");
        else histos["DY"] 			= (TH1D*) file->Get(plot + jes + "_DYEE-powheg");
      } else {
        histos["DY"] 				= (TH1D*) file->Get(plot + jes + "_DY0");
        for(TString i : {"1","2","3","4"})      histos["DY"]->Add((TH1D*) file->Get(plot + jes + "_DY" + i));
      }



      TObjArray *mc = new TObjArray(2);
      mc->Add(histos["DY"]);
      mc->Add(histos["signal"]);
      TFractionFitter* fit = new TFractionFitter(histos["data"], mc, "Q");

      fit->Constrain(1,.95,1);
      fit->Constrain(2,0,.05);
      fit->SetRangeX(10,23); 
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

        std::cout << "Chi²/NDF\t" << fit->GetChisquare()/fit->GetNDF() << std::endl;
        std::cout << "DY:\t\t" << fractionDY << "\t" << errorFractionDY << std::endl;
        std::cout << "signal:\t\t" << fractionSignal << "\t" << errorFractionSignal << std::endl;
        std::cout << "kb:\t\t" << kb << "\t+/-\t" << eb << std::endl;
        std::cout << "ks:\t\t" << ks << "\t+/-\t" << es << std::endl << std::endl;;
      } else {std::cout << "Error in fit" << std::endl;}
    }
  }

  return 0; 
}
