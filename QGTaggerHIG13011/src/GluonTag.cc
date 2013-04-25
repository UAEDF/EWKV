// system include files
#include <memory>
#include <TROOT.h>
#include <TMVA/Reader.h>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/JetReco/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/Jet.h"

using namespace std;
using namespace reco;
using namespace edm;

class QGTaggerHIG13011 : public EDProducer {
   public:
      explicit QGTaggerHIG13011(const ParameterSet&);
      ~QGTaggerHIG13011(){};
      static void fillDescriptions(ConfigurationDescriptions& descriptions);

   private:
      virtual void beginJob(){};
      virtual void endJob(){};
      virtual void beginRun(Run&, EventSetup const&){};
      virtual void endRun(Run&, EventSetup const&){};
      virtual void beginLuminosityBlock(LuminosityBlock&, EventSetup const&){};
      virtual void endLuminosityBlock(LuminosityBlock&, EventSetup const&){};

      virtual void produce(Event&, const EventSetup&);
      float getMVA(TString, double);
      float interpolate(double, int, int, float&, float&);
      void setCorrections(TString regionAndPt, float corrAxis1, float corrAxis2, float corrMult, float corrJetR, float corrJetPull, float corrPtD);
      void calcVariables(TString region, PFJetCollection::const_iterator jet, Handle<VertexCollection> vC);

      // ----------member data ---------------------------
      InputTag src;
      string mva, xmldir;  
      bool useProbValue;
      TMVA::Reader *reader;
      map<TString, float> corrections, mvaVariables, mvaVariables_corr;
      float rhokt6PFJets;
};


