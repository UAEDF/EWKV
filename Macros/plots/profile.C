#include <map>
#include <vector>
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TFile.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TString.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <TProfile.h>
#include <TStyle.h>

#include "log.h"
#include "../environment.h"
#include "color.h"

class plotProfile{
  int bins;
  double min, max, xmin, xmax, ymin, ymax, rmin, rmax;
  TString xtitle, ytitle, nameData, nameMC;
  bool logX;
  bool JES;
  int removeJESbinLeft, removeJESbinRight;
  std::map<TString, TString> fileName;

  std::vector<TString> mcs;
  std::map<TString, int> color;
  std::map<TString, bool> useLegend;
  std::map<TString, TString> legendNames;

  public:
  plotProfile(){};
  void configureStack();
  void loop(TString type);
  void defaultStyle(TString type);
  void combinedStyle();
  void ratioStyle();
};

int main(){
  plotProfile *myPlotHistos = new plotProfile();
  myPlotHistos->configureStack();
//myPlotHistos->loop("ZMUMU");
//myPlotHistos->loop("ZEE");
  myPlotHistos->loop("both");
  delete myPlotHistos;
  return 0;
}


void plotProfile::configureStack(){
  std::ifstream readFile; 
  getStream(readFile, getCMSSWBASE() + "src/EWKV/Macros/plots/stack.config"); 
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
}


void plotProfile::loop(TString type){
  //Get histogram info from file and loop
  std::ifstream readFile; 
  getStream(readFile, getCMSSWBASE() + "src/EWKV/Macros/histos/profilePAS.config"); 
  while(!readFile.eof()){
    TString useLine;
    readFile >> useLine;
    if(useLine != "1" && useLine != "3"){
      readFile.ignore(unsigned(-1), '\n');
      continue;
    }
    bool plot;
    readFile >> nameData >> bins >> min >> max >> logX >> plot;
    nameMC = nameData;
    if(!plot){
      readFile.ignore(unsigned(-1), '\n');
      continue;
    }
    TString fileName_;
    readFile >> fileName_ >> xtitle >> ytitle >> ymin >> ymax >> xmin >> xmax >> rmin >> rmax;
    xtitle.ReplaceAll("__"," ");
    ytitle.ReplaceAll("__"," ");
    for(TString i : {"ZEE","ZMUMU"}) fileName[i] = getTreeLocation() + "outputs/rootfiles/" + i + "/" + fileName_ + ".root";
    if(type != "both") defaultStyle(type);
//    else combinedStyle();
    else {
      ratioStyle();
      if(useLine == "3"){
        nameMC += "mcfmUp";
        ratioStyle();
      }
    }
  }
  readFile.close();
  return;
}


void plotProfile::defaultStyle(TString type){
  std::map<TString, TProfile*> profileHists;
 
  TCanvas *c = new TCanvas("Canvas", "Canvas", 1500, 1200);
  TLegend *leg = new TLegend();
  if(type == "ZMUMU") leg->AddEntry((TObject*)0, "Z #rightarrow #mu#mu", "");
  if(type == "ZEE")   leg->AddEntry((TObject*)0, "Z #rightarrow ee", "");
  TFile *file = new TFile(fileName[type]);

  profileHists["data"] = getProfile(file, "data", nameData);
  for(TString i : {"0","1","2","3","4"}) profileHists["DY"] = safeAdd(profileHists["DY"], getProfile(file, "DY" + i, nameMC, true));
  if(!profileHists["DY"]){ std::cout << "No DY for " << nameMC << "!" << std::endl; exit(1);}

  //data
  profileHists["data"]->SetMarkerStyle(20);
  profileHists["data"]->SetStats(0);
  profileHists["data"]->SetTitle("");
  profileHists["data"]->SetMarkerSize(1.5);
  leg->AddEntry(profileHists["data"], "data", "p");
  if(isLogX(profileHists["data"])) c->SetLogx();
  profileHists["data"]->GetXaxis()->SetTitle(xtitle);
  profileHists["data"]->GetYaxis()->SetTitle(ytitle);
  profileHists["data"]->Draw("e1p X0");

  int j = 1;
  for(TString i : {"ZVBF","DY"}){
    double displaceFactor;
    if(i == "ZVBF") displaceFactor = -.1;
    if(i == "DY") displaceFactor = .1;

    fixRange(profileHists["data"], profileHists[i]);
    TH1* displacedBinHisto = displaceBins(profileHists[i], displaceFactor);
    displacedBinHisto->SetMarkerStyle(2+j%4);
    displacedBinHisto->SetMarkerSize(1.5);
    displacedBinHisto->SetMarkerColor(color[i]);
    displacedBinHisto->SetLineColor(color[i]);
    if(i == "ZVBF") leg->AddEntry(displacedBinHisto, "signal", "p");
    else leg->AddEntry(displacedBinHisto, i, "p");
    displacedBinHisto->Draw("e1p X0 same");
    ++j;
  }
  leg->Draw();

  c->SaveAs(getTreeLocation() + "outputs/pdf/" + type + "/" + nameMC + ".pdf");  
  delete c;
  return;
}


