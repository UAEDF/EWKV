// Filter DY or W + 0 jets out of inclusive sample
#include <TROOT.h>
#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include <iostream>
#include "../environment.h"

int main(){
  for(TString sample : {"DY", "WJets"}){
    for(TString fileType : {"pileUp","ewkv"}){
      TString inclusiveSample = "", jets0Sample = "";
      if(sample == "DY"){
        inclusiveSample = 	getTreeLocation() + fileType + "_DY.root";
        jets0Sample = 		getTreeLocation() + fileType + "_DY0.root";
      } else if(sample == "WJets"){
        inclusiveSample = 	getTreeLocation() + fileType + "_WJets.root";
        jets0Sample = 		getTreeLocation() + fileType + "_W0Jets.root";
      } else exit(1);
      if(exists(jets0Sample)){ std::cout << "vnjets.C:\t\t!!!\t" << jets0Sample << " already exists" << std::endl; continue;}

      std::cout << "vnJets.C:\t\t\tCreating " << jets0Sample << std::endl;
      TFile *file = new TFile(inclusiveSample);
      TFile *file0jets = new TFile(jets0Sample,"RECREATE");

      if(fileType == "pileUp"){
        for(TString name : {"pileUp","true"}){
          TH1 *hist; file->GetObject(name + "5", hist);
          if(!hist) continue;
          hist->SetName(name);
          file0jets->cd();
          file0jets->WriteTObject(hist);
        }
      }

      TTree *tree; file->GetObject((fileType == "ewkv"?"EWKV":"pileUp"), tree);
      if(!tree) continue;
      file0jets->cd();
      TTree *tree0jets = tree->CloneTree(0);

      int nParticleEntries, nEvent;
      tree->SetBranchAddress("nParticleEntries", &nParticleEntries);
      tree->SetBranchAddress("event", &nEvent);
      for(int i = 0, j = 0; i < tree->GetEntries(); ++i){
        tree->GetEntry(i);
        if(nParticleEntries == 5) tree0jets->Fill(); 
      }
      tree0jets->AutoSave();
      file0jets->Close();
      file->Close();
    }
  }
  return 0;
}
