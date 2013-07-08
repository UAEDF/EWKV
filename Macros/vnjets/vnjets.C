// Filter DY or W + 0 jets out of inclusive sample
#include <TROOT.h>
#include <TFile.h>
#include <TTree.h>
#include <TH1I.h>
#include <iostream>
#include "../environment.h"

int main(){
  for(TString inclusiveSample : {"DY", "WJets"}){
    TString jets0Sample;
    if(inclusiveSample == "DY") jets0Sample = "DY0";
    if(inclusiveSample == "WJets") jets0Sample = "W0Jets";

    // pile-up
    std::cout << "Get pile-up distribution for " << jets0Sample << std::flush;
    if(!exists(getTreeLocation() + "pileUp_" + jets0Sample + ".root")){
      TFile *puFile = new TFile(getTreeLocation() + "pileUp_" + inclusiveSample + ".root");
      TFile *puFile0jets = new TFile(getTreeLocation() + "pileUp_" + jets0Sample + ".root","RECREATE");
      TH1I *puHisto = (TH1I*) puFile->Get("pileUp5");
      puHisto->SetName("pileUp");
      puFile0jets->cd();
      puFile0jets->WriteTObject(puHisto);
      puFile0jets->Close();
      std::cout << " --> DONE" << std::endl;
    } else std::cout << " --> Already exists" << std::endl;

    // tree
    std::cout << "Get " << jets0Sample << " tree" << std::flush;
    if(!exists(getTreeLocation() + "ewkv_" + jets0Sample + ".root")){
      TFile *file = new TFile(getTreeLocation() + "ewkv_" + inclusiveSample + ".root");
      TTree *tree = (TTree*) file->Get("EWKV");
      TFile *file0jets = new TFile(getTreeLocation() + "ewkv_" + jets0Sample + ".root","RECREATE");
      file0jets->cd();
      TTree *tree0jets = tree->CloneTree(0);

      int nParticleEntries;
      tree->SetBranchAddress("nParticleEntries", &nParticleEntries);

      for(int i = 0; i < tree->GetEntries(); ++i){
        tree->GetEntry(i);
        if(nParticleEntries == 5) tree0jets->Fill(); 
      }
      file0jets->Close();
      std::cout << " --> DONE" << std::endl;
    } else std::cout << " --> Already exists" << std::endl;
  }
  return 0;
}
