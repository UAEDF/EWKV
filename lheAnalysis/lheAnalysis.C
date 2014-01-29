#include <fstream>
#include <iostream>
#include <map>
#include <math.h>
#include <TString.h>
#include <TLorentzVector.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TLegend.h>
#include "../Macros/environment.h"


int main(int argc, char* argv[]){
  std::map<TString, TH1D*> hist;
  for(TString sample : {"ewk","qcd","all"}){
    hist[sample] = new TH1D(sample, sample, 250, 1000, 3500);

    std::vector<std::pair<int, double>> eventsAndXsec;
    for(int i = 1; i <= (sample == "all"? 2000 : 1500); ++i){
      std::ifstream lheInput;
      if(!getStream(lheInput, "/localgrid/tomc/" + sample + "Zjj_gridpack/lhe/" + sample + "Zjj_gridpack_" + TString::Format("%04d", i) + ".lhe", true)) continue;
      std::stringstream line;

      int events = 0; double xsec = 0.;
      while(getLine(lheInput, line)){													//Read lhe line
        TString tag; line >> tag;
        if(tag == "<init>"){														//Init tag: cross section two lines further
          getLine(lheInput, line);getLine(lheInput, line);
          TString intWeight; line >> intWeight;
          xsec = intWeight.Atof();
        } else if(tag == "<event>"){													//Event found
          ++events;
          TLorentzVector jj; int jets = 0;
          while(getLine(lheInput, line)){												//Read particles until </event> tag
            TString pid; line >> pid; if(pid == "</event>") break;
            TString status, mother, mother2, color, color2, p1, p2, p3, p4, mass, lifetime, spin;
            line >> status >> mother >> mother2 >> color >> color2 >> p1 >> p2 >> p3 >> p4 >> mass >> lifetime >> spin;
            if(status.Atoi() == 1 && fabs(pid.Atoi()) < 6){										//Find jet
              ++jets;
              jj = TLorentzVector(jj + TLorentzVector(p1.Atof(), p2.Atof(), p3.Atof(), p4.Atof()));					//Add it to the dijet vector
              if(jets == 2) hist[sample]->Fill(jj.M());											//Fill if the two jets were found
              else if(jets > 2) std::cout << "Warning: more than 2 jets in the event!" << std::endl;
            }
          }
        }
      }
      lheInput.close();
      eventsAndXsec.push_back(std::make_pair(events, xsec));
    }

    int events = 0;
    double a = 0, b = 0;
    for(auto it = eventsAndXsec.begin(); it != eventsAndXsec.end(); ++it){
      int w = (*it).first;
      double x = (*it).second;
      events += w; a += w*x; b += w*x*x;
    }
    double xsec = a/(double)events;
    double RMS = (b*events - a*a)/((double)events*events);

    hist[sample]->Scale(1./events*xsec);
    std::cout << events << " events in " << sample << "Zjj with xsec = " << xsec << " +/- " << sqrt(RMS) << " pb" << std::endl;
  }

  TCanvas *c = new TCanvas("Canvas", "Canvas", 1);
  TPad *padUp = new TPad("padUP","up",   0, 0.3, 1., 1.,   0); padUp->Draw(); padUp->SetLeftMargin(0.1);
  TPad *padDn = new TPad("padDN","down", 0, 0, 1., 0.3, 0); padDn->Draw(); padDn->SetLeftMargin(0.1);
  padUp->cd();
  hist["all"]->SetLineColor(kViolet);
  hist["all"]->SetStats(0);
  hist["all"]->SetTitle("Interference between QCD and EWK Zjj");
  hist["all"]->GetXaxis()->SetTitle("M_{jj}");
  hist["all"]->GetYaxis()->SetTitle("normalized events");
  hist["all"]->GetYaxis()->SetTitleOffset(1.5);
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
  padDn->cd();
  hist["diff"] = (TH1D*) hist["all"]->Clone();
  hist["diff"]->SetLineColor(kBlack);
  hist["diff"]->Add(hist["qcd"], -1);
  hist["diff"]->Divide(hist["ewk"]);
  hist["diff"]->GetYaxis()->SetTitle("#sigma_{ewk+int}/#sigma_{ewk}");
  hist["diff"]->GetYaxis()->SetTitleSize(.15);
  hist["diff"]->GetYaxis()->SetTitleOffset(.3);
  hist["diff"]->SetMinimum(0.5);
  hist["diff"]->SetMaximum(1.8);
  hist["diff"]->DrawCopy();
  c->SaveAs("interference.pdf");
  return 0;
}