QGTaggerHIG13011::QGTaggerHIG13011(const ParameterSet& iConfig) :
  src            ( iConfig.getUntrackedParameter<edm::InputTag>("src")),
  mva            ( iConfig.getUntrackedParameter<string>("mva","Likelihood")),
  xmldir         ( iConfig.getUntrackedParameter<string>("xmldir","QGTaggerHIG13011/data/")), 
  useProbValue   ( iConfig.getUntrackedParameter<bool>("useProbValue", false)) 
{
  for(TString product : {"qg","axis1", "axis2","pull","R","mult"}) produces<edm::ValueMap<float>>((product + "HIG13011").Data());

  //Initialize reader  
  reader = new TMVA::Reader("");
  for(TString variable :  {"axis1","axis2","Mult","JetR","JetPull"}) reader->AddVariable(variable, &mvaVariables_corr[variable]);

  TString rangePt[7] = {"30to50","50to80","80to120","120to170","170to300","300to470","470to600"};
  TString pt[7] = {"30","50","80","120","170","300","470"};
  TString region[3] = {"central","transition","forward"};
  for(int i = 0; i < 7; ++i){
    for(int j = 0; j < 3; ++j){
      reader->BookMVA(TString(mva)+pt[i]+region[j], FileInPath(TString(xmldir) + rangePt[i] + "_" + region[j] + "_" + TString(mva) + ".xml").fullPath());
    }
  }
  cout << "XML files are booked" << endl;

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


void QGTaggerHIG13011::produce(Event& iEvent, const EventSetup& iSetup){
  std::map<TString, std::vector<float>*> products;
  for(TString product : {"qg","axis1", "axis2","mult","pull","R"}) products[product + "HIG13011"] = new std::vector<float>;

  //Get rhokt6PFJets and primary vertex
  Handle<double> rho;
  iEvent.getByLabel("kt6PFJets","rho",rho);
  rhokt6PFJets = *rho;

  Handle<VertexCollection> vC;
  iEvent.getByLabel("offlinePrimaryVertices", vC);

  //Loop over jets with pt > 20
  Handle<PFJetCollection> pfJets;
  iEvent.getByLabel(src, pfJets);
  for(PFJetCollection::const_iterator jet = pfJets->begin(); jet != pfJets->end(); ++jet){
    if(fabs(jet->eta()) < 4.7 && jet->pt() > 20){
      TString region = "central";
      if(fabs(jet->eta())>=2) region = "transition";
      if(fabs(jet->eta())>=3) region = "forward";
      calcVariables(region, jet, vC);
      products["qgHIG13011"]->push_back(getMVA(region, jet->pt()));
      for(TString product : {"axis1","axis2","mult","pull","R"}) products[product + "HIG13011"]->push_back(mvaVariables[product]);
    } else {
      for(TString product : {"qg","axis1", "axis2","mult","pull","R"}) products[product + "HIG13011"]->push_back(-999);
    }
  }

  for(std::map<TString, std::vector<float>*>::iterator product = products.begin(); product != products.end(); ++product){
    std::auto_ptr<edm::ValueMap<float>> out(new edm::ValueMap<float>());
    edm::ValueMap<float>::Filler filler(*out);
    filler.insert(pfJets, product->second->begin(), product->second->end());
    filler.fill();
    iEvent.put(out, (product->first).Data());
    delete product->second;
  }
}


void QGTaggerHIG13011::calcVariables(TString region, PFJetCollection::const_iterator jet, Handle<VertexCollection> vC){
      VertexCollection::const_iterator vtxLead = vC->begin();

      float sum_weight(0.), sum_deta(0.), sum_dphi(0.), sum_deta2(0.), sum_dphi2(0.), sum_detadphi(0.), sum_pt(0.);
      vector<float> jetPart_pt, jetPart_deta, jetPart_dphi; 
      
      float pTMax(0.), pTMaxChg_QC(0.);
      int nChg_QC = 0, nChg_ptCut = 0, nNeutral_ptCut = 0;

      //Loop over the jet constituents
      vector<PFCandidatePtr> constituents = jet->getPFConstituents();
      for(unsigned i = 0; i < constituents.size(); ++i){
	PFCandidatePtr part = jet->getPFConstituent(i);      
	if(!part.isNonnull()) continue;

	TrackRef itrk = part->trackRef();;
	if(part->pt() > pTMax) pTMax = part->pt();

	bool trkForAxis = false;
	if(itrk.isNonnull()){					//Track exists --> charged particle
	  if(part->pt() > 1.0) nChg_ptCut++;

	  //Search for closest vertex to track
	  VertexCollection::const_iterator vtxClose = vC->begin();
	  for(VertexCollection::const_iterator vtx = vC->begin(); vtx != vC->end(); ++vtx){
	    if(fabs(itrk->dz(vtx->position())) < fabs(itrk->dz(vtxClose->position()))) vtxClose = vtx;
	  }
	    
	  if(vtxClose == vtxLead){
	    float dz = itrk->dz(vtxClose->position());
	    float dz_sigma = sqrt(pow(itrk->dzError(),2) + pow(vtxClose->zError(),2));
	      
	    if(itrk->quality(TrackBase::qualityByName("highPurity")) && fabs(dz/dz_sigma) < 5.){
	      trkForAxis = true;
	      if(part->pt() > pTMaxChg_QC) pTMaxChg_QC = part->pt(); 
	      float d0 = itrk->dxy(vtxClose->position());
	      float d0_sigma = sqrt(pow(itrk->d0Error(),2) + pow(vtxClose->xError(),2) + pow(vtxClose->yError(),2));
	      if(fabs(d0/d0_sigma) < 5.) nChg_QC++;
	    }
	  }
	} else {							//No track --> neutral particle
	  if(part->pt() > 1.0) nNeutral_ptCut++;
	  trkForAxis = true;
	}
	  
	//------calculation of axis-----------
	float deta = part->eta() - jet->eta();
	float dphi = 2*atan(tan(((part->phi()-jet->phi()))/2));           
	float partPt = part->pt(); 
	float weight = partPt*partPt;

	if(region != "central" || trkForAxis){			//In central region, only use when trkForAxis
	  sum_weight += weight;
	  sum_pt += partPt;
	  sum_deta += deta*weight;                  
	  sum_dphi += dphi*weight;                                                                                             
	  sum_deta2 += deta*deta*weight;                    
	  sum_detadphi += deta*dphi*weight;                               
	  sum_dphi2 += dphi*dphi*weight;

	  jetPart_pt.push_back(partPt);				//Store for next loop
	  jetPart_deta.push_back(deta);
	  jetPart_dphi.push_back(dphi);
	}	
      }

      //Calculate axis and ptD
      float a(0.), b(0.), c(0.);
      float ave_deta(0.), ave_dphi(0.), ave_deta2(0.), ave_dphi2(0.);
      if(sum_weight > 0){
        mvaVariables["ptD"] = sqrt(sum_weight)/sum_pt;
	ave_deta = sum_deta/sum_weight;
	ave_dphi = sum_dphi/sum_weight;
	ave_deta2 = sum_deta2/sum_weight;
	ave_dphi2 = sum_dphi2/sum_weight;
	a = ave_deta2 - ave_deta*ave_deta;                          
	b = ave_dphi2 - ave_dphi*ave_dphi;                          
	c = -(sum_detadphi/sum_weight - ave_deta*ave_dphi);                
      } else mvaVariables["ptD"] = 0;
      float delta = sqrt(fabs((a-b)*(a-b)+4*c*c));
      if(a+b+delta > 0) mvaVariables["axis1"] = sqrt(0.5*(a+b+delta));
      else mvaVariables["axis1"] = 0.;
      if(a+b-delta > 0) mvaVariables["axis2"] = sqrt(0.5*(a+b-delta));
      else mvaVariables["axis2"] = 0.;
      

      //Loop again over the constituents to calculate jet pull
      float ddetaR_ave(0.), ddphiR_ave(0.);
      if(sum_weight > 0){
	float ddetaR_sum(0.), ddphiR_sum(0.);
	for(unsigned int i=0; i < jetPart_pt.size(); ++i){
	  float weight = jetPart_pt[i]*jetPart_pt[i];
	  float ddeta = jetPart_deta[i] - ave_deta;
	  float ddphi = 2*atan(tan((jetPart_dphi[i] - ave_dphi)/2.)) ;             
	  float ddR = sqrt(ddeta*ddeta + ddphi*ddphi);
	  ddetaR_sum += ddR*ddeta*weight;
	  ddphiR_sum += ddR*ddphi*weight;  
	} 
	ddetaR_ave = ddetaR_sum/sum_weight;
	ddphiR_ave = ddphiR_sum/sum_weight;
      } 
      mvaVariables["JetPull"]  = sqrt(ddetaR_ave*ddetaR_ave+ddphiR_ave*ddphiR_ave);
      mvaVariables["pull"]  = sqrt(ddetaR_ave*ddetaR_ave+ddphiR_ave*ddphiR_ave);

      if(region == "central"){
	mvaVariables["Mult"] = nChg_QC;
	mvaVariables["mult"] = nChg_QC;
	mvaVariables["JetR"] = pTMaxChg_QC/sum_pt;
	mvaVariables["R"] = pTMaxChg_QC/sum_pt;
      } else {
	mvaVariables["Mult"] = (nChg_ptCut + nNeutral_ptCut);
	mvaVariables["mult"] = (nChg_ptCut + nNeutral_ptCut);
	mvaVariables["JetR"] = pTMax/sum_pt;
	mvaVariables["R"] = pTMax/sum_pt;
      }
}

float QGTaggerHIG13011::getMVA(TString region, double pt){
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
  for(map<TString, float>::iterator it = mvaVariables_corr.begin(); it != mvaVariables_corr.end(); ++it){
    mvaVariables_corr[it->first] = mvaVariables[it->first] - corrections[TString::Format("%d",pTlow) + region + it->first]*rhokt6PFJets;
  }

  if(useProbValue) mvaval = reader->GetProba(TString(mva) + TString::Format("%d",pTlow) + region);
  else             mvaval = reader->EvaluateMVA(TString(mva) + TString::Format("%d",pTlow) + region);

  if(pTlow != pThigh){
    for(map<TString, float>::iterator it = mvaVariables_corr.begin(); it != mvaVariables_corr.end(); ++it){
      mvaVariables_corr[it->first] = mvaVariables[it->first] - corrections[TString::Format("%d",pThigh) + region + it->first]*rhokt6PFJets;
    }
    float mvaval_high;
    if(useProbValue) mvaval_high = reader->GetProba(TString(mva) + TString::Format("%d",pTlow) + region);
    else             mvaval_high = reader->EvaluateMVA(TString(mva) + TString::Format("%d",pThigh) + region);
    mvaval = interpolate(pt, pTlow, pThigh, mvaval, mvaval_high);
  }
  return mvaval; 
}


// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void QGTaggerHIG13011::fillDescriptions(ConfigurationDescriptions& descriptions){
  ParameterSetDescription desc;
  desc.addUntracked<edm::InputTag>("src");
  desc.addUntracked<string>("mva","Likelihood");
  desc.addUntracked<string>("xmldir","QGTaggerHIG13011/data/");
  desc.addUntracked<bool>("useProbValue", false);
  descriptions.add("QGTaggerHIG13011", desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(QGTaggerHIG13011);