void plotProfile::combinedStyle(){
  std::map<TString, TFile*> file;
  std::map<TString, TProfile*> profileHists;
  std::map<TString, TH1*> th1Hists;
  TCanvas *c = new TCanvas("c","c", 600, 600);

  for(TString j : {"ZEE","ZMUMU"}){
    file[j] = new TFile(fileName[j]);
    profileHists["data" + j] = getProfile(file[j], "data", nameData);
    for(TString i : {"0","1","2","3","4"}) profileHists["DY"+j] = safeAdd(profileHists["DY"+j], getProfile(file[j], "DY" + i, nameMC, true));
    if(!profileHists["DY"+j]){ std::cout << "No DY for " << nameMC << "!" << std::endl; exit(1);}
  }

  double k = -0.15;
  for(TString i : {"DYZMUMU","dataZMUMU","dataZEE","DYZEE"}){ th1Hists[i] = displaceBins(profileHists[i], k); k += 0.1;}
  for(TString i : {"DYZMUMU","dataZMUMU","dataZEE","DYZEE"}){ th1Hists[i]->SetMarkerSize(.8);}
  for(TString i : {"dataZMUMU","dataZEE"}){ th1Hists[i]->SetLineColor(1); th1Hists[i]->SetMarkerColor(1);}
  for(TString i : {"DYZMUMU","DYZEE"}){ th1Hists[i]->SetLineColor(2); th1Hists[i]->SetMarkerColor(2);}
  for(TString i : {"DYZMUMU","dataZMUMU"}){ th1Hists[i]->SetMarkerStyle(21);}
  for(TString i : {"DYZEE","dataZEE"}){ th1Hists[i]->SetMarkerStyle(24);}

//  if(th1Hists["DYZMUMU"]->GetBinWidth(1) != th1Hists["DYZMUMU"]->GetBinWidth(2)) c->SetLogx();
  th1Hists["DYZMUMU"]->GetXaxis()->SetTitle(xtitle);
  th1Hists["DYZMUMU"]->GetYaxis()->SetTitle(ytitle);
  th1Hists["DYZMUMU"]->GetYaxis()->SetTitleOffset(1.2);
  th1Hists["DYZMUMU"]->SetMinimum(ymin);
  th1Hists["DYZMUMU"]->SetMaximum(ymax);
  th1Hists["DYZMUMU"]->SetTitle("");
  th1Hists["DYZMUMU"]->SetStats(0);
  th1Hists["DYZMUMU"]->Draw("e1p X0");
  for(TString i : {"dataZMUMU","dataZEE","DYZEE"}) th1Hists[i]->Draw("e1p X0 same");


  TLegend *l = new TLegend(.15,.65,.37,.88);
  l->AddEntry(th1Hists["dataZMUMU"], "Z #rightarrow #mu#mu data", "p");
  l->AddEntry(th1Hists["DYZMUMU"], "Z #rightarrow #mu#mu simulation", "p");
  l->AddEntry(th1Hists["dataZEE"], "Z #rightarrow ee data", "p");
  l->AddEntry(th1Hists["DYZEE"], "Z #rightarrow ee simulation", "p");
  l->SetBorderSize(0);
  l->SetFillColor(0);
  l->SetTextSize(0.045);
  l->Draw("");

  TLatex *tex = new TLatex(0.12,0.97,"CMS preliminary Z+jets      #sqrt{s} = 8 TeV      L = 19.7 fb^{ -1}");
  tex->SetNDC();
  tex->SetTextFont(43);
  tex->SetTextSize(20);
  tex->SetLineWidth(2);
  tex->Draw();

  if(!exists(getTreeLocation() + "outputs/pdf/combined/")) system("mkdir -p " + getTreeLocation() + "outputs/pdf/combined/");
  c->SaveAs(getTreeLocation() + "outputs/pdf/combined/" + nameMC + ".pdf");  
  delete c;
  gDirectory->GetList()->Delete();
  return;
}


