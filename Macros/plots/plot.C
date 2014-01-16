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
#include "../environment.h"
#include "color.h"

bool retrieveMeanAndRMS;
bool putEvents = true;
bool bottomLegend = true;
std::ofstream writeFile;


class plotHistos{
  int bins;
  double min, max, xmin, xmax, ymin, ymax, rmin, rmax;
  TString xtitle, ytitle, tag, fileName, name, nameData;
  bool logX, logY;
  bool JES;
  int removeJESbinLeft, removeJESbinRight;
  bool plotSignificance;

  std::vector<TString> mcs;
  std::map<TString, int> color;
  std::map<TString, bool> useLegend;
  std::map<TString, TString> legendNames;

  public:
  plotHistos(){};
  bool configureStack();
  void loop(TString type);
  void next(TString type);
  void addLegEntry(TLegend *leg, TH1D *hist, TString type, TString sample, TString option);
};

int main(int argc, char *argv[]){
  if(argc > 2 && (((TString) argv[2]) == "-r")) retrieveMeanAndRMS = true;
  else retrieveMeanAndRMS = false;
  if(retrieveMeanAndRMS){
    writeFile.open((getCMSSWBASE() + "src/EWKV/Macros/ewkv/meanAndRMS.h").Data());
    writeFile << "#include \"ewkv.h\"" << std::endl << std::endl;
    writeFile << "void ewkvAnalyzer::initQGCorrections(){" << std::endl;
  }
  plotHistos *myPlotHistos = new plotHistos();
  if(!myPlotHistos->configureStack()) return 1;
  for(TString type : typeSelector(argc, argv)) myPlotHistos->loop(type);
  if(retrieveMeanAndRMS){
    writeFile << "}" << std::endl;
    writeFile.close();
  }
  delete myPlotHistos;
  return 0;
}

bool plotHistos::configureStack(){
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
    legendName.ReplaceAll("__"," ");
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
  getStream(readFile, getCMSSWBASE() + "src/EWKV/Macros/histos/1D.config");
  TString version;
  readFile >> version;
  if(version != "v2") version == "v1";
  std::cout << "plot.C:\t\t\t\t1D.config " << version << std::endl;
  while(!readFile.eof()){
    TString useLine; readFile >> useLine;
    if(!useLine.IsDigit() || useLine == "0"){ readFile.ignore(unsigned(-1), '\n'); continue;}
    bool plot; readFile >> name >> bins >> min >> max >> logX >> plot;
    if(!plot){ readFile.ignore(unsigned(-1), '\n'); continue;}
    if(useLine == "2" || useLine == "4") plotSignificance = true;
    else plotSignificance = false;
    logY = true;
    if(version == "v1") readFile >> tag >> xtitle >> ytitle >> xmin >> xmax >> ymin >> ymax >> rmin >> rmax >> JES >> removeJESbinLeft >> removeJESbinRight;
    else readFile >> tag >> xtitle >> ytitle >> xmin >> xmax >> logY >> ymin >> ymax >> rmin >> rmax >> JES >> removeJESbinLeft >> removeJESbinRight;
    xtitle.ReplaceAll("__"," "); ytitle.ReplaceAll("__"," ");
    fileName = getTreeLocation() + "outputs/rootfiles/" + type + "/" + tag + ".root";
    nameData = name;
    next(type);
    if(useLine == "3" || useLine == "4"){
      name += "mcfmUp";
      next(type);
      name = nameData;
      name += "mjjUp";
      next(type);
    }
    if(name.Contains("BDT")) system("cp " + getTreeLocation() +  "cutflow/" + type + "/" + tag + ".tex " + getTreeLocation() +  "cutflow/" + type + "/cutflow.tex");
  }
  readFile.close();
  return;
}


