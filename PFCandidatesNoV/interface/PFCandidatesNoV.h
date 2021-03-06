/* PFCandidatesNoV.h
 * Package:	EWKV/PFCandidatesNoV
 * Author:	Tom Cornelis
 * Update:	2013/03/21
 * Based on:	http://cmssw.cvs.cern.ch/cgi-bin/cmssw.cgi/UserCode/PaoloA/VBFZ/PFCandidatesNoV/src/PFCandidatesNoV.cc?view=markup
 *
 * Class to select and extract the lepton(s) from Z or W from the PFCandidates collection
 */

#include <memory>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDFilter.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/MuonReco/interface/Muon.h" 
#include "DataFormats/EgammaCandidates/interface/GsfElectron.h"
#include "DataFormats/METReco/interface/PFMET.h"

#include "TH1.h"
#include "TTree.h"
#include "TFile.h"


class PFCandidatesNoV : public edm::EDFilter{
  public:
    explicit PFCandidatesNoV(const edm::ParameterSet&);
    ~PFCandidatesNoV(){};
    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
    enum VType { WMUNU, WENU, ZMUMU, ZEE, UNDEFINED};

  private:
    virtual void beginJob();
    virtual bool filter(edm::Event&, const edm::EventSetup&);
    virtual void endJob();

    bool TryW(const reco::PFCandidate *lepton, const reco::PFMET *met);
    bool TryZ(const reco::PFCandidate *lepton, std::vector<const reco::PFCandidate*> selectedLeptons); 

    bool muonSelection(const reco::MuonRef, edm::Handle<reco::VertexCollection>);
    bool muonSelectionVeto(const reco::MuonRef);
    bool electronSelection(const reco::GsfElectronRef, edm::Handle<reco::PFCandidateCollection> pfCandidates, edm::Handle<reco::VertexCollection>, 
                           edm::Handle<reco::ConversionCollection>, edm::Handle<reco::BeamSpot>);
    bool electronSelectionVeto(const reco::GsfElectronRef, edm::Handle<reco::PFCandidateCollection> pfCandidates, edm::Handle<reco::VertexCollection>, 
                               edm::Handle<reco::ConversionCollection>, edm::Handle<reco::BeamSpot>);
    void fillPU(edm::Event&);

    TString fileName;
    double dilepton_mass_min;
    double transverse_Wmass_min;
    double muon_pt_min;
    double veto_muon_pt_min;
    double muon_eta_max;
    double electron_pt_min;
    double veto_electron_pt_min;
    double electron_eta_max;

    const reco::PFCandidate *lepton1, *lepton2;

    TFile *f_pileUp; 
    TTree *t_pileUp;
    std::map<TString, TH1I*> h_pileUp;
    std::map<TString, TH1D*> h_true;
    int nPileUp, nParticleEntries;
    float nTrue;

    edm::InputTag               pfCandidatesInputTag;
    edm::InputTag               metInputTag;
    edm::InputTag               conversionsInputTag;
    edm::InputTag               beamSpotInputTag;
    edm::InputTag               primaryVertexInputTag;

    VType type;
};
