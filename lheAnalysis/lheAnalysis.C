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

// first argument: ewk, qcd or all
// second argument: nothing or mjj1000
int main(int argc, char* argv[]){
  TString mjjBin = "_cuts";
  if(argc > 2) mjjBin = "_mjj1000";
  std::map<TString, TH1D*> hist;
  for(TString sample : {(TString) argv[1]}){
    hist[sample] = new TH1D(sample, sample, 36, 100, 1000);
    hist[sample+"y*"] = new TH1D(sample + "y*", sample + "y*", 25, 0, 5);
    hist[sample + "1000"] = new TH1D(sample + "1000", sample + "1000", 100, 1000, 3500);
    hist[sample + "1000y*"] = new TH1D(sample + "1000y*", sample + "1000y*", 25, 0, 5);

    int accepted = 0, accepted1000 = 0, events1000 = 0;
    std::vector<std::pair<int, double>> eventsAndXsec;
    for(int i = 1; i <= (mjjBin == "_mjj1000"? (sample == "all"? 2000 : 1500) : 3000); ++i){
      std::ifstream lheInput;
      if(!getStream(lheInput, "/localgrid/tomc/" + sample + "Zjj" + mjjBin + "_gridpack/lhe/" + sample + "Zjj" + (mjjBin == "_cuts"?"_cuts":"") + "_gridpack_" + TString::Format("%04d", i) + ".lhe", true)) continue;
      std::stringstream line;

      int events = 0; double xsec = 0.;
      while(getLine(lheInput, line)){													//Read lhe line
        TString tag; line >> tag;
        if(tag == "<init>"){														//Init tag: cross section two lines further
          getLine(lheInput, line);getLine(lheInput, line);
          TString intWeight; line >> intWeight;
          xsec = intWeight.Atof();
        } else if(tag == "<event>"){													//Event found
          TLorentzVector jj; int jets = 0, leptons = 0; bool leadingJet = false, Zboson = false;
          double yZ, yjj = 0;
	  while(getLine(lheInput, line)){												//Read particles until </event> tag
            TString pid; line >> pid; 
            if(pid == "</event>"){
              ++events;
              if(jj.M() > 1000) ++events1000;
              if(jets == 2 && leptons == 2 && leadingJet && Zboson){
                double ystar = fabs(yZ - yjj/2);
                if(jj.M() < 1000){
		  hist[sample]->Fill(jj.M());										//Fill if the two jets were found
                  hist[sample + "y*"]->Fill(ystar);									//Fill if the two jets were found
                } else {
         	  hist[sample + "1000"]->Fill(jj.M());
		  hist[sample + "1000y*"]->Fill(ystar);
                  ++accepted1000;
                }
                ++accepted;
              }
              break;
            } 
            if(pid.Atoi() == 0){ std::cout<< "PID=0 in file " << i << ", event " << events << std::endl; break;}

            TString status, mother, mother2, color, color2, p1, p2, p3, p4, mass, lifetime, spin;
            line >> status >> mother >> mother2 >> color >> color2 >> p1 >> p2 >> p3 >> p4 >> mass >> lifetime >> spin;
            if(pid.Atoi() == 23 && fabs(mass.Atof() - 91.1876) < 20){ 
              Zboson = true;
              TLorentzVector Z = TLorentzVector(p1.Atof(), p2.Atof(), p3.Atof(), p4.Atof());
              yZ = Z.Rapidity();
            }
            if(fabs(pid.Atoi()) == 11 || fabs(pid.Atoi()) == 13 || fabs(pid.Atoi()) == 15){
              TLorentzVector l = TLorentzVector(p1.Atof(), p2.Atof(), p3.Atof(), p4.Atof());
              if(l.Pt() > 20 && fabs(l.Eta()) < 2.4) ++leptons;
            }
            if(status.Atoi() == 1 && (fabs(pid.Atoi()) < 6 || fabs(pid.Atoi()) == 21)){							//Find jet
              TLorentzVector j = TLorentzVector(p1.Atof(), p2.Atof(), p3.Atof(), p4.Atof());
              jj = TLorentzVector(jj + j);
              if((j.Pt() > 30) && fabs(j.Eta()) < 4.7) ++jets;
              else continue;
              yjj += j.Rapidity();
              if(j.Pt() > 50) leadingJet = true;
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

    std::cout << events << " events in " << sample << "Zjj with xsec = " << xsec << " +/- " << sqrt(RMS) << " pb" << std::endl;
    std::cout << accepted << " events in " << sample << "Zjj with xsec = " << xsec*((double)accepted/(double)events) << " +/- " << sqrt(RMS)*((double)accepted/(double)events) << " pb (accepted)" << std::endl;
    std::cout << events1000 << " events in " << sample << "Zjj (mjj > 1000 GeV) with xsec = " << xsec*((double)events1000/(double)events) << " +/- " << sqrt(RMS)*((double)events1000/(double)events) << " pb" << std::endl;
    std::cout << accepted1000 << " events in " << sample << "Zjj (mjj > 1000 GeV) with xsec = " << xsec*((double)accepted1000/(double)events) << " +/- " << sqrt(RMS)*((double)accepted1000/(double)events) << " pb (accepted)" << std::endl;

    TFile *file = new TFile(sample + mjjBin + ".root","RECREATE");
    hist[sample]->Write();
    hist[sample+"1000"]->Write();
    hist[sample+"y*"]->Write();
    hist[sample+"1000y*"]->Write();
    file->Close();
  }
  return 0;
}
