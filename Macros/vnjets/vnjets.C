#include <TROOT.h>
#include <TFile.h>
#include <TTree.h>

int main(){
  for(TString fileName : {"ewkv_DY"}){
    TFile *file = new TFile("~/public/merged/EWKV/2013-06/" + fileName + ".root");
    TTree *tree = (TTree*) file->Get("EWKV");
    TString fileName0jets;
    if(fileName == "ewkv_DY") fileName0jets = "ewkv_DY0";
    if(fileName == "ewkv_WJets") fileName0jets = "ewkv_W0Jets";
    TFile *file0jets = new TFile("~/public/merged/EWKV/2013-06/" + fileName0jets + ".root","RECREATE");
    file0jets->cd();
    TTree *tree0jets = tree->CloneTree(0);

    int nParticleEntries;
    tree->SetBranchAddress("nParticleEntries", &nParticleEntries);

    for(int i = 0; i < tree->GetEntries(); ++i){
      tree->GetEntry(i);
      if(nParticleEntries == 5) tree0jets->Fill(); 
    }
  }
  return 0;
}