void plotProfile::ratioStyle(){
  std::map<TString, TFile*> file;
  std::map<TString, TProfile*> profileHists;
  std::map<TString, TH1*> th1Hists;
  TCanvas *c = new TCanvas("c","c", 600, 600);

  double horizontalSplit = 0.30;
  TPad *padUP = new TPad("padUP","up",   0.02, horizontalSplit, 1, 1,   0);
  TPad *padDN = new TPad("padDN","down", 0.02, 0.02, 1, horizontalSplit, 0);
  padUP->Draw(); padDN->Draw();
  padDN->SetGridy();
  padUP->SetLeftMargin(.12);   padDN->SetLeftMargin(.12);
  padUP->SetRightMargin(.005); padDN->SetRightMargin(.005);
  padUP->SetTopMargin(0.07);   padDN->SetTopMargin(0.);
  padUP->SetBottomMargin(0.);  padDN->SetBottomMargin(.30);
  if(logX){ padUP->SetLogx(); padDN->SetLogx();};

  //Set up frontground frames
  TH2F* frame = new TH2F("frame","",2, xmin, xmax ,10, ymin, ymax);
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

  TH2F* frameRatio = new TH2F("frameratio","",2, xmin, xmax ,10, rmin , rmax);
  frameRatio->SetStats(kFALSE);
  frameRatio->GetXaxis()->SetMoreLogLabels(kTRUE);
  frameRatio->GetXaxis()->SetNoExponent(kTRUE);

  frameRatio->GetXaxis()->SetTitle(xtitle);
  frameRatio->GetXaxis()->SetTitleSize(.12);
  frameRatio->GetXaxis()->SetTitleOffset(1);

  frameRatio->GetYaxis()->SetTitle("data/MC");
  frameRatio->GetYaxis()->SetNdivisions(10);
  frameRatio->GetYaxis()->SetTitleSize(.12);
  frameRatio->GetYaxis()->SetLabelSize(.09);
  frameRatio->GetXaxis()->SetLabelSize(.1);
  frameRatio->GetYaxis()->SetTitleOffset(.5);
  padDN->cd(); frameRatio->Draw();


  padUP->cd();

  for(TString j : {"ZEE","ZMUMU"}){
    file[j] = new TFile(fileName[j]);
    profileHists["data" + j] = getProfile(file[j], "data", nameData);
    for(TString i : {"0","1","2","3","4"}) profileHists["DY"+j] = safeAdd(profileHists["DY"+j], getProfile(file[j], "DY" + i, nameMC, true));
    if(!profileHists["DY"+j]){ std::cout << "No DY for " << nameMC << "!" << std::endl; exit(1);}
  }

  double k = -0.15;
  for(TString i : {"dataZEE","DYZEE","dataZMUMU","DYZMUMU"}){ th1Hists[i] = displaceBins(profileHists[i], k); k += 0.1;}
  for(TString i : {"DYZMUMU","dataZMUMU","dataZEE","DYZEE"}){ th1Hists[i]->SetMarkerSize(.8);}
  for(TString i : {"dataZMUMU","dataZEE"}){ th1Hists[i]->SetLineColor(1); th1Hists[i]->SetMarkerColor(1);}
  for(TString i : {"DYZMUMU","DYZEE"}){ th1Hists[i]->SetLineColor(2); th1Hists[i]->SetMarkerColor(2);}
  for(TString i : {"DYZMUMU","dataZMUMU"}){ th1Hists[i]->SetMarkerStyle(21);}
  for(TString i : {"DYZEE","dataZEE"}){ th1Hists[i]->SetMarkerStyle(24);}

  if(th1Hists["DYZMUMU"]->GetBinWidth(1) != th1Hists["DYZMUMU"]->GetBinWidth(2)) c->SetLogx();
  for(TString i : {"DYZMUMU","dataZMUMU","dataZEE","DYZEE"}) th1Hists[i]->Draw("e1p X0 same");

  TLegend *l = new TLegend(.15,.65,.37,.88);
  l->AddEntry(th1Hists["dataZMUMU"], "Z #rightarrow #mu#mu data", "p");
  l->AddEntry(th1Hists["DYZMUMU"], "Z #rightarrow #mu#mu simulation", "p");
  l->AddEntry(th1Hists["dataZEE"], "Z #rightarrow ee data", "p");
  l->AddEntry(th1Hists["DYZEE"], "Z #rightarrow ee simulation", "p");
  l->SetBorderSize(0);
  l->SetFillColor(0);
  l->SetTextSize(0.045);
  l->Draw("");

  TLatex *tex = new TLatex(0.12,0.95,"#bf{CMS preliminary, #sqrt{s}=8 TeV, #scale[0.5]{#int}L=19.7 fb^{-1}}");
  tex->SetNDC();
  tex->SetTextFont(43);
  tex->SetTextSize(20);
  tex->SetLineWidth(2);
  tex->Draw();


  padDN->cd();

  k = -0.1;
  for(TString i : {"ZEE","ZMUMU"}){   
    th1Hists["ratio" + i] = (TH1D*) displaceBins(profileHists["data" + i]->ProjectionX(), k); k += 0.2;
    for(int bin = 1; bin < th1Hists["ratio"+i]->GetNbinsX()+1; ++bin){ 
      double hMC     = th1Hists["DY"+i]->GetBinContent(bin);
      double hMC_er  = th1Hists["DY"+i]->GetBinError(bin);
      double hDat    = th1Hists["data"+i]->GetBinContent(bin);
      double hDat_er = th1Hists["data"+i]->GetBinError(bin);
      if(hMC != 0){
        double hR = hDat/hMC;
        double h_e = sqrt((hMC_er/hMC)*(hMC_er/hMC)+(hDat_er/hDat)*(hDat_er/hDat));
        double error = h_e*hR;
        th1Hists["ratio"+i]->SetBinContent(bin, hR);
        th1Hists["ratio"+i]->SetBinError(bin, error);
      } else { 
        th1Hists["ratio"+i]->SetBinContent(bin, NULL);
        th1Hists["ratio"+i]->SetBinError(bin, NULL);
      }
    }
  }
  for(TString i : {"ratioZEE","ratioZEE"}){ th1Hists[i]->SetMarkerSize(.8);}
  th1Hists["ratioZMUMU"]->SetMarkerStyle(21);
  th1Hists["ratioZEE"]->SetMarkerStyle(24);
  th1Hists["ratioZMUMU"]->SetLineColor(kBlack);
  th1Hists["ratioZEE"]->SetLineColor(kBlack);
  gStyle->SetErrorX(0.);    
  th1Hists["ratioZEE"]->Draw("same e");
  th1Hists["ratioZMUMU"]->Draw("same e");

  if(!exists(getTreeLocation() + "outputs/pdf/combined/")) system("mkdir -p " + getTreeLocation() + "outputs/pdf/combined/");
  c->SaveAs(getTreeLocation() + "outputs/pdf/combined/" + nameMC + ".pdf");  
  delete c;
  gDirectory->GetList()->Delete();
  return;
}
