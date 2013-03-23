/* Analyzer.cc
 * Package:	EWKV/Analyzer
 * Author:	Tom Cornelis, Paolo Azzurri, Alex Van Spilbeeck
 * Update:	2013/03/22
 * Based on:	http://cmssw.cvs.cern.ch/cgi-bin/cmssw.cgi/UserCode/PaoloA/VBFZ/Analyzer/src/Analyzer.cc?view=markup
 * 
 * Analyzer class for the EWKV analysis
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

//Temp
#include "EGamma/EGammaAnalysisTools/interface/PFIsolationEstimator.h"
//End temp


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
    bool jetId(const reco::PFJet*);

    // Tempory to check different isolation methods
    bool muonSelection(const reco::MuonRef mu, edm::Handle<reco::VertexCollection> vtxs);
    bool electronSelection(const reco::GsfElectronRef e, edm::Handle<reco::PFCandidateCollection> pfCandidates, edm::Handle<reco::VertexCollection> vtxs, 
                                        edm::Handle<reco::ConversionCollection> conversions, edm::Handle<reco::BeamSpot> beamspot, edm::Handle<double> rhoIso);
    PFIsolationEstimator isolator;
    bool alternativeIsolation[2];
    // End tempory to check different isolation mehtods

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

    float met, metPhi, metSig; 

    int nJets;
    int ncJets[maxJet];
    bool jetID[maxJet];
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
    edm::InputTag metInputTag;
    edm::InputTag softTrackJetsInputTag;
    edm::InputTag rhoInputTag;
    edm::InputTag primaryVertexInputTag;
};
