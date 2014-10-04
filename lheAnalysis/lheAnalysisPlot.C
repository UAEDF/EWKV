#include <fstream>
#include <iostream>
#include <map>
#include <math.h>
#include <TString.h>
#include <TStyle.h>
#include <TLorentzVector.h>
#include <TH1D.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TLegend.h>
#include "../Macros/environment.h"


void addHist(TH1D* destination, TH1D* source, double scale, bool underflow, bool overflow){
  for(int i = (underflow ? 0 : 1); i <= source->GetNbinsX() + (overflow ? 1 : 0); ++i){
    int bin = destination->Fill(source->GetBinCenter(i), source->GetBinContent(i)*scale);
    destination->SetBinError(bin, source->GetBinError(i)*scale);
  }
}


int main(int argc, char* argv[]){
  bool ystar = false;
  std::map<TString, double> events, xsec;
  events["all"] = 299999990.;	events["all1000"] = 18872752.;
  events["qcd"] = 299999992.; 	events["qcd1000"] = 11738168.;
  events["ewk"] = 299999993.; 	events["ewk1000"] = 14989893.;
  xsec["all"] = 40.8845;	xsec["all1000"] = 1.69653;	xsec["all_cut1000"] = 1.59065;
  xsec["qcd"] = 40.3595;	xsec["qcd1000"] = 1.56452;	xsec["qcd_cut1000"] = 1.45615;
  xsec["ewk"] = 0.397162;	xsec["ewk1000"] = 0.10709;	xsec["ewk_cut1000"] = 0.114177;

  std::map<TString, TH1D*> hist;
  for(TString sample : {"ewk","qcd","all"}){
    if(ystar)   hist[sample] = new TH1D(sample+"ystar", sample+"y*", 25, 0, 5);
    else	hist[sample] = new TH1D(sample, sample, 136, 100, 3500);

    TFile *f = new TFile(sample + "_cuts.root");
    TH1D *h; f->GetObject(sample + (ystar ? "y*" : ""), h);
    addHist(hist[sample], h, 1./events[sample]*xsec[sample], true, ystar);
    delete h;

    if(!ystar){
      f->GetObject(sample + "1000", h);
      addHist(hist[sample], h, 1./events[sample]*xsec[sample], false, true);
    }
    delete f, h;
  }

  TCanvas *c = new TCanvas("Canvas", "Canvas", 1);
  TPad *pad1 = new TPad("pad1","1", 0, 0.4, 1., 1.0, 0); pad1->Draw(); pad1->SetLeftMargin(0.1);
  TPad *pad2 = new TPad("pad2","2", 0, 0.2, 1., 0.4, 0); pad2->Draw(); pad2->SetLeftMargin(0.1);
  TPad *pad3 = new TPad("pad3","3", 0, 0.0, 1., 0.2, 0); pad3->Draw(); pad3->SetLeftMargin(0.1);
  pad1->cd();
  pad1->SetLogy();
  hist["all"]->SetLineColor(kViolet);
  hist["all"]->SetStats(0);
  hist["all"]->SetTitle("Interference between QCD and EWK Zjj");
  hist["all"]->GetXaxis()->SetTitle(ystar ? "y*" : "M_{jj}");
  hist["all"]->GetYaxis()->SetTitle("normalized events");
  hist["all"]->GetYaxis()->SetTitleOffset(1.5);
  hist["all"]->SetMinimum(hist["ewk"]->GetMinimum());
  hist["all"]->DrawCopy();
  hist["ewk"]->SetLineColor(kGreen);
  hist["ewk"]->DrawCopy("same");
  hist["qcd"]->SetLineColor(kRed);
  hist["qcd"]->DrawCopy("same");
  hist["sum"] = (TH1D*) hist["qcd"]->Clone("sum");
  hist["sum"]->Add(hist["ewk"]);
  hist["sum"]->SetLineColor(kBlue);
  hist["sum"]->DrawCopy("same");
  TLegend *l = new TLegend(0.70, 0.70, 0.9, 0.9);
  l->SetFillColor(kWhite);
  for(auto entry = hist.begin(); entry != hist.end(); ++entry) l->AddEntry(entry->second, entry->first, "l");
  l->Draw();
  pad2->cd();
  pad2->SetGridy();
  gStyle->SetGridWidth(.6);
  hist["diff"] = (TH1D*) hist["all"]->Clone("ewk_plus_Int");
  hist["diff"]->SetLineColor(kRed);
  hist["diff"]->Add(hist["qcd"], -1);
  hist["diff"]->Divide(hist["ewk"]);
  hist["diff"]->SetTitle("");
  hist["diff"]->GetXaxis()->SetTitle("");
  hist["diff"]->GetXaxis()->SetLabelSize(.1);
  hist["diff"]->GetYaxis()->SetTitle("#sigma_{ewk+int}/#sigma_{ewk}");
  hist["diff"]->GetYaxis()->SetTitleSize(.15);
  hist["diff"]->GetYaxis()->SetNdivisions(5);
  hist["diff"]->GetYaxis()->SetLabelSize(.1);
  hist["diff"]->GetYaxis()->SetTitleOffset(.3);
  hist["diff"]->SetMaximum(ystar ? 3.5 : 1.8);
  hist["diff"]->SetMinimum(ystar ? 0.9 : 0.5);
  hist["diff"]->DrawCopy();
  gStyle->SetFuncWidth(0.3); 

  if(ystar){
/*    TF1 *fit = new TF1("fit","[0]+[1]*x+[2]*(x*x)+[3]*(x*x*x)", 0, 5);
    hist["diff"]->Fit(fit,"QSMR");
    fit->DrawCopy("same");
    std::cout << "(" << fit->GetParameter(0) << " +- " << fit->GetParError(0) << ") + ";
    std::cout << "(" << fit->GetParameter(1) << " +- " << fit->GetParError(1) << ")*x + ";
    std::cout << "(" << fit->GetParameter(2) << " +- " << fit->GetParError(2) << ")*(x*x) + ";
    std::cout << "(" << fit->GetParameter(3) << " +- " << fit->GetParError(3) << ")*(x*x*x)" << std::endl;*/
  } else {
    TF1 *fit = new TF1("fit","[0]+[1]/x+[2]/(x*x)+[3]/(x*x*x)+[4]*x+[5]/log(x)", 100,3500); 
    hist["diff"]->Fit(fit,"QSMR");
    fit->DrawCopy("same");
    std::cout << "(" << fit->GetParameter(0) << " +- " << fit->GetParError(0) << ") + ";
    std::cout << "(" << fit->GetParameter(1) << " +- " << fit->GetParError(1) << ")/x + ";
    std::cout << "(" << fit->GetParameter(2) << " +- " << fit->GetParError(2) << ")/(x*x) + ";
    std::cout << "(" << fit->GetParameter(3) << " +- " << fit->GetParError(3) << ")/(x*x*x) + ";
    std::cout << "(" << fit->GetParameter(4) << " +- " << fit->GetParError(4) << ")*x + ";
    std::cout << "(" << fit->GetParameter(5) << " +- " << fit->GetParError(5) << ")/logx" << std::endl;
  }

  pad3->cd();
  pad3->SetGridy();
  hist["diff"] = (TH1D*) hist["all"]->Clone("QCD_plus_Int");
  hist["diff"]->SetLineColor(kRed);
  hist["diff"]->Add(hist["ewk"], -1);
  hist["diff"]->Divide(hist["qcd"]);
  hist["diff"]->SetTitle("");
  hist["diff"]->GetXaxis()->SetTitle("");
  hist["diff"]->GetXaxis()->SetLabelSize(.1);
  hist["diff"]->GetYaxis()->SetTitle("#sigma_{qcd+int}/#sigma_{qcd}");
  hist["diff"]->GetYaxis()->SetTitleSize(.15);
  hist["diff"]->GetYaxis()->SetLabelSize(.1);
  hist["diff"]->GetYaxis()->SetNdivisions(5);
  hist["diff"]->GetYaxis()->SetTitleOffset(.3);
  hist["diff"]->SetMinimum(ystar? 0.99 : 0.75);
  if(ystar) hist["diff"]->SetMaximum(1.035);
  hist["diff"]->DrawCopy();
  c->SaveAs("interference.pdf");
  c->SaveAs("interference.C");
  return 0;
}
