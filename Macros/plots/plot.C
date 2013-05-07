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

class plotHistos{
  int bins;
  double min, max, xmin, xmax, ymin, ymax, rmin, rmax;
  TString xtitle, ytitle, fileName, name;
  bool logX;
  bool JES;
  int removeJESbinLeft, removeJESbinRight;

  std::vector<TString> mcs;
  std::map<TString, int> color;
  std::map<TString, bool> useLegend;
  std::map<TString, TString> legendNames;

  public:
  plotHistos(){};
  bool configureStack();
  void loop(TString type);
  void next(TString type);
};

int main(){
  plotHistos *myPlotHistos = new plotHistos();
  if(!myPlotHistos->configureStack()) return 1;
  myPlotHistos->loop("ZMUMU");
  myPlotHistos->loop("ZEE");
  delete myPlotHistos;
  return 0;
}

bool plotHistos::configureStack(){
  std::ifstream readFile;
  readFile.open((getCMSSWBASE() + "/src/EWKV/Macros/plots/stack.config")); 
  if(!readFile.is_open()){
    std::cout << "plot.C:\t\t\t!!!\t" + getCMSSWBASE() + "/src/EWKV/Macros/histos/stack.config not found!" << std::endl;
    return false;
  }
  while(!readFile.eof()){
    TString useLine;
    readFile >> useLine;
    if(useLine != "1"){
      readFile.ignore(unsigned(-1), '\n');
      continue;
    }
    TString legendName, stackColor;
    std::string sample;
    readFile >> legendName >> stackColor >> sample;
    std::stringstream sampleList(sample);
    bool first = true;
    while(std::getline(sampleList, sample, ',')){
      mcs.push_back(TString(sample));
      color[TString(sample)] = getColor(stackColor);
      useLegend[TString(sample)] = first;
      legendNames[TString(sample)] = legendName;
      first = false;
    }
  }
  readFile.close();
  return true;
}


void plotHistos::loop(TString type){
  //Get histogram info from file and loop
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
    fileName = getCMSSWBASE() + "/src/EWKV/Macros/outputs/rootfiles/" + type + "/" + fileName + ".root";
    next(type);
  }
  readFile.close();
  return;
}


