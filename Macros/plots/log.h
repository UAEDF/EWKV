#include "TMath.h"

void binLogX(TH1* h){
  TAxis *axis = h->GetXaxis();
  int bins = axis->GetNbins();
  Axis_t logMin = TMath::Log10(axis->GetXmin());
  Axis_t logMax = TMath::Log10(axis->GetXmax());
  Axis_t logWidth = (logMax-logMin)/bins;
  Axis_t *new_bins = new Axis_t[bins + 1];
  for(int i = 0; i <= bins; i++) new_bins[i] = TMath::Power(10, logMin + i*logWidth);
  axis->Set(bins, new_bins);
  axis->SetMoreLogLabels(); axis->SetNoExponent();
  delete[] new_bins;
}

