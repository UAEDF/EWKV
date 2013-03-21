/* Analyzer.cc
 * Package:	EWKV/Analyzer
 * Author:	Tom Cornelis, Paolo Azzurri, Alex Van Spilbeeck
 * Update:	2013/03/20
 * Based on:	http://cmssw.cvs.cern.ch/cgi-bin/cmssw.cgi/UserCode/PaoloA/VBFZ/Analyzer/src/Analyzer.cc?view=markup
 * 
 * Analyzer class for the EWKV analysis
 *
 * TO DO:	- Update/clean up code in order to work in the EWKV framework
 *		- Implement jet ID
 */

#include <memory>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include <DataFormats/MuonReco/interface/Muon.h> 
#include "DataFormats/MuonReco/interface/MuonFwd.h"
#include "DataFormats/MuonReco/interface/MuonQuality.h"
#include "DataFormats/MuonReco/interface/MuonSelectors.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "DataFormats/JetReco/interface/GenJetCollection.h"
#include "DataFormats/JetReco/interface/PFJet.h"
#include "DataFormats/JetReco/interface/PFJetCollection.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "JetMETCorrections/Objects/interface/JetCorrector.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectionUncertainty.h"
#include "JetMETCorrections/Objects/interface/JetCorrectionsRecord.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectorParameters.h"
#include "DataFormats/JetReco/interface/TrackJetCollection.h"
#include "HLTrigger/HLTcore/interface/HLTConfigProvider.h"
#include "DataFormats/HLTReco/interface/HLTPrescaleTable.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include "FWCore/Common/interface/TriggerNames.h"

#include "TH1.h"
#include "TFile.h"
#include "TTree.h"
#include "TLorentzVector.h"
#include "TClonesArray.h"


const int maxGen = 10;
const int maxJet = 10;
const int maxSTJ = 10;
const int maxTrg = 10;

class Analyzer : public edm::EDAnalyzer{
  public:
    explicit Analyzer(const edm::ParameterSet&);
    ~Analyzer(){};
    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
  
  private:
    virtual void beginJob() ;
    virtual void analyze(const edm::Event&, const edm::EventSetup&);
    virtual void endJob() ;
    virtual void beginRun(edm::Run const&, edm::EventSetup const&);

    TFile *f_Analyzer; 
    TTree *t_Analyzer; 
    TString fileName;

    int nEvent,nRun,nLumi;
    int nPileUp, nPriVtxs;
    float rhokt6PFJets;

    int nGenPart;
    int idGenPart[maxGen];
    TClonesArray *vGenPart; 

    int vType, nLeptons;
    int leptonCharge[2];
    TClonesArray *vLeptons; 

    int nJets;
    int ncJets[maxJet];
    double jetUncertainty[maxJet];
    float jetSmearedPt[maxJet], genJetPt[maxJet], jetQGMLP[maxJet], jetQGLikelihood[maxJet];
    TClonesArray *vJets; 

    int nSoftTrackJets;
    TClonesArray *vSoftTrackJets; 

    std::vector<std::string> HLT_paths;
    std::string HLT_process;
    int trigRes[maxTrg];
    double trigPres[maxTrg];
    bool prescalersOK;
    HLTConfigProvider hltConfig_;

    edm::InputTag genJetsInputTag;
    edm::InputTag pfJetsNoVInputTag;
    edm::InputTag pfLeptonsInputTag;
    edm::InputTag softTrackJetsInputTag;
    edm::InputTag rhoInputTag;
    edm::InputTag primaryVertexInputTag;
};
