#include "TLatex.h"

void drawText(TString type){
  TLatex *tex;
  if(type == "ZMUMU") tex = new TLatex(0.16,0.87,"#splitline{preliminary CMS #mu#mujj}{#sqrt{s} = 8 TeV, L = 19.6 fb^{ -1}}");
  if(type == "ZEE")   tex = new TLatex(0.16,0.87,"#splitline{preliminary CMS eejj}{#sqrt{s} = 8 TeV, L = 19.6 fb^{ -1}}");
  tex->SetNDC();
  tex->SetTextFont(43);
  tex->SetTextSize(27);
  tex->SetLineWidth(2);
  tex->Draw();
}
