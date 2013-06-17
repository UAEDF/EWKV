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
  auto trackPoints = cutflowsMap["ZVBF"]->getTrackPoints();
  for(auto trackPoint = trackPoints.begin(); trackPoint != trackPoints.end(); ++trackPoint){
    bool errors = ((*(cutflows.begin()))->exist(*trackPoint, "JES+"));
    texstream << "  " << *trackPoint;

    int i = 0;
    totalMC = 0; totalMCplus = 0; totalMCmin = 0;

    double signal, signalPlus, signalMin, data;
    for(auto cutflow : cutflows){
      double counts = cutflow->get(*trackPoint, "");
      double countsJESplus = cutflow->get(*trackPoint, "JES+");
      double countsJESmin = cutflow->get(*trackPoint, "JES-");

      if(cutflow->getName() == "data"){
        texstream << " & " << (counts);
        data = counts;
      } else{
        if(cutflow->getName() == "ZVBF"){
          signal = counts;
          signalPlus = countsJESplus;
          signalMin = countsJESmin;
        }
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

    if(errors){
      //Calculate K_s
      double background = totalMC - signal;
      double backgroundPlus = totalMCplus - signalPlus;
      double backgroundMin = totalMCmin - signalMin;
      double K_s = (data - background)/signal;
      double K_s_statErr = sqrt(data)/signal;
      double K_s_jesErrPlus = (data - backgroundPlus)/signalPlus - K_s;
      double K_s_jesErrMin = K_s - (data - backgroundMin)/signalMin;
      cout << *trackPoint << "\t" << K_s << "\\pm " << K_s_statErr << "^{ +" << K_s_jesErrPlus << "}_{ -" << K_s_jesErrMin << "}$ \\\\" << endl;
    }
  }

  texstream << "  \\hline" << endl;
  texstream << " \\end{tabular}" << endl;
  texstream.close();
}

bool cutFlowHandler::merge(TString newName, std::vector<TString> mergeList){
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
  while((*cutflow)->getName() != mergeList[0]) ++cutflow;
  cutflows.insert(cutflow, cutflowsMap[newName]);
  for(TString i : mergeList){
    cutflow = cutflows.begin();
    while((*cutflow)->getName() != i) ++cutflow;
    cutflows.erase(cutflow);
  }
}
#endif
