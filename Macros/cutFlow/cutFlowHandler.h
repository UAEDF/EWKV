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
  bool hasTotalMC;

  public:
  cutFlowHandler(){ hasTotalMC = false;};
  void add(cutFlow* myCutFlow){ cutflows.push_back(myCutFlow); cutflowsMap[myCutFlow->getName()] = myCutFlow;};

  void toLatex(TString fileName);
  bool merge(TString newName, std::vector<TString> mergeList, bool replace = true);
};


void cutFlowHandler::toLatex(TString fileName){
  using namespace std;
  ofstream texstream;
  texstream.setf(ios::fixed);
  texstream.open(fileName);
  texstream << setprecision(0); double roundingUp = .5;

  if(!hasTotalMC){
    std::vector<TString> mcList;
    for(auto cutflow : cutflows) if(cutflow->getName() != "data") mcList.push_back(cutflow->getName());
    merge("Total MC", mcList, false);
    hasTotalMC = true;
  }

  //Header
  texstream << " \\begin{tabular}{| l |";
  for(auto cutflow : cutflows) texstream << " c ";
  texstream << "|}" << endl << "  \\hline" << endl;
  for(auto cutflow : cutflows) texstream << " & " << cutflow->getName();
  texstream << "\\\\" << endl << "  \\hline" << endl;

  auto trackPoints = cutflowsMap["ZVBF"]->getTrackPoints();
  for(auto trackPoint = trackPoints.begin(); trackPoint != trackPoints.end(); ++trackPoint){
    texstream << "  " << *trackPoint;

    for(auto cutflow : cutflows){
      bool errors = (cutflow->exist(*trackPoint, "JESUp"));
      double counts = cutflow->get(*trackPoint, "");
      double countsJESplus = cutflow->get(*trackPoint, "JESUp");
      double countsJESmin = cutflow->get(*trackPoint, "JESDown");

      if(cutflow->getName() == "data") texstream << " & " << (counts);
      else {
        if(counts < 10){ texstream << setprecision(1); roundingUp = 0.05;}
        texstream << " & " << counts; 
        if(errors && ((countsJESplus-counts) != 0 || (counts-countsJESmin) != 0)){
          texstream << "$^{ +" << (countsJESplus-counts+roundingUp) << "}_{ -" << (counts-countsJESmin+roundingUp) << "}$"; 
        }
        if(counts < 10){ texstream << setprecision(0); roundingUp = 0.5;}
      }
    }
    texstream << "\\\\" << endl;
  }

  texstream << "  \\hline" << endl;
  texstream << " \\end{tabular}" << endl;
  texstream.close();
}

bool cutFlowHandler::merge(TString newName, std::vector<TString> mergeList, bool replace){
  cutflowsMap[newName] = new cutFlow(newName);
  auto trackPoints = cutflowsMap["ZVBF"];
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
  while((cutflow != cutflows.end()) && (std::find(mergeList.begin(), mergeList.end(), (*cutflow)->getName()) == mergeList.end())) ++cutflow;
  if(cutflow != cutflows.end()) cutflows.insert(cutflow, cutflowsMap[newName]);
  if(replace){
    for(TString i : mergeList){
      cutflow = cutflows.begin();
      while((cutflow != cutflows.end()) && ((*cutflow)->getName() != i)) ++cutflow;
      if(cutflow != cutflows.end()) cutflows.erase(cutflow);
    }
  }
}
#endif
