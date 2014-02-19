// argv[1]: type selection (ZEE,ZMUMU or ALL)
// argv[2]: variation 
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

#include <TMVA/Factory.h>
#include <TMVA/Tools.h>
#include <TMVA/Config.h>
#include "../environment.h"

TString tag = "20140121_InclusiveDY";			// Trees used for training
TString DYtype = "inclusive";				// Inclusive or composed DY
TString mva = "BDT";					// Choosen MVA
//TCut mjjCut = "M_jj>100";				// Mjj cut
TCut mjjCut = "";
   
int main(int argc, char *argv[]){
  for(TString type : typeSelector(argc, argv)){
    std::cout << "TMVA classification for " << (tag + "_" + mva) << " with " << type << std::endl;
    TString variation = "";
    if(argc > 2) variation = TString(argv[2]);

    //Initialization
    TString nTrainS = "40000", nTrainB = (type == "ZEE"? "70000" : "110000");
    TString outputDir = getTreeLocation() + "tmvaWeights/" + type + "/" + tag + "_" + mva + (variation == ""? "" : ("_" + variation)) + "/";
    makeDirectory(outputDir);
    TFile *outputFile = new TFile(outputDir + "TMVA.root" , "RECREATE" );
    (TMVA::gConfig().GetIONames()).fWeightFileDir = outputDir + "/weights/";
    TMVA::Factory *factory = new TMVA::Factory( "TMVAClassification", outputFile, "!V:!Silent:Color:DrawProgressBar:Transformations=I;D;P;G,D" );

    //Variable configuration
    factory->AddVariable( "pT_Z", 'F' );
    if(variation == "13var") factory->AddVariable( "pT_j1", 'F' );
    if(variation == "13var") factory->AddVariable( "pT_j2", 'F' );
    factory->AddVariable( "abs(eta_Z)", 'F' );
    if(variation == "13var") factory->AddVariable( "dPhi_j1", 'F' );
    if(variation == "13var") factory->AddVariable( "dPhi_j2", 'F' );
    if(variation == "13var") factory->AddVariable( "dPhi_jj", 'F' );
    if(variation == "13var") factory->AddVariable( "dEta_jj", 'F' );
    factory->AddVariable( "zstarZ", 'F' );
    factory->AddVariable( "avEta_jj", 'F' );
    factory->AddVariable( "qgHIG13011_j1", 'F' );
    factory->AddVariable( "qgHIG13011_j2", 'F' );
    factory->AddVariable( "M_jj", 'F' );

    // Init trees
    TString treeDir = getTreeLocation() + "tmva-input/" + type + "/" + tag + "/";
    std::map<TString, TFile*> files;
    std::map<TString, TTree*> trees;
    files["signal"] = new TFile(treeDir + "EWKZ.root");
    if(DYtype == "inclusive") 	files["DY"] = new TFile(treeDir + "DY.root");
    else if(DYtype == "data")   files["DY"] = new TFile(treeDir + "data.root");
    else for(TString i : {"2","3","4"}) files["DY" + i] = new TFile(treeDir + "DY" + i + ".root");

    for(auto file = files.begin(); file != files.end(); ++file) trees[file->first] = (TTree*) file->second->Get("ewkv-TMVA-input");

    factory->AddSignalTree(trees["signal"]);
    if(DYtype != "composed") factory->AddBackgroundTree(trees["DY"]);
    else for(TString i : {"2","3","4"}) factory->AddBackgroundTree(trees["DY" + i]);
   
    factory->SetSignalWeightExpression("weight");
    factory->SetBackgroundWeightExpression("weight");

    factory->PrepareTrainingAndTestTree( mjjCut, mjjCut, "nTrain_Signal=" + nTrainS + ":nTrain_Background=" + nTrainB + ":SplitMode=Random:!V" );

    if(mva == "BDT") factory->BookMethod( TMVA::Types::kBDT, "BDT", "!H:!V:NTrees=400:nEventsMin=400:MaxDepth=3:BoostType=AdaBoost:SeparationType=GiniIndex:nCuts=20:PruneMethod=NoPruning" );
    else{ std::cout << "classification.C:\t!!!\tNo method specified for " << mva << std::endl; exit(1);}
   
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
