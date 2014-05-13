#include "TLatex.h"

void drawText(TString type){
  TLatex *tex = new TLatex(0.12,0.955,"#bf{CMS, #sqrt{s}=8 TeV, #scale[0.5]{#int}L=19.7 fb^{-1}}");
  tex->SetNDC();
  tex->SetTextFont(43);
  tex->SetTextSize(20);
  tex->SetLineWidth(2);
  tex->Draw();

  TLatex *tex2 = new TLatex(0.995,0.955,(type == "both"? TString("$ll$"):(type == "ZMUMU"?TString("#mu#mu"):TString("ee"))) + TString(" events"));
  tex2->SetNDC();
  tex2->SetTextFont(43);
  tex2->SetTextSize(20);
  tex2->SetTextAlign(31);
  tex2->Draw();
}
