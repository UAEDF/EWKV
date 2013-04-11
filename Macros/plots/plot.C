#include <map>
#include <vector>
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TFile.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TString.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "text.h"
#include "log.h"
#include "tdrstyle.h"
#include "../CMSSWBASE.h"
#include "color.h"

class plotHist{
  int bins;
  double min, max, xmin, xmax, ymin, ymax, rmin, rmax;
  TString xtitle, ytitle, fileName, name;
  bool logX;
  bool JES;
  int removeJESbinLeft, removeJESbinRight;
  TString type;

  std::vector<TString> mcs;
  std::map<TString, int> color;
  std::map<TString, bool> useLegend;
  std::map<TString, TString> legendNames;

  public:
  plotHist();
  void loop();
  void next();
};

int main(){
  initColor();
  plotHist *myPlotHist = new plotHist();
  myPlotHist->loop();
  delete myPlotHist;
  return 0;
}

plotHist::plotHist(){
  std::ifstream readFile;
  readFile.open((getCMSSWBASE() + "/src/EWKV/Macros/plots/stack.config")); 
  if(!readFile.is_open()){
    std::cout << "plot.C:\t\t\t!!!\t" + getCMSSWBASE() + "/src/EWKV/Macros/histos/stack.config not found!" << std::endl;
    return;
  }
  while(!readFile.eof()){
    TString useLine;
    readFile >> useLine;
    if(useLine != "1"){
      readFile.ignore(unsigned(-1), '\n');
      continue;
    }
    TString legendName;
    std::string sample;
    int stackColor;
    readFile >> legendName >> stackColor >> sample;
    std::stringstream sampleList(sample);
    bool first = true;
    while(std::getline(sampleList, sample, ',')){
      mcs.push_back(TString(sample));
      color[TString(sample)] = stackColor;
      useLegend[TString(sample)] = first;
      legendNames[TString(sample)] = legendName;
      first = false;
    }
  }
  readFile.close();
}

void plotHist::loop(){
  type = "ZMUMU";

  std::ifstream readFile;
  readFile.open((getCMSSWBASE() + "/src/EWKV/Macros/histos/1D.config")); 
  if(!readFile.is_open()){
    std::cout << "plot.C:\t\t\t!!!\t" + getCMSSWBASE() + "/src/EWKV/Macros/histos/1D.config not found!" << std::endl;
    return;
  }
  while(!readFile.eof()){
    TString useLine;
    readFile >> useLine;
    if(useLine != "1"){
      readFile.ignore(unsigned(-1), '\n');
      continue;
    }
    bool plot;
    readFile >> name >> bins >> min >> max >> logX >> plot;
    if(!plot){
      readFile.ignore(unsigned(-1), '\n');
      continue;
    }
    readFile >> fileName >> xtitle >> ytitle >> xmin >> xmax >> ymin >> ymax >> rmin >> rmax >> JES >> removeJESbinLeft >> removeJESbinRight; 
    xtitle.ReplaceAll("__"," ");
    ytitle.ReplaceAll("__"," ");
    fileName = getCMSSWBASE() + "/src/EWKV/Macros/outputs/rootfiles/" + type + "/" + fileName;
    next();
  }
  readFile.close();
  return;
}


