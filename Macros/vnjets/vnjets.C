#include <TROOT.h>
#include <TFile.h>
#include <TTree.h>
#include <TH1I.h>

int main(){
  for(TString inclusiveSample : {"DY"}){
    TString production = "2013-06-JetIDfix";
    TString jets0Sample;
    if(inclusiveSample == "DY") jets0Sample = "DY0";
    if(inclusiveSample == "WJets") jets0Sample = "W0Jets";

    TFile *puFile = new TFile("~/public/merged/EWKV/" + production + "/pileUp_" + inclusiveSample + ".root");
    TFile *puFile0jets = new TFile("~/public/merged/EWKV/" + production + "/pileUp_" + jets0Sample + ".root","RECREATE");
    TH1I *puHisto = (TH1I*) puFile->Get("pileUp5");
    puHisto->SetName("pileUp");

    puFile0jets->cd();
    puFile0jets->WriteTObject(puHisto);
    puFile0jets->Close();

    TFile *file = new TFile("~/public/merged/EWKV/" + production + "/ewkv_" + inclusiveSample + ".root");
    TTree *tree = (TTree*) file->Get("EWKV");
    TFile *file0jets = new TFile("~/public/merged/EWKV/" + production + "/ewkv_" + jets0Sample + ".root","RECREATE");
    file0jets->cd();
    TTree *tree0jets = tree->CloneTree(0);

    int nParticleEntries;
    tree->SetBranchAddress("nParticleEntries", &nParticleEntries);

    for(int i = 0; i < tree->GetEntries(); ++i){
      tree->GetEntry(i);
      if(nParticleEntries == 5) tree0jets->Fill(); 
    }
    file0jets->Close();
  }
  return 0;
}
