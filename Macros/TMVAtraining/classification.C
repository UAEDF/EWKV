#include <cstdlib>
#include <iostream> 
#include <map>
#include <string>

#include "TChain.h"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "TObjString.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TPluginManager.h"
#include <map>

#include <TMVA/Factory.h>
#include <TMVA/Tools.h>
#include <TMVA/Config.h>
#include "../environment.h"

TString tag = "20131010_InclusiveDY";
TString option = "_BDT_zstar_noDPhis";
TString DYtype = "inclusive";
TString nTrain = "50000";
   
int main(int argc, char *argv[]){
  std::vector<TString> types {"ZEE","ZMUMU"};								//If no type given as option, run both
  if(argc > 1 && ((TString) argv[1]) == "ZEE") types = {"ZEE"};
  if(argc > 1 && ((TString) argv[1]) == "ZMUMU") types = {"ZMUMU"};

  for(TString type : types){
    std::cout << "TMVA classification for " << tag << option << " with " << type << std::endl;
         
    TString outputDir = getTreeLocation() + "tmvaWeights/" + type + "/" + tag + option + "/";
    makeDirectory(outputDir);
    TFile *outputFile = new TFile(outputDir + "TMVA.root" , "RECREATE" );
    (TMVA::gConfig().GetIONames()).fWeightFileDir = outputDir + "/weights/";

    TMVA::Factory *factory = new TMVA::Factory( "TMVAClassification", outputFile, "!V:!Silent:Color:DrawProgressBar:Transformations=I;D;P;G,D" );
    factory->AddVariable( "pT_Z", 'F' );
    factory->AddVariable( "pT_j1", 'F' );
    factory->AddVariable( "pT_j2", 'F' );
    factory->AddVariable( "pT_jj", 'F' );
    factory->AddVariable( "eta_Z", 'F' );
//    factory->AddVariable( "dPhi_j1", 'F' );
//    factory->AddVariable( "dPhi_j2", 'F' );
//    factory->AddVariable( "dPhi_jj", 'F' );
//    factory->AddVariable( "dEta_jj", 'F' );
    factory->AddVariable( "zstarZ", 'F' );
    factory->AddVariable( "avEta_jj", 'F' );
    factory->AddVariable( "qgHIG13011_j1", 'F' );
    factory->AddVariable( "qgHIG13011_j2", 'F' );
    factory->AddVariable( "M_jj", 'F' );

    TString treeDir = getTreeLocation() + "tmva-input/" + type + "/" + tag + "/";
    std::map<TString, TFile*> files;
    std::map<TString, TTree*> trees;
    files["signal"] = new TFile(treeDir + "ZVBF.root");
    if(DYtype == "powheg"){
      if(type == "ZEE") files["DY"] = new TFile(treeDir + "DYEE-powheg.root");
      if(type == "ZMUMU") files["DY"] = new TFile(treeDir + "DYMUMU-powheg.root");
    } else if(DYtype == "inclusive"){
      files["DY"] = new TFile(treeDir + "DY.root");
    } else if(DYtype == "data"){
      files["DY"] = new TFile(treeDir + "data.root");
    } else {
      for(TString i : {"2","3","4"}) files["DY" + i] = new TFile(treeDir + "DY" + i + ".root");
    }

    for(auto file = files.begin(); file != files.end(); ++file) trees[file->first] = (TTree*) file->second->Get("ewkv-TMVA-input");

    factory->AddSignalTree(trees["signal"]);
    if(DYtype == "powheg" || DYtype  == "inclusive" || DYtype == "data") factory->AddBackgroundTree(trees["DY"]);
    else for(TString i : {"2","3","4"}) factory->AddBackgroundTree(trees["DY" + i]);
   
    factory->SetSignalWeightExpression("weight");
    factory->SetBackgroundWeightExpression("weight");

    factory->PrepareTrainingAndTestTree( "", "", "nTrain_Signal=" + nTrain + ":nTrain_Background=" + nTrain + ":SplitMode=Random:!V" );
    factory->BookMethod( TMVA::Types::kBDT, "BDT", "!H:!V:NTrees=400:nEventsMin=400:MaxDepth=3:BoostType=AdaBoost:SeparationType=GiniIndex:nCuts=20:PruneMethod=NoPruning" );
   

    factory->TrainAllMethods();
    factory->TestAllMethods();
    factory->EvaluateAllMethods();    

    outputFile->Close();

    std::cout << "==> Wrote root file: " << outputFile->GetName() << std::endl;
    std::cout << "==> TMVAClassification is done!" << std::endl;      

    delete factory;
  }
  return 0;
}
