#ifndef COLOR_H
#define COLOR_H

#include <map>
#include <TColor.h>

int getColor(TString color){
  std::map<TString, int> colors;
  colors["kWhite"] 	= 0;
  colors["kBlack"] 	= 1;
  colors["kGray"] 	= 920;
  colors["kRed"]   	= 632;
  colors["kGreen"] 	= 416;
  colors["kBlue"] 	= 600;
  colors["kYellow"]	= 400;
  colors["kMagenta"]	= 616;
  colors["kCyan"] 	= 432;
  colors["kOrange"]	= 800;
  colors["kSpring"]	= 820;
  colors["kTeal"]	= 840;
  colors["kAzure"]	= 860;
  colors["kViolet"]	= 880;
  colors["kPink"]	= 900;
  colors["kUABlue"]	= TColor::GetColor("#003D64");
  colors["kUABlue50"]	= TColor::GetColor("#60A6BF");
  colors["kUARed"]	= TColor::GetColor("#A10040");
  colors["kUARed50"]	= TColor::GetColor("#CF678F");
  colors["kUABrown"]	= TColor::GetColor("#D79A46");
  colors["kUABrown50"]	= TColor::GetColor("#EBCDA3");

  auto knownColor = colors.find(color);
  if(knownColor != colors.end()) return knownColor->second;
  if(color.IsDigit()) return color.Atoi();
  return 1;
}


#endif
