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
#include "../shellVariables.h"
#include "color.h"

class plotProfile{
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
  plotProfile(){};
  bool configureStack();
  void loop(TString type);
  void next(TString type);
  TH1* displaceBins(TH1* hh, double displaceFactor);
};

int main(){
  plotProfile *myPlotHistos = new plotProfile();
  if(!myPlotHistos->configureStack()) return 1;
 // myPlotHistos->loop("ZMUMU");
  myPlotHistos->loop("ZEE");
  delete myPlotHistos;
  return 0;
}

bool plotProfile::configureStack(){
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


void plotProfile::loop(TString type){
  //Get histogram info from file and loop
  std::ifstream readFile;
  readFile.open((getCMSSWBASE() + "/src/EWKV/Macros/histos/profile.config")); 
  if(!readFile.is_open()){
    std::cout << "plot.C:\t\t\t!!!\t" + getCMSSWBASE() + "/src/EWKV/Macros/histos/profile.config not found!" << std::endl;
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
    readFile >> fileName >> xtitle >> ytitle;
    xtitle.ReplaceAll("__"," ");
    ytitle.ReplaceAll("__"," ");
    fileName = getCMSSWBASE() + "/src/EWKV/Macros/outputs/rootfiles/" + type + "/" + fileName + ".root";
    next(type);
  }
  readFile.close();
  return;
}


void plotProfile::next(TString type){
  std::map<TString, TH1*> profileHists;

  TLegend *leg = new TLegend(.85,.85,1,1);
  TCanvas *c = new TCanvas("Canvas", "Canvas", 1500, 1200);
  if(type == "ZMUMU") leg->AddEntry((TObject*)0, "Z #rightarrow #mu#mu", "");
  if(type == "ZEE") leg->AddEntry((TObject*)0, "Z #rightarrow ee", "");


  TFile *file = new TFile(fileName);

  //data
  TH1 *data = (TH1*) file->Get(name + "_data");
  data->SetMarkerStyle(20);
  data->SetStats(0);
  data->SetTitle("");
  leg->AddEntry(data, "data", "p");
  if(data->GetBinWidth(1) != data->GetBinWidth(2)) c->SetLogx();
  data->GetXaxis()->SetTitle(xtitle);
  data->GetYaxis()->SetTitle(ytitle);
  data->Draw("e1p X0");


  int i = 1;
  for(auto mc = mcs.begin(); mc != mcs.end(); ++mc){
    if((*mc != "ZVBF") && (*mc != "DY")) continue;
    double displaceFactor;
    if(*mc == "ZVBF") displaceFactor = -.1;
    if(*mc == "DY") displaceFactor = .1;

    profileHists[*mc] = (TH1*) file->Get(name + "_" + *mc);
    if(profileHists[*mc]->GetMaximum()*1.1 > data->GetMaximum()) data->SetMaximum(profileHists[*mc]->GetMaximum()*1.1);
    if(profileHists[*mc]->GetMinimum()*.9 < data->GetMinimum()) data->SetMinimum(profileHists[*mc]->GetMinimum()*.9);
    TH1* displacedBinHisto = displaceBins(profileHists[*mc], displaceFactor);
    displacedBinHisto->SetMarkerStyle(2+i%4);
    displacedBinHisto->SetMarkerColor(color[*mc]);
    displacedBinHisto->SetLineColor(color[*mc]);
    leg->AddEntry(displacedBinHisto, *mc, "p");
    displacedBinHisto->Draw("e1p X0 same");
  }
  leg->Draw();

  c->SaveAs(getCMSSWBASE() + "/src/EWKV/Macros/outputs/pdf/" + type + "/" + name + ".pdf");  
  delete c;
  return;
}


TH1* plotProfile::displaceBins(TH1* hh, double displaceFactor){
  bool logx = (hh->GetBinWidth(1) != hh->GetBinWidth(2));
  TH1* h = (TH1*) hh->Clone();
  TAxis *axis = h->GetXaxis();
  int bins = axis->GetNbins();
  Axis_t logMin = TMath::Log10(axis->GetXmin());
  Axis_t logMax = TMath::Log10(axis->GetXmax());
  Axis_t logWidth = (logMax-logMin)/bins;
  Axis_t width = (axis->GetXmax() - axis->GetXmin())/bins;
  Axis_t *new_bins = new Axis_t[bins + 1];
  for(int i = 0; i <= bins; i++){
    if(logx) new_bins[i] = TMath::Power(10, logMin + logWidth*(i+displaceFactor)); 
    else new_bins[i] = axis->GetXmin() + width*(i+displaceFactor);
  }
  axis->Set(bins, new_bins);
  for(int i = 1; i <= bins; i++) h->Fill(h->GetBinCenter(i), hh->GetBinContent(i));
  delete new_bins;
  return h;
}

