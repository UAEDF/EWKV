#ifndef CUTFLOWHANDLER_H
#define CUTFLOWHANDLER_H

#include <vector>
#include <fstream>
#include <TString.h>
#include <limits>

#include "cutFlow.h"
#include "../samples/sample.h"

class cutFlowHandler{
  std::map<sample*, cutFlow*> cutflows;

  public:
  cutFlowHandler(){};
  void add(sample* mySample, cutFlow* myCutFlow){ cutflows[mySample] = myCutFlow;};

  void toLatex(TString fileName);
//void merge(sample* firstSample, sample* secondSample)
};


void cutFlowHandler::toLatex(TString fileName){
  using namespace std;
  ofstream texstream;
  texstream.setf(ios::fixed);
  texstream.open(fileName);
  texstream << setprecision(0); double roundingUp = .5;

  //Header
  texstream << " \\begin{tabular}{l|";
  for(int i=0; i < cutflows.size(); ++i){ texstream << " c ";}
  texstream << "| c}" << endl << "  \\hline" << endl;
  for(auto cutflow = cutflows.begin(); cutflow != cutflows.end(); ++cutflow){ texstream << " & " << cutflow->first->getName();}
  texstream << " & Total MC ($\\pm$ JES) \\\\" << endl << "  \\hline" << endl;

  double totalMC, totalMCplus, totalMCmin;
  auto trackPoints = cutflows.begin()->second->getTrackPoints();
  for(auto trackPoint = trackPoints.begin(); trackPoint != trackPoints.end(); ++trackPoint){
    bool errors = (cutflows.begin()->second->exist(*trackPoint, "JES+"));
    texstream << "  " << *trackPoint;

    int i = 0;
    totalMC = 0; totalMCplus = 0; totalMCmin = 0;

    for(auto cutflow = cutflows.begin(); cutflow != cutflows.end(); ++cutflow){
      double counts = cutflow->second->get(*trackPoint, "");
      double countsJESplus = cutflow->second->get(*trackPoint, "JES+");
      double countsJESmin = cutflow->second->get(*trackPoint, "JES-");

      if(cutflow->first->getName() == "data") texstream << " & " << counts;
      else{
        if(counts < 10){ texstream << setprecision(1); roundingUp = 0.05;}
        texstream << " & " << counts; 
        if(errors) texstream << "$^{ +" << (countsJESplus-counts+roundingUp) << "}_{ -" << counts-countsJESmin+roundingUp << "}$"; 
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
#endif
