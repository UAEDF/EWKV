#ifndef HIG13011_H
#define HIG13011_H

#include <memory>
#include <TROOT.h>
#include <TMVA/Reader.h>
#include <map>

#include "../environment.h"

class QGTaggerHIG13011{
   public:
      QGTaggerHIG13011();
      ~QGTaggerHIG13011(){};
      float getMVA(double, double, float, std::map<TString, float>);

   private:
      float interpolate(double, int, int, float&, float&);
      void setCorrections(TString regionAndPt, float corrAxis1, float corrAxis2, float corrMult, float corrJetR, float corrJetPull, float corrPtD);

      bool useProbValue;
      TMVA::Reader *reader;
      std::map<TString, float> corrections, mvaVariables_corr;
};


QGTaggerHIG13011::QGTaggerHIG13011(){
  useProbValue = false;

  //Initialize reader  
  reader = new TMVA::Reader("silent");
  for(TString variable :  {"axis1","axis2","Mult","JetR","JetPull"}) reader->AddVariable(variable, &mvaVariables_corr[variable]);

  TString rangePt[7] = {"30to50","50to80","80to120","120to170","170to300","300to470","470to600"};
  TString pt[7] = {"30","50","80","120","170","300","470"};
  TString region[3] = {"central","transition","forward"};
  for(int i = 0; i < 7; ++i){
    for(int j = 0; j < 3; ++j){
      reader->BookMVA("Likelihood"+pt[i]+region[j], getCMSSWBASE() + "src/EWKV/QGTaggerHIG13011/data/" + rangePt[i] + "_" + region[j] + "_Likelihood.xml");
    }
  }
  std::cout << "QGTaggerHIG13011:\t\tXML files are booked" << std::endl;

  // Set correction arrays, ordered as 	ax1, 		ax2, 		mult, 		R, 		Pull,		ptD
  setCorrections("30central",		0.000696, 	0.000642, 	0, 		-0.002013, 	0.000060,	-0.002029);
  setCorrections("30transition",	0.001469, 	0.001128, 	0.230114, 	-0.003204, 	0.000099,	-0.003301); 
  setCorrections("30forward",		0.000960, 	0.000985, 	0.228834, 	-0.003283, 	0.000063,	-0.003322); 

  setCorrections("50central", 		0.000446, 	0.000406, 	0, 		-0.001154, 	0.000033,	-0.001312);
  setCorrections("50transition",	0.001347, 	0.000964, 	0.196515, 	-0.002769, 	0.000100,	-0.002907); 
  setCorrections("50forward",		0.000845, 	0.000784, 	0.211113, 	-0.003340, 	0.000059,	-0.003351);

  setCorrections("80central",		0.000291, 	0.000264, 	0, 		-0.000713, 	0.000018,	-0.000919);
  setCorrections("80transition",	0.001045, 	0.000736, 	0.182445, 	-0.002352, 	0.000076,	-0.002531); 
  setCorrections("80forward",		0.000630, 	0.000590, 	0.201430, 	-0.003097, 	0.000043,	-0.003090); 

  setCorrections("120central",		0.000208, 	0.000184, 	0, 		-0.000486, 	0.000011,	-0.000667);
  setCorrections("120transition",	0.000754, 	0.000540, 	0.170863, 	-0.001904, 	0.000049,	-0.002085);
  setCorrections("120forward",		0.000470, 	0.000451, 	0.196330, 	-0.002270, 	0.000030,	-0.002766);

  setCorrections("170central",		0.000146, 	0.000127, 	0, 		-0.000337, 	0.000007,	-0.000476);
  setCorrections("170transition",	0.000541, 	0.000395, 	0.168255, 	-0.001517, 	0.000030,	-0.001685);
  setCorrections("170forward", 		0.000384, 	0.000361, 	0.188181, 	-0.002517, 	0.000020,	-0.002458);

  setCorrections("300central",		0.000091, 	0.000070, 	0., 		-0.000231, 	0.000004,	-0.000275);
  setCorrections("300tranistion",	0.000310, 	0.000243, 	0.170718, 	-0.001004, 	0.000014,	-0.001141);
  setCorrections("300forward",		0.000410, 	0.000226,	0.135866, 	-0.003550, 	-0.000097,	-0.003048);

  setCorrections("470central",		0.000062, 	0.000043, 	0., 		-0.000189, 	0.000003,	-0.000139);
  setCorrections("470tranistion",	0.000211, 	0.000163, 	0.182038, 	-0.000792, 	0.000009,	-0.000895);
  setCorrections("470forward",		-0.002664, 	-0.001128,	0.114548, 	0.005115, 	-0.000142,	0.000035);
}