void plotHistos::next(TString type){
  setTDRStyle();

  //Set up histos
  std::map<TString, TH1D*> hists;
  for(TString h : mcs) 							 hists[h] = new TH1D(h, h, bins, min, max);
  for(TString h : {"data", "ratio", "ratio+", "ratio-", "JES+", "JES-"}) hists[h] = new TH1D(h, h, bins, min, max);
  hists["signal only"] = new TH1D("signal only" ,"signal only" , bins, min, max);
  if(logX){
    for(auto h = hists.begin(); h != hists.end(); ++h) binLogX(h->second);
  }

  //Set up Canvas and TPads
  TCanvas *c = new TCanvas("Canvas", "Canvas", 1);
  double horizontalSplit = 0.30;
  TPad *padUP = new TPad("padUP","up",   0.02, horizontalSplit, 1., 1.,   0);
  TPad *padDN = new TPad("padDN","down", 0.02, 0.02, 1., horizontalSplit, 0);
  padUP->Draw(); padDN->Draw();
  padUP->SetLogy();
  padDN->SetGridy();
  padUP->SetLeftMargin(.12);   padDN->SetLeftMargin(.12);
  padUP->SetRightMargin(.005); padDN->SetRightMargin(.005);
  padUP->SetTopMargin(0.02);   padDN->SetTopMargin(0.);
  padUP->SetBottomMargin(0.);  padDN->SetBottomMargin(.30);
  if(logX){ padUP->SetLogx(); padDN->SetLogx();};


  //Set up frontground frames
  TH2F* frame = new TH2F("frame","frame",2, xmin, xmax ,10, ymin, ymax);
  frame->SetStats(kFALSE);
  frame->GetXaxis()->SetMoreLogLabels(kTRUE);
  frame->GetXaxis()->SetNoExponent(kTRUE);

  frame->GetXaxis()->SetTitle(xtitle);

  frame->GetXaxis()->SetLabelSize(.06);
  frame->GetYaxis()->SetTitle(ytitle);
  frame->GetYaxis()->SetTitleSize(.06);
  frame->GetYaxis()->SetLabelSize(.05);
  frame->GetYaxis()->SetTitleOffset(.9);
  frame->GetYaxis()->SetTickLength(0.015);
  padUP->cd(); frame->Draw();

  TH2F* frameRatio = new TH2F("frameRatio","frameRatio",2, xmin, xmax ,10, rmin , rmax);
  frameRatio->SetStats(kFALSE);
  frameRatio->GetXaxis()->SetMoreLogLabels(kTRUE);
  frameRatio->GetXaxis()->SetNoExponent(kTRUE);

  frameRatio->GetXaxis()->SetTitle(xtitle);
  frameRatio->GetXaxis()->SetTitleSize(.12);
  frameRatio->GetXaxis()->SetTitleOffset(1.2);

  frameRatio->GetYaxis()->SetTitle("Data/MC");
  frameRatio->GetYaxis()->SetNdivisions(10);
  frameRatio->GetYaxis()->SetTitleSize(.12);
  frameRatio->GetYaxis()->SetLabelSize(.09);
  frameRatio->GetXaxis()->SetLabelSize(.1);
  frameRatio->GetYaxis()->SetTitleOffset(.5);
  padDN->cd(); frameRatio->Draw();


  //Set up legend
  TLegend *leg = new TLegend(0.78, 0.64, 0.95, 0.94);
  leg->SetBorderSize(0);
  leg->SetFillColor(0);
  leg->SetTextSize(0.045);


  //Get the histograms (and immidiatly stack the MC histograms)
  TFile *file = new TFile(fileName);
  auto mcbefore = mcs.rbegin();
  for(auto mc = mcs.rbegin(); mc != mcs.rend(); ++mc){
    hists[*mc]->Add(hists[*mcbefore], (TH1D*) file->Get(name + "_" + *mc));
    if(file->FindKey(name + "JES+_" + *mc) != 0) hists["JES+"]->Add(hists["JES+"], (TH1D*) file->Get(name + "JES+_" + *mc));
    if(file->FindKey(name + "JES-_" + *mc) != 0) hists["JES-"]->Add(hists["JES-"], (TH1D*) file->Get(name + "JES-_" + *mc));
    if(*mc == "signal") hists["signal only"] = (TH1D*) file->Get(name + "_" + *mc);
    mcbefore = mc;
  }
  hists["data"]->Add(hists["data"], (TH1D*) file->Get(name + "_data"));


  //Draw the histograms
  padUP->cd();
  for(TString mc : mcs){
    hists[mc]->SetLineWidth(1.);
    hists[mc]->SetLineColor(color[mc]);
    hists[mc]->SetFillColor(color[mc]);
    hists[mc]->Draw("same hist");
    if(useLegend[mc]) leg->AddEntry(hists[mc], " " + legendNames[mc] + " ", "F");
  }
/*
  //In case of line for the signal only
  hists["signal only"]->SetLineWidth(2.);
  hists["signal only"]->SetLineColor(kBlack);
  hists["signal only"]->Draw("same");
  leg->AddEntry(hists["signal only"], " signal only ", "L");
*/
  hists["data"]->SetLineWidth(2.);
  hists["data"]->Draw("same e");
  leg->AddEntry(hists["data"], " Data ", "P");
  leg->Draw();
  drawText(type);
  fixOverlay();

  //Draw the ratio histograms (and calculate errors)
  padDN->cd();
  hists["ratio"]->Divide(hists["data"], hists[mcs.front()]);
  for(int bin = 1; bin < bins+1; ++bin){ 
    double hMC     = hists[mcs.front()]->GetBinContent(bin);
    double hMC_er  = hists[mcs.front()]->GetBinError(bin);
    double hDat    = hists["data"]->GetBinContent(bin);
    double hDat_er = hists["data"]->GetBinError(bin);
    if(hMC > 1.&& hDat > 0.5){
      double hR = hDat/hMC;
      double h_e = sqrt((hMC_er/hMC)*(hMC_er/hMC)+(hDat_er/hDat)*(hDat_er/hDat));
      double error = h_e*hR;
      hists["ratio"]->SetBinError(bin, error);
    } 
  }
  hists["ratio"]->SetLineWidth(2.);
  hists["ratio"]->Draw("same e");

  //Ratio for JES
  if(JES){
    hists["ratio+"]->Divide(hists["JES+"], hists[mcs.front()]);
    hists["ratio-"]->Divide(hists["JES-"], hists[mcs.front()]);

    //To remove bins from the JES (when fluctuations are too high)
    if(removeJESbinRight != 0 || removeJESbinLeft != 0){
      float maxJEC = hists["ratio"]->GetBinLowEdge(bins + 1 - removeJESbinRight);
      float minJEC = hists["ratio"]->GetBinLowEdge(1 + removeJESbinLeft);
      for(TString sign : {"+","-"}){
        hists["ratio"+sign+"temp"] = new TH1D("ratio"+sign+"_","ratio"+sign+"_", bins - removeJESbinRight - removeJESbinLeft, minJEC, maxJEC);
        if(logX) binLogX(hists["ratio"+sign+"temp"]);
        for(int i=1+removeJESbinLeft; i < bins + 1 - removeJESbinRight; ++i){
          hists["ratio"+sign+"temp"]->SetBinContent(i-removeJESbinLeft, hists["ratio"+sign]->GetBinContent(i));
        }
        hists["ratio"+sign] = hists["ratio"+sign+"temp"];
      }
    }

    TLegend *JESleg = new TLegend(.15, .76, .35, .96);
    JESleg->AddEntry(hists["ratio+"], "JES up", "l");
    JESleg->AddEntry(hists["ratio-"], "JES down", "l");
    JESleg->Draw();

    hists["ratio+"]->SetLineWidth(2.);
    hists["ratio+"]->SetLineColor(kRed);
    hists["ratio+"]->Draw("same hist");
    hists["ratio-"]->SetLineWidth(2.);
    hists["ratio-"]->SetLineColor(kBlue);
    hists["ratio-"]->Draw("same hist");
  }

  c->SaveAs(getCMSSWBASE() + "/src/EWKV/Macros/outputs/pdf/" + type + "/" + name + ".pdf");  
  delete c;
  return;
}