void plotHistos::next(TString type){
  setTDRStyle(bottomLegend);

  //Set up histos
  std::map<TString, TH1D*> hists;
  for(TString h : mcs) 							 					hists[h] = nullptr;
  for(TString h : {"data", "ratio", "ratio+", "ratio-", "JES+", "JES-","signal only","thisLegendItem"}) 	hists[h] = nullptr;

  double horizontalSplit = 0.275;
  double leftMargin = .12;
  double rightMargin = .005;
  double topMargin = 0.06;
  double bottomMargin = 0.25;
  double bottomPad = 0;

  if(bottomLegend) horizontalSplit = (200.+horizontalSplit*600.)/800.;
  if(bottomLegend) bottomPad = 200./800.;

  //Set up Canvas and TPads
  TCanvas *c = new TCanvas("Canvas", "Canvas", 1);
  TPad *padUP = new TPad("padUP","up",   0, horizontalSplit, 1., 1.,   0);
  TPad *padDN = new TPad("padDN","down", 0, bottomPad, 1., horizontalSplit, 0);
  TPad *padLegend = new TPad("padLegend","downLegend", 0, 0, 1., bottomPad, 0);
  padUP->Draw(); padDN->Draw(); padLegend->Draw();
  if(logY) padUP->SetLogy();			
  padDN->SetGridy();
  padUP->SetLeftMargin(leftMargin);   	padDN->SetLeftMargin(leftMargin);
  padUP->SetRightMargin(rightMargin); 	padDN->SetRightMargin(rightMargin);
  padUP->SetTopMargin(topMargin);   	padDN->SetTopMargin(0.);
  padUP->SetBottomMargin(0.);  		padDN->SetBottomMargin(bottomMargin);
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
  frameRatio->GetXaxis()->SetTitleOffset(1.);

  if(plotSignificance) frameRatio->GetYaxis()->SetTitle("S");
//  if(plotSignificance) frameRatio->GetYaxis()->SetTitle("purity");
  else frameRatio->GetYaxis()->SetTitle("Data/MC");
  frameRatio->GetYaxis()->SetNdivisions(10);
  frameRatio->GetYaxis()->SetTitleSize(.12);
  frameRatio->GetYaxis()->SetLabelSize(.09);
  frameRatio->GetXaxis()->SetLabelSize(.1);
  frameRatio->GetYaxis()->SetTitleOffset(.5);
  padDN->cd(); frameRatio->Draw();


  //Set up legend
  TLegend *leg;
  if(bottomLegend){
    leg = new TLegend(leftMargin, 0, 1.-rightMargin, 1.);
    leg->SetTextSize(0.1);
    leg->SetNColumns(4);
    leg->AddEntry("", "","");
    leg->AddEntry("", "events","");
    leg->AddEntry("", "mean","");
    leg->AddEntry("", "RMS","");
  } else if(putEvents){
    leg = new TLegend(0.70, 0.61, 0.95, 0.91);
    leg->SetTextSize(0.04);
    leg->SetNColumns(2);
  } else {
    leg = new TLegend(0.78, 0.61, 0.95, 0.91);
    leg->SetTextSize(0.045);
  }
  leg->SetBorderSize(0);
  leg->SetFillColor(0);


  //Get the histograms (and immidiatly stack the MC histograms)
  TFile *file = new TFile(fileName);
  std::map<TString, double> events, RMS, mean;
  bool newLegendItem = true;
  auto mcbefore = mcs.rbegin();
  for(auto mc = mcs.rbegin(); mc != mcs.rend(); ++mc){
    hists[*mc] = safeAdd(hists[*mcbefore], getPlot(file, *mc, name, true));
    if(newLegendItem){ hists["temp"] = nullptr; newLegendItem = false;}
    hists["temp"] = safeAdd(hists["temp"], getPlot(file, *mc, name, true));
    if(useLegend[*mc]){
      if(hists["temp"]){
        events[*mc] = hists["temp"]->Integral();
        RMS[*mc] = hists["temp"]->GetRMS();
        mean[*mc] = hists["temp"]->GetMean();
      } else {
        events[*mc] = 0;
        RMS[*mc] = 0;
        mean[*mc] = 0;
      }
      newLegendItem = true;
    }
    hists["JES+"] = safeAdd(hists["JES+"], getPlot(file, *mc, name + "JES+", true));			// Old files ( < 21/10/2013)
    hists["JES-"] = safeAdd(hists["JES-"], getPlot(file, *mc, name + "JES-", true));			// Old files ( < 21/10/2013)
    hists["JES+"] = safeAdd(hists["JES+"], getPlot(file, *mc, name + "JESUp", true));			// New files 
    hists["JES-"] = safeAdd(hists["JES-"], getPlot(file, *mc, name + "JESDown", true));			// New files
    if(name.Contains("mcfmUp")){
      hists["JES+"] = safeAdd(hists["JES+"], getPlot(file, *mc, nameData + "JESUpmcfmUp", true));	// New files 
      hists["JES-"] = safeAdd(hists["JES-"], getPlot(file, *mc, nameData + "JESDownmcfmUp", true));	// New files
    }
    mcbefore = mc;
  }
  hists["data"] = getPlot(file, "data", nameData);
  hists["signal only"] = getPlot(file, "EWKZ", name);
  

  //Draw the histograms
  padUP->cd();
  bool firstLegendItem = true;
  for(TString mc : mcs){
    hists[mc]->SetLineWidth(1.);
    hists[mc]->SetLineColor(color[mc]);
    hists[mc]->SetFillColor(color[mc]);
    hists[mc]->Draw("same hist");
    if(useLegend[mc] && events[mc] > 0.5){
      leg->AddEntry(hists[mc], " " + legendNames[mc] + " ", "F");
      if(putEvents || bottomLegend){
        leg->AddEntry("", TString::Format("%d", (int)(events[mc]+.5)),"");
        if(bottomLegend){
          leg->AddEntry("", TString::Format("%.5f", mean[mc]),"");
          leg->AddEntry("", TString::Format("%.5f", RMS[mc]),"");
        }
      }
    }
  }
  addLegEntry(leg, hists[mcs.front()], type, "Total MC", "");
/*
  //In case of line for the signal only
  hists["signal only"]->SetLineWidth(2.);
  hists["signal only"]->SetLineColor(kBlack);
  hists["signal only"]->Draw("same");
  leg->AddEntry(hists["signal only"], " signal only ", "L");
*/

  hists["data"]->SetMarkerStyle(20);
  hists["data"]->SetLineWidth(3.);
  hists["data"]->Draw("same e");
  addLegEntry(leg, hists["data"], type, "Data", "P");
  drawText(type);
  fixOverlay();
  if(bottomLegend) padLegend->cd();
  leg->Draw();

  //Draw the ratio histograms (and calculate errors)
  padDN->cd();
  if(plotSignificance){
    TH1D* hObserved = (TH1D*) hists["data"]->Clone();
    TH1D* hExpected = (TH1D*) hists["data"]->Clone();
    for(int i = 0; i < bins + 2; ++i){
      double data 	= hists["data"]->GetBinContent(i);
      double MC 	= hists[mcs.front()]->GetBinContent(i);
      double signal 	= hists["signal only"]->GetBinContent(i);
      double plus 	= hists["JES+"]->GetBinContent(i);
      double min 	= hists["JES-"]->GetBinContent(i);
      double bkg 	= MC - signal;
      double deltaB 	= 0.5*((bkg-plus)*(bkg-plus)+(bkg-min)*(bkg-min));
      if(sqrt(bkg+deltaB) != 0){
        hObserved->SetBinContent(i, (data-bkg)/sqrt(bkg+deltaB));
        hExpected->SetBinContent(i, signal/sqrt(bkg+deltaB));
      } else {
        hObserved->SetBinContent(i, nullptr);
        hExpected->SetBinContent(i, nullptr);
      }
    }
 
    hObserved->SetLineWidth(2.);
    hObserved->SetFillColor(26);
    hObserved->Draw("same hist ][");
    hExpected->SetLineWidth(2.);
    hExpected->SetLineColor(color["EWKZ"]);
    hExpected->Draw("same hist ][");

    TLegend *legSignificance = new TLegend(0.15, 0.75, 0.35, 0.95);
    legSignificance->AddEntry(hObserved, " observed ", "l");
    legSignificance->AddEntry(hExpected, " expected ", "l");
    legSignificance->Draw();
  } else {
    hists["ratio"] = (TH1D*) hists["data"]->Clone();
    hists["ratio"]->Divide(hists[mcs.front()]);
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

    double kolmogorov = hists["data"]->KolmogorovTest(hists[mcs.front()]);
    TText kstext = TText();
    kstext.SetTextSize(0.1);
    kstext.DrawTextNDC(0.18,0.01, TString::Format("KS=%e",kolmogorov));

    //Ratio for JES
    if(JES && hists["JES+"] && hists["JES-"]){
      hists["ratio+"] = (TH1D*) hists["JES+"]->Clone();
      hists["ratio-"] = (TH1D*) hists["JES-"]->Clone();
      hists["ratio+"]->Divide(hists[mcs.front()]);
      hists["ratio-"]->Divide(hists[mcs.front()]);

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
   //   JESleg->Draw();

      hists["ratio+"]->SetLineWidth(2.);
      hists["ratio+"]->SetLineColor(kRed);
      hists["ratio+"]->Draw("same hist");
      hists["ratio-"]->SetLineWidth(2.);
      hists["ratio-"]->SetLineColor(kBlue);
      hists["ratio-"]->Draw("same hist");
    }
  }

  if(!exists(getTreeLocation() + "outputs/pdf/" + type + "/")) system("mkdir -p " + getTreeLocation() + "/outputs/pdf/" + type + "/");
  c->SaveAs(getTreeLocation() + "outputs/pdf/" + type + "/" + name + ".pdf");  
  delete c, padUP, padDN, padLegend, frame, frameRatio, file;
  gDirectory->GetList()->Delete();
  return;
}

void plotHistos::addLegEntry(TLegend *leg, TH1D *hist, TString type, TString sample, TString option){
  leg->AddEntry(hist, " " + sample + " ", option);
  if(putEvents){
    double events = hist->Integral();
    double RMS = hist->GetRMS();
    double mean = hist->GetMean();

    leg->AddEntry("", TString::Format("%d", (int)(events+.5)),"");
    if(bottomLegend){
      leg->AddEntry("", TString::Format("%.5f", mean),"");
      leg->AddEntry("", TString::Format("%.5f", RMS),"");
    }

    if(retrieveMeanAndRMS){
      if(name.Contains("HIG13011")){
        if(sample == "Total MC") sample = "MC";
        writeFile << "  mean" + sample + "[\"" << type << name << "\"] = " << TString::Format("%.5f", mean) << ";" << std::endl;
        writeFile << "  sigma" + sample + "[\"" << type << name << "\"] = " << TString::Format("%.5f", RMS) << ";" << std::endl << std::endl;
      }
    }
  }
}