float QGTaggerHIG13011::interpolate(double pt, int ptlow, int pthigh, float &mvalow, float &mvahigh){
  return (mvahigh-mvalow)/(pthigh-ptlow)*(pt-ptlow)+mvalow;
}


void QGTaggerHIG13011::setCorrections(TString regionAndPt, float corrAxis1, float corrAxis2, float corrMult, float corrJetR, float corrJetPull, float corrPtD){
  corrections[regionAndPt + "axis1"] = corrAxis1;
  corrections[regionAndPt + "axis2"] = corrAxis2;
  corrections[regionAndPt + "Mult"] = corrMult;
  corrections[regionAndPt + "JetR"] = corrJetR;
  corrections[regionAndPt + "JetPull"] = corrJetPull;
  corrections[regionAndPt + "PtD"] = corrPtD;
}


float QGTaggerHIG13011::getMVA(double eta, double pt, float rhokt6PFJets, std::map<TString, float> mvaVariables){
  TString region = "central";
  if(fabs(eta)>=2) region = "transition";
  if(fabs(eta)>=3) region = "forward";

  //Get pT bin
  int pTlow, pThigh;
  if(pt>=20 && pt<30){ pTlow = 30; pThigh = 30;}
  else if(pt>=30 && pt<50){ pTlow = 30; pThigh = 50;}
  else if(pt>=50 && pt<80){ pTlow = 50; pThigh = 80;}
  else if(pt>=80 && pt<120){ pTlow = 80; pThigh = 120;}
  else if(pt>=120 && pt<170){ pTlow = 120; pThigh = 170;}
  else if(pt>=170 && pt<300){ pTlow = 170; pThigh = 300;}
  else if(pt>=300 && pt<470){ pTlow = 300; pThigh = 470;}
  else{ pTlow = 470; pThigh = 470;}

  //Calculate (interpolated) mva value
  float mvaval;
  for(std::map<TString, float>::iterator it = mvaVariables_corr.begin(); it != mvaVariables_corr.end(); ++it){
    mvaVariables_corr[it->first] = mvaVariables[it->first] - corrections[TString::Format("%d",pTlow) + region + it->first]*rhokt6PFJets;
  }

  if(useProbValue) mvaval = reader->GetProba("Likelihood" + TString::Format("%d",pTlow) + region);
  else             mvaval = reader->EvaluateMVA("Likelihood" + TString::Format("%d",pTlow) + region);

  if(pTlow != pThigh){
    for(std::map<TString, float>::iterator it = mvaVariables_corr.begin(); it != mvaVariables_corr.end(); ++it){
      mvaVariables_corr[it->first] = mvaVariables[it->first] - corrections[TString::Format("%d",pThigh) + region + it->first]*rhokt6PFJets;
    }
    float mvaval_high;
    if(useProbValue) mvaval_high = reader->GetProba("Likelihood" + TString::Format("%d",pTlow) + region);
    else             mvaval_high = reader->EvaluateMVA("Likelihood" + TString::Format("%d",pThigh) + region);
    mvaval = interpolate(pt, pTlow, pThigh, mvaval, mvaval_high);
  }
  return mvaval; 
}

#endif
