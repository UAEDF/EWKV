#include <fstream>
#include <iostream>
#include <map>
#include <math.h>
#include <TString.h>
#include <TStyle.h>
#include <TLorentzVector.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TLegend.h>
#include "../Macros/environment.h"


int main(int argc, char* argv[]){
  TString mjjBin = "";
  if(argc > 1) mjjBin = "_mjj1000";

  std::map<TString, double> events, xsec;
  events["all"] = (mjjBin == ""? 137439997. : 18872752.);
  events["qcd"] = (mjjBin == ""? 135989997. : 11738168.);
  events["ewk"] = (mjjBin == ""? 186532772. : 14989893.);
  xsec["all"] = (mjjBin == ""? 228.758 : 1.69653);
  xsec["qcd"] = (mjjBin == ""? 227.218 : 1.56452);
  xsec["ewk"] = (mjjBin == ""? 0.988631 : 0.10709);

  std::map<TString, TH1D*> hist;
  for(TString sample : {"ewk","qcd","all"}){
    TFile *f = new TFile(sample + mjjBin + ".root");
    f->GetObject(sample + (mjjBin == ""? "":"1000"), hist[sample]);
    hist[sample]->Scale(1./events[sample]*xsec[sample]);
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
  hist["all"]->GetXaxis()->SetTitle("M_{jj}");
  hist["all"]->GetYaxis()->SetTitle("normalized events");
  hist["all"]->GetYaxis()->SetTitleOffset(1.5);
  hist["all"]->SetMinimum(hist["ewk"]->GetMinimum());
  hist["all"]->DrawCopy();
  hist["ewk"]->SetLineColor(kGreen);
  hist["ewk"]->DrawCopy("same");
  hist["qcd"]->SetLineColor(kRed);
  hist["qcd"]->DrawCopy("same");
  hist["sum"] = (TH1D*) hist["qcd"]->Clone();
  hist["sum"]->Add(hist["ewk"]);
  hist["sum"]->SetLineColor(kBlue);
  hist["sum"]->DrawCopy("same");
  hist["all"]->SetLineColor(kViolet);
  hist["all"]->DrawCopy("same");
  TLegend *l = new TLegend(0.65, 0.65, 0.9, 0.9);
  l->SetFillColor(kWhite);
  for(auto entry = hist.begin(); entry != hist.end(); ++entry) l->AddEntry(entry->second, entry->first, "l");
  l->Draw();
  pad2->cd();
  pad2->SetGridy();
  gStyle->SetGridWidth(.6);
  hist["diff"] = (TH1D*) hist["all"]->Clone();
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
  hist["diff"]->DrawCopy();
  pad3->cd();
  pad3->SetGridy();
  hist["diff"] = (TH1D*) hist["all"]->Clone();
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
  hist["diff"]->DrawCopy();
  c->SaveAs("interference" + mjjBin + ".pdf");
  return 0;
}
