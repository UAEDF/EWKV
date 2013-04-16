#ifndef CUTFLOWHANDLER_H
#define CUTFLOWHANDLER_H

#include <vector>
#include <fstream>
#include <TString.h>
#include <limits>

#include "cutFlow.h"
#include "../samples/sample.h"

class cutFlowHandler{
  std::map<TString, cutFlow*> cutflowsMap;
  std::vector<cutFlow*> cutflows;

  public:
  cutFlowHandler(){};
  void add(cutFlow* myCutFlow){ cutflows.push_back(myCutFlow); cutflowsMap[myCutFlow->getName()] = myCutFlow;};

  void toLatex(TString fileName);
  bool merge(TString newName, std::vector<TString> mergeList);
};


void cutFlowHandler::toLatex(TString fileName){
  using namespace std;
  ofstream texstream;
  texstream.setf(ios::fixed);
  texstream.open(fileName);
  texstream << setprecision(0); double roundingUp = .5;

  //Header
  texstream << " \\begin{tabular}{| l |";
  for(auto cutflow : cutflows) texstream << " c ";
  texstream << "| c |}" << endl << "  \\hline" << endl;
  for(auto cutflow : cutflows) texstream << " & " << cutflow->getName();
  texstream << " & Total MC ($\\pm$ JES) \\\\" << endl << "  \\hline" << endl;

  double totalMC, totalMCplus, totalMCmin;
  auto trackPoints = cutflowsMap["signal"]->getTrackPoints();
  for(auto trackPoint = trackPoints.begin(); trackPoint != trackPoints.end(); ++trackPoint){
    bool errors = ((*(cutflows.begin()))->exist(*trackPoint, "JES+"));
    texstream << "  " << *trackPoint;

    int i = 0;
    totalMC = 0; totalMCplus = 0; totalMCmin = 0;

    for(auto cutflow : cutflows){
      double counts = cutflow->get(*trackPoint, "");
      double countsJESplus = cutflow->get(*trackPoint, "JES+");
      double countsJESmin = cutflow->get(*trackPoint, "JES-");

      if(cutflow->getName() == "data") texstream << " & " << (counts);
      else{
        if(counts < 10){ texstream << setprecision(1); roundingUp = 0.05;}
        texstream << " & " << counts; 
        if(errors && ((countsJESplus-counts) != 0 || (counts-countsJESmin) != 0)){
          texstream << "$^{ +" << (countsJESplus-counts+roundingUp) << "}_{ -" << (counts-countsJESmin+roundingUp) << "}$"; 
        }
        if(counts < 10){ texstream << setprecision(0); roundingUp = 0.5;}
        totalMC += counts;
        if(errors){
          totalMCplus += countsJESplus;
          totalMCmin += countsJESmin;
        }
      }
    }
    texstream << " & " << totalMC;
    if(errors) texstream << "$^{ +" << (totalMCplus-totalMC+roundingUp) << "}_{ -" << (totalMC-totalMCmin+roundingUp) << "}$";
    texstream << "\\\\" << endl;
  }

  texstream << "  \\hline" << endl;
  texstream << " \\end{tabular}" << endl;
  texstream.close();
}

bool cutFlowHandler::merge(TString newName, std::vector<TString> mergeList){
  cutflowsMap[newName] = new cutFlow(newName);
  auto trackPoints = cutflowsMap["signal"];
  for(auto trackPoint = trackPoints->begin(); trackPoint != trackPoints->end(); ++trackPoint){
    for(auto branch = trackPoint->second.begin(); branch != trackPoint->second.end(); ++branch){
      for(TString i : mergeList){
        auto cutflow = cutflowsMap.find(i);
        if(cutflow == cutflowsMap.end()) continue;
        cutflowsMap[newName]->add(trackPoint->first, branch->first, cutflow->second->get(trackPoint->first, branch->first));
      }
    }
  }
  auto cutflow = cutflows.begin();
  while(cutflow != cutflows.end()){
    if((*cutflow)->getName() == mergeList[0]){ 
      cutflows.insert(cutflow, cutflowsMap[newName]); ++cutflow;
      cutflows.erase(cutflow); --cutflow;
    } else {
      for(TString i : mergeList){
        if((*cutflow)->getName() == i){ cutflows.erase(cutflow); --cutflow;}
      }
    }
    ++cutflow;
  }
}
#endif
