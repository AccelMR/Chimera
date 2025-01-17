/************************************************************************/
/**
 * @file chLinearColor.cpp
 * @author AccelMR
 * @date 2022/09/21
 *   Linear color class that holds a color as 4 bytes normalized float RGBA
 */
 /************************************************************************/

 /************************************************************************/
 /*
  * Includes
  */                                                                     
 /************************************************************************/
#include "chLinearColor.h"

#include "chRandom.h"
#include "chColor.h"


namespace chEngineSDK{

static constexpr uint32 MAX_COLOR = 255;
static const Random RAND(MAX_COLOR * 558);//Random number taken from my...

const LinearColor LinearColor::Transparent = LinearColor(0.0f, 0.0f, 0.0f, 0.0f);
const LinearColor LinearColor::White = LinearColor(1.0f, 1.0f, 1.0f, 1.0f);
const LinearColor LinearColor::Gray = LinearColor(0.5f, 0.5f, 0.5f, 1.0f);
const LinearColor LinearColor::Black = LinearColor(0.0f, 0.0f, 0.0f, 1.0f);
const LinearColor LinearColor::Red = LinearColor(1.0f, 0.0f, 0.0f, 1.0f);
const LinearColor LinearColor::Green = LinearColor(0.0f, 1.0f, 0.0f, 1.0f);
const LinearColor LinearColor::Blue = LinearColor(0.0f, 0.0f, 1.0f, 1.0f);
const LinearColor LinearColor::Yellow = LinearColor(1.0f, 1.0f, 0.5f, 1.0f);
const LinearColor LinearColor::Cyan = LinearColor(0.0f, 1.0f, 1.0f, 1.0f);
const LinearColor LinearColor::Magenta = LinearColor(1.0f, 0.0f, 1.0f, 1.0f);

const LinearColor LinearColor::Light_Gray = LinearColor(0.878f, 0.878f, 0.878f);
const LinearColor LinearColor::Dark_Gray = LinearColor(0.25f, 0.25f, 0.25f);
const LinearColor LinearColor::Pink = LinearColor(1.0f, 0.37f, 0.8156f);
const LinearColor LinearColor::Purple = LinearColor(0.6274f, 0.1254f, 1.0f);
const LinearColor LinearColor::Light_Blue = LinearColor(0.313f, 0.8156f, 1.0f);
const LinearColor LinearColor::Yellow_Green = LinearColor(0.375f, 1.0f, 0.5f);
const LinearColor LinearColor::Orange = LinearColor(1.0f, 0.625f, 0.0627f);
const LinearColor LinearColor::Brown = LinearColor(0.625f, 0.5f, 0.375f);
const LinearColor LinearColor::Pale_Pink = LinearColor(0.8945f, 0.7265f, 0.9375f);


/*
*/
constexpr
LinearColor::LinearColor(const Color& color)
  : rgba{ static_cast<float>(color.r / 255.0f),
          static_cast<float>(color.g / 255.0f),
          static_cast<float>(color.b / 255.0f),
          static_cast<float>(color.a / 255.0f) }
{}

/*
*/
LinearColor
LinearColor::makeRandomColor() {
  LinearColor ret;
  float factor = 1 / static_cast<float>(MAX_COLOR);
  ret.r = (RAND.getPseudoRandom() % MAX_COLOR) * factor;
  ret.g = (RAND.getPseudoRandom() % MAX_COLOR) * factor;
  ret.b = (RAND.getPseudoRandom() % MAX_COLOR) * factor;
  ret.a = 1.f;
  return ret;
}
}