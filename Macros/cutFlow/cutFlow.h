#ifndef CUTFLOW_H
#define CUTFLOW_H

#include <vector>
#include <fstream>
#include <TString.h>

class cutFlow {
  private:
  double fillWeight, eventWeight;
  TString branch;
  TString name;
  std::map<TString, std::map<TString, double>> counters;
  std::vector<TString> trackPointOrder;

  public:
  cutFlow(TString name_);
  void setEventWeight(double weight){		eventWeight = weight; fillWeight = weight;};
  void restoreEventWeight(){			fillWeight = eventWeight;};
  void multiplyEventWeight(double weight){	fillWeight *= weight;};
  void setBranch(TString branch_ = ""){		branch = branch_;};
  std::vector<TString> getTrackPoints(){ 	return trackPointOrder;};
  TString getName(){ 				return name;};

  void track(TString trackPoint);
  void add(TString trackPoint, TString branch_, double countsToAdd);
  bool exist(TString trackPoint, TString branch);
  bool exist(TString trackPoint);
  double get(TString trackPoint, TString branch = "");

  typedef std::map<TString, std::map<TString, double>>::iterator iterator;
  iterator begin(){ return counters.begin();};
  iterator end(){ return counters.end();};
};


cutFlow::cutFlow(TString name_){
  name = name_;
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


void cutFlow::add(TString trackPoint, TString branch_, double countsToAdd){
  if(!exist(trackPoint)){
    trackPointOrder.push_back(trackPoint);
    counters[trackPoint][branch_] = countsToAdd;
    return;
  }
  if(!exist(trackPoint, branch_)) counters[trackPoint][branch_] = countsToAdd;
  else counters[trackPoint][branch_] += countsToAdd;
  return;
}


double cutFlow::get(TString trackPoint, TString branch){ 
  if(!exist(trackPoint, branch)) return 0;
  else return counters[trackPoint][branch];
}


#endif
