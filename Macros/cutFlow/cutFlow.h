/********************************************************************************************************
 * cutFlowTable.h:											*
 *													*
 * void init(sample, scalefactor)	start column with new sample					*
 *					the numbers in this column are scaled with the scalefactor	*
 * void track(trackPoint, errorSign=0)  count the number of events at this point			*
 *                                      use errorSign = +1 or -1 when errors are specified		*
 * void toLatex()			table is written into a tex-file				*
 ********************************************************************************************************/
#ifndef CUTFLOW_H
#define CUTFLOW_H

#include <vector>
#include <fstream>
#include <TString.h>
#include <limits>
#include "../samples/sample.h"

class cutFlow {
  private:
  double fillWeight;
  TString branch;
  sample* mySample;
  std::map<TString, std::map<TString, double>> counters;
  std::vector<TString> trackPointOrder;

  public:
  cutFlow(sample* mySample_);
  void track(TString trackPoint);
  void setFillWeight(double weight){fillWeight = weight;};
  void setBranch(TString branch_ = ""){branch = branch_;};
  std::vector<TString> getTrackPoints(){ return trackPointOrder;};
  bool exist(TString trackPoint, TString branch);
  bool exist(TString trackPoint);
  double get(TString trackPoint, TString branch = "");
};


cutFlow::cutFlow(sample* mySample_){
  mySample = mySample_;
  fillWeight = 0;
  branch = "";
}

bool cutFlow::exist(TString trackPoint){
  return counters.find(trackPoint) != counters.end();
}

bool cutFlow::exist(TString trackPoint, TString branch){
  if(!exist(trackPoint)) return false;
  auto branchCounters = counters.find(trackPoint)->second;
  return branchCounters.find(branch) != branchCounters.end();
}

void cutFlow::track(TString trackPoint){
  if(!exist(trackPoint)){
    trackPointOrder.push_back(trackPoint);
    counters[trackPoint][branch] = fillWeight;
    return;
  }
  if(!exist(trackPoint, branch)) counters[trackPoint][branch] = fillWeight;
  else counters[trackPoint][branch] += fillWeight;
  return;
}

double cutFlow::get(TString trackPoint, TString branch){ 
  if(!exist(trackPoint, branch)) return 0;
  else return counters[trackPoint][branch];
}


#endif
