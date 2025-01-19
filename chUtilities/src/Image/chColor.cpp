/************************************************************************/
/**
 * @file chColor.cpp
 * @author AccelMR
 * @date 2023/03/19
 *
 *    Color class that handles a color rgba 0-255 range.
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chColor.h"

#include "chRandom.h"
#include "chLinearColor.h"

namespace chEngineSDK{

static constexpr uint32 MAX_COLOR = 255;
static const Random RAND(MAX_COLOR * 551);//Random number taken from my...

// Color constant definitions
const Color Color::Transparent{ 0, 0, 0, 0 };
const Color Color::White{ 255, 255, 255 };
const Color Color::Gray{ 128, 128, 128 };
const Color Color::Black{ 0, 0, 0 };
const Color Color::Red{ 255, 0, 0 };
const Color Color::Green{ 0, 255, 0 };
const Color Color::Blue{ 0, 0, 255 };
const Color Color::Yellow{ 255, 255, 0 };
const Color Color::Cyan{ 0, 255, 255 };
const Color Color::Magenta{ 255, 0, 255 };
const Color Color::Light_Gray{ 222, 222, 222 };
const Color Color::Dark_Gray{ 64, 64, 64 };
const Color Color::Pink{ 255, 95, 209 };
const Color Color::Purple{ 160, 32, 255 };
const Color Color::Light_Blue{ 80, 209, 255 };
const Color Color::Yellow_Green{ 96, 255, 128 };
const Color Color::Orange{ 255, 160, 16 };
const Color Color::Brown{ 160, 128, 96 };
const Color Color::Pale_Pink{ 227, 186, 240 };

/*
*/
constexpr
Color::Color(const LinearColor& linearColor)
  : rgba{ static_cast<uint8>(linearColor.r * 255.0f),
          static_cast<uint8>(linearColor.g * 255.0f),
          static_cast<uint8>(linearColor.b * 255.0f),
          static_cast<uint8>(linearColor.a * 255.0f) }
{}

/*
*/
uint16
Color::to16Bit(bool bIsRGB565) const {
  if (bIsRGB565) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
  }
  else {
    return ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
  }
}

/*
*/
Color
Color::makeRandomColor() {
  Color ret;
  ret.r = (RAND.getPseudoRandom() % MAX_COLOR);
  ret.g = (RAND.getPseudoRandom() % MAX_COLOR);
  ret.b = (RAND.getPseudoRandom() % MAX_COLOR);
  ret.a = 1.f;
  return ret;
}

/*
*/
Color
Color::from16Bit(uint16 value, bool bIsRGB565) {
  Color color;
  const float RGB555 = 255.0f / 31.0f;
  const float RGB565 = 255.0f / 63.0f;

  if (bIsRGB565) {
    color.r = static_cast<uint8>(((value >> 11) & 0x1F) * RGB555);
    color.g = static_cast<uint8>(((value >> 5) & 0x3F) * RGB565);
    color.b = static_cast<uint8>((value & 0x1F) * RGB555);
  } else {
    color.r = static_cast<uint8>(((value >> 10) & 0x1F) * RGB555);
    color.g = static_cast<uint8>(((value >> 5) & 0x1F) * RGB555);
    color.b = static_cast<uint8>((value & 0x1F) * RGB555);
  }

  color.a = 255;
  return color;
}

} // namespace chEngineSDK


