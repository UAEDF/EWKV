// @(#)root/tmva $Id: classification.C,v 1.3 2013/06/17 14:16:41 tomc Exp $
/**********************************************************************************
 * Project   : TMVA - a Root-integrated toolkit for multivariate data analysis    *
 * Package   : TMVA                                                               *
 * Root Macro: TMVAClassification                                                 *
 *                                                                                *
 * This macro provides examples for the training and testing of the               *
 * TMVA classifiers.                                                              *
 *                                                                                *
 * As input data is used a toy-MC sample consisting of four Gaussian-distributed  *
 * and linearly correlated input variables.                                       *
 *                                                                                *
 * The methods to be used can be switched on and off by means of booleans, or     *
 * via the prompt command, for example:                                           *
 *                                                                                *
 *    root -l TMVAClassification.C\(\"Fisher,Likelihood\"\)                       *
 *                                                                                *
 * (note that the backslashes are mandatory)                                      *
 * If no method given, a default set is used.                                     *
 *                                                                                *
 * The output file "TMVA.root" can be analysed with the use of dedicated          *
 * macros (simply say: root -l <macro.C>), which can be conveniently              *
 * invoked through a GUI that will appear at the end of the run of this macro.    *
 **********************************************************************************/

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
#include "../shellVariables.h"

TString type = "ZMUMU";
TString tag = "20130617e";
TString option = "_BDTD50k";
TString DYtype = "inclusive";
TString production = "2013-06-JetIDfix";
   
int main(){
   if(!exists(type)) system("mkdir " + type);
   if(!exists(type + "/" + tag + option)) system("mkdir " + type + "/" + tag + option);
   // Create a new root output file.
   TString outfileName( type + "/" + tag  + option + "/TMVA.root" );
   TFile* outputFile = TFile::Open( outfileName, "RECREATE" );
   (TMVA::gConfig().GetIONames()).fWeightFileDir = type + "/" + tag  + option + "/weights/";

   TMVA::Factory *factory = new TMVA::Factory( "TMVAClassification", outputFile, "!V:!Silent:Color:DrawProgressBar:Transformations=I;D;P;G,D" );

   factory->AddVariable( "pT_Z", 'F' );
   factory->AddVariable( "pT_j1", 'F' );
   factory->AddVariable( "pT_j2", 'F' );
   factory->AddVariable( "eta_Z", 'F' );
   factory->AddVariable( "dPhi_j1", 'F' );
   factory->AddVariable( "dPhi_j2", 'F' );
   factory->AddVariable( "dPhi_jj", 'F' );
   factory->AddVariable( "dEta_jj", 'F' );
   factory->AddVariable( "avEta_jj", 'F' );
   factory->AddVariable( "qgHIG13011_j1", 'F' );
   factory->AddVariable( "qgHIG13011_j2", 'F' );
   factory->AddVariable( "M_jj", 'F' );

   std::map<TString, TFile*> files;
   std::map<TString, TTree*> trees;
   files["signal"] = new TFile("~/public/merged/EWKV/" + production + "/tmva-input/" + type + "/" + tag + "/ZVBF.root");
   if(DYtype == "powheg"){
     if(type == "ZEE") files["DY"] = new TFile("~/public/merged/EWKV/" + production + "/tmva-input/" + type + "/" + tag + "/DYEE-powheg.root");
     if(type == "ZMUMU") files["DY"] = new TFile("~/public/merged/EWKV/" + production + "/tmva-input/" + type + "/" + tag + "/DYMUMU-powheg.root");
   } else {
     if(DYtype == "inclusive") files["DY"] = new TFile("~/public/merged/EWKV/" + production + "/tmva-input/" + type + "/" + tag + "/DY.root");
     else for(TString i : {"0","1","2","3","4"}) files["DY" + i] = new TFile("~/public/merged/EWKV/" + production + "/tmva-input/" + type + "/" + tag + "/DY" + i + ".root");
   }

   for(auto file = files.begin(); file != files.end(); ++file) trees[file->first] = (TTree*) file->second->Get("ewkv-TMVA-input");

   factory->AddSignalTree(trees["signal"]);
   if(DYtype == "powheg" || DYtype  == "inclusive") factory->AddBackgroundTree(trees["DY"]);
   else for(TString i : {"0","1","2","3","4"}) factory->AddBackgroundTree(trees["DY" + i]);
   
   factory->SetSignalWeightExpression("weight");
   factory->SetBackgroundWeightExpression("weight");

   factory->PrepareTrainingAndTestTree( "", "", "nTrain_Signal=50000:nTrain_Background=50000:SplitMode=Random:!V" );
   factory->BookMethod( TMVA::Types::kBDT, "BDTD", "!H:!V:NTrees=400:nEventsMin=400:MaxDepth=3:BoostType=AdaBoost:SeparationType=GiniIndex:nCuts=20:PruneMethod=NoPruning:VarTransform=Decorrelate" );
//   factory->BookMethod( TMVA::Types::kMLP, "MLP", "!H:!V:VarTransform=N:TestRate=3:UseRegulator" );
   

   factory->TrainAllMethods();
   factory->TestAllMethods();
   factory->EvaluateAllMethods();    

   outputFile->Close();

   std::cout << "==> Wrote root file: " << outputFile->GetName() << std::endl;
   std::cout << "==> TMVAClassification is done!" << std::endl;      

   delete factory;
   return 0;
}