void plotHist::next(){
  setTDRStyle();

  //Set uo histos  
  TH1D* hData 		= new TH1D("hData" ,"" , bins, min, max);
  TH1D* hRatio 		= new TH1D("hRatio" ,"" , bins, min, max);
  TH1D* hRatioPlus 	= new TH1D("hRatio+" ,"" , bins, min, max);
  TH1D* hRatioMin 	= new TH1D("hRatio-" ,"" , bins, min, max);
  TH1D* hJECplus 	= new TH1D("hJEC+" ,"" , bins, min, max);
  TH1D* hJECmin  	= new TH1D("hJEC-" ,"" , bins, min, max);
  if(logX){
    binLogX(hData);
    binLogX(hRatio);
    binLogX(hRatioPlus);
    binLogX(hRatioMin);
    binLogX(hJECplus);
    binLogX(hJECmin);
  }


  std::map<TString, TH1D*> hMC;
  for(auto mc = mcs.begin(); mc != mcs.end(); ++mc){
    hMC[*mc] = new TH1D(*mc ,"" , bins, min, max);
    if(logX) binLogX(hMC[*mc]);
  }
  hMC["signal only"] = new TH1D("signal only" ,"" , bins, min, max);
  if(logX) binLogX(hMC["signal only"]);

  //Make grid for plot
  TH2F* h = new TH2F("h","",2, xmin, xmax ,10, ymin, ymax);
  h->SetStats(kFALSE);
  h->GetXaxis()->SetMoreLogLabels(kTRUE);
  h->GetXaxis()->SetNoExponent(kTRUE);

  h->GetXaxis()->SetTitle(xtitle);
  h->GetYaxis()->SetTitle(ytitle);
  h->GetYaxis()->SetTitleSize(.06);
  h->GetYaxis()->SetLabelSize(.05);
  h->GetXaxis()->SetLabelSize(.06);
  h->GetYaxis()->SetTitleOffset(.9);

  TH2F* h1 = new TH2F("h1","",2, xmin, xmax ,10, rmin , rmax);
  h1->SetStats(kFALSE);
  h1->GetXaxis()->SetMoreLogLabels(kTRUE);
  h1->GetXaxis()->SetNoExponent(kTRUE);

  h1->GetYaxis()->SetNdivisions(10);
  h1->GetXaxis()->SetTitle(xtitle);
  h1->GetXaxis()->SetTitleSize(.12);
  h1->GetXaxis()->SetTitleOffset(1.2);

  h1->GetYaxis()->SetTitle("Data/MC");
  h1->GetYaxis()->SetTitleSize(.12);
  h1->GetYaxis()->SetLabelSize(.09);
  h1->GetXaxis()->SetLabelSize(.1);
  h1->GetYaxis()->SetTitleOffset(.5);

  //Legend and canvas
  TLegend *leg = new TLegend(0.78, 0.6, 0.95, 0.9);
  leg->SetBorderSize(0);
  leg->SetFillColor(0);
  leg->SetTextSize(0.045);
  TCanvas *MyC1 = new TCanvas("MyC1", "", 1);

  //Stack the histograms
  TFile *file = new TFile(fileName);
  auto mcbefore = mcs.begin();
  for(auto mc = mcs.begin(); mc != mcs.end(); ++mc){
    TH1D* histo = (TH1D*) file->Get(name + "_" + *mc);
    hMC[*mc]->Add(hMC[*mcbefore], histo);
    if(*mc == "signal") hMC["signal only"] = histo;
    mcbefore = mc;
  }

  if(JES){
    TH1D *histo = (TH1D*) file->Get(name + "_JECplus");
    hJECplus->Add(hJECplus, histo);

    histo = (TH1D*) file->Get(name + "_JECmin");
    hJECmin->Add(hJECmin, histo);
  }

  TH1D* histo = (TH1D*) file->Get(name + "_data");
  hData->Add(hData, histo);

  //Set up TPad
  float Top_margin   = 0.;
  float Left_margin  = 0.12;
  float Right_margin = 0.005;

  TPad *padUP = new TPad("padUP","up",0.02,0.30,1.,1.00);
  padUP->Draw();
  padUP->cd();
  if(logX) padUP->SetLogx();
 
  padUP->SetLogy();
  padUP->SetBorderSize(0);
  padUP->SetLeftMargin(Left_margin);
  padUP->SetRightMargin(Right_margin);
  padUP->SetTopMargin(Top_margin+0.02);
  padUP->SetBottomMargin(0.00);

  h->Draw();

  for(auto mc = mcs.rbegin(); mc != mcs.rend(); ++mc){
    hMC[*mc]->SetLineWidth(1.);
    hMC[*mc]->SetLineColor(color[*mc]);
    hMC[*mc]->SetFillColor(color[*mc]);
    if(*mc == "DY"){
     hMC[*mc]->SetLineWidth(2.);
     hMC[*mc]->SetLineColor(kRed);
//     hMC[*mc]->SetLineWidth(1.);
//     hMC[*mc]->SetLineColor(kBlack);
    }
    hMC[*mc]->Draw("same hist");
    if(useLegend[*mc]) leg->AddEntry(hMC[*mc], " "+legendNames[*mc] + " ", "F");
  }

  //In case of line for the signal only
  hMC["signal only"]->SetLineWidth(2.);
  hMC["signal only"]->SetLineColor(kBlack);
//hMC["signal only"]->Draw("same");
//leg->AddEntry(hMC["signal only"], " signal only ", "L");

  hData->SetLineWidth(2.);
  hData->Draw("same e");
  leg->AddEntry(hData, " Data ", "P");

  leg->Draw();

  drawText();

  hRatio->Divide(hData, hMC[mcs.back()]);

  //Ratio for JES
  if(JES){
    hRatioPlus->Divide(hJECplus, hMC[mcs.back()]);
    hRatioMin->Divide(hJECmin, hMC[mcs.back()]);

    //To remove bins from the JES (when fluctuations are too high)
    if(removeJESbinRight != 0 || removeJESbinLeft != 0){
      Float_t maxJEC = hRatio->GetBinLowEdge(bins + 1 - removeJESbinRight);
      Float_t minJEC = hRatio->GetBinLowEdge(1 + removeJESbinLeft);
      TH1D* hRatioPlus_ = new TH1D("hRatio" ,"" , bins - removeJESbinRight - removeJESbinLeft, minJEC, maxJEC);
      TH1D* hRatioMin_ = new TH1D("hRatio" ,"" , bins - removeJESbinRight - removeJESbinLeft, minJEC, maxJEC);
      if(logX){
        binLogX(hRatioPlus_);
        binLogX(hRatioMin_);
      }
      for(Int_t i=1+removeJESbinLeft; i < bins + 1 - removeJESbinRight; ++i){
        hRatioPlus_->SetBinContent(i-removeJESbinLeft, hRatioPlus->GetBinContent(i));
        hRatioMin_->SetBinContent(i-removeJESbinLeft, hRatioMin->GetBinContent(i));
      }
      hRatioPlus = hRatioPlus_;
      hRatioMin = hRatioMin_;
    }
  }

  //Error calculation in ratio
  for(int it = 1; it < bins+1; it++){ 
    double hMC_ = hMC[mcs.back()]->GetBinContent(it);
    double hMC_er = hMC[mcs.back()]->GetBinError(it);
    double hDat = hData->GetBinContent(it);
    double hDat_er = hData->GetBinError(it);
    if(hMC_ > 1.&& hDat > 0.5){
      double hR = hDat/hMC_;
      double h_e = sqrt((hMC_er/hMC_)*(hMC_er/hMC_)+(hDat_er/hDat)*(hDat_er/hDat));
      double Error = h_e*hR;
      hRatio->SetBinError(it, Error);
    } 
  }

  MyC1->cd(); 
  TPad *padDN = new TPad("padDN","",0.02,0.02,1.,0.30);
  padDN->Draw();
  padDN->cd();
  if(logX) padDN->SetLogx();

  padDN->SetGridy();
  padDN->SetBorderSize(0);
  padDN->SetLeftMargin(Left_margin);
  padDN->SetRightMargin(Right_margin);
  padDN->SetTopMargin(Top_margin);
  padDN->SetBottomMargin(0.30);

  h1->Draw();

  hRatio->SetLineWidth(2.);
  hRatio->Draw("same e");
  if(JES){
    TLegend *JESleg = new TLegend(.15, .76, .35, .96);
    JESleg->AddEntry(hRatioPlus, "JES up", "l");
    JESleg->AddEntry(hRatioMin, "JES down", "l");
    JESleg->Draw();

    hRatioPlus->SetLineWidth(2.);
    hRatioPlus->SetLineColor(kRed);
    hRatioPlus->Draw("same hist");
    hRatioMin->SetLineWidth(2.);
    hRatioMin->SetLineColor(kBlue);
    hRatioMin->Draw("same hist");
  }

  MyC1->SaveAs(getCMSSWBASE() + "/src/EWKV/Macros/outputs/pdf/" + type + "/" + name + ".pdf");  
  delete MyC1;
  return;
}

