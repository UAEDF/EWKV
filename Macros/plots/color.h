#ifndef COLOR_H
#define COLOR_H

#include <TColor.h>

// Get UA colors
void initColor(){
  TColor::GetColor("#003D64");
  TColor::GetColor("#60A6BF");
  TColor::GetColor("#A10040");
  TColor::GetColor("#CF678F");
  TColor::GetColor("#D79A46");
  TColor::GetColor("#EBCDA3");
  //Add here your color
}

enum uaColor{
  kUABlue =	924,
  kUABlue50 =	925,
  kUARed =	926,
  kUARed50 =	927,
  kUABrown =	928,
  kUABrown50 =	929
  //Add here your enum
};

#endif
