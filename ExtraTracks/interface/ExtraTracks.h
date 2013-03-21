/* ExtraTracks.h
 * Package:	EWKV/ExtraTracks
 * Author:	Paolo Azzurri
 * Update:	2013/03/19
 * Based on:	http://cmssw.cvs.cern.ch/cgi-bin/cmssw.cgi/UserCode/PaoloA/VBFZ/ExtraTracks/src/ExtraTracks.cc?view=markup
 *
 * Class to produce the extra track collection
 */

#include <memory>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"


class ExtraTracks : public edm::EDProducer {
   public:
      explicit ExtraTracks(const edm::ParameterSet&);
      ~ExtraTracks(){};
      static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

   private:
      virtual void produce(edm::Event&, const edm::EventSetup&);

      edm::InputTag pfJetsNoVInputTag;
      edm::InputTag pfLeptonsInputTag;
      edm::InputTag primaryVertexInputTag;
      edm::InputTag tracksInputTag;
};
