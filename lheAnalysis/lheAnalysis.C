#include <fstream>
#include <iostream>
#include <map>
#include <TString.h>
#include <TLorentzVector.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TLegend.h>
#include "../Macros/environment.h"

int main(int argc, char* argv[]){
  std::map<TString, TH1D*> hist;
  for(TString sample : {"ewk","qcd","all"}){
    int nEvent = 0, nEventInFile = 0;
    double xsec = 0, xsecInFile = 0;
    hist[sample] = new TH1D(sample, sample, 1000, 1000, 2500);
    for(int i = 1; i <= 1000; ++i){
      std::ifstream lheInput;
      getStream(lheInput, "/localgrid/tomc/" + sample + "Zjj_gridpack/lhe/" + sample + "Zjj_gridpack_" + TString::Format("%04d", i) + ".lhe");
      bool eventMode = false;
      TLorentzVector firstJet;
      bool firstJetInEvent = true;
      nEventInFile = 0;
      std::stringstream line;
      while(getLine(lheInput, line)){
        TString pid; line >> pid;
        if(eventMode && pid == "</event>"){ eventMode = false; firstJetInEvent = true;}
        if(eventMode){ 
          TString status; line >> status;
          if(status.Atoi() == 1 && fabs(pid.Atoi()) < 6){
            TString mother, mother2, color, color2, p1, p2, p3, p4, mass, lifetime, spin;
            line >> mother >> mother2 >> color >> color2 >> p1 >> p2 >> p3 >> p4 >> mass >> lifetime >> spin;
            TLorentzVector j = TLorentzVector(p1.Atof(), p2.Atof(), p3.Atof(), p4.Atof());
            if(firstJetInEvent){
              firstJet = j;
              firstJetInEvent = false;
            } else {
              TLorentzVector jj = TLorentzVector(j + firstJet);
              hist[sample]->Fill(jj.M());
            }
          }
        }
        if(pid == "<event>"){ eventMode = true; nEventInFile++; }
        if(pid == "<init>"){
          getLine(lheInput, line);getLine(lheInput, line);
          TString intWeight; line >> intWeight;
          xsecInFile = intWeight.Atof();
        }
      }
      lheInput.close();
      xsec = (xsec*nEvent + xsecInFile*nEventInFile)/(nEventInFile+nEvent);
      nEvent += nEventInFile;
    }
    hist[sample]->Scale(1./nEvent*xsec);
    std::cout << nEvent << " events in " << sample << "Zjj with xsec = " << xsec << " pb" << std::endl;
  }
  TCanvas *c = new TCanvas("Canvas", "Canvas", 1);
  hist["all"]->SetLineColor(kViolet);
  hist["all"]->SetStats(0);
  hist["all"]->SetTitle("Interference between QCD and EWK Zjj");
  hist["all"]->GetXaxis()->SetTitle("M_{jj}");
  hist["all"]->GetYaxis()->SetTitle("normalized events");
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
  c->SaveAs("interference.pdf");
  return 0;
}
