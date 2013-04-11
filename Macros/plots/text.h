#include "TLatex.h"

void drawText(){
  TLatex *tex = new TLatex(0.16,0.87,"#splitline{preliminary CMS #mu#mujj}{#sqrt{s} = 8 TeV, L = 5.3 fb^{ -1}}");
  tex->SetNDC();
  tex->SetTextFont(43);
  tex->SetTextSize(27);
  tex->SetLineWidth(2);
  tex->Draw();
}
