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
#include "chColor.h"
#include "chMath.h"
#include "chRandom.h"

namespace chEngineSDK {

// Predefined color constants
const LinearColor LinearColor::Transparent = LinearColor(0.0f, 0.0f, 0.0f, 0.0f);
const LinearColor LinearColor::White = LinearColor(1.0f, 1.0f, 1.0f, 1.0f);
const LinearColor LinearColor::Gray = LinearColor(0.5f, 0.5f, 0.5f, 1.0f);
const LinearColor LinearColor::Black = LinearColor(0.0f, 0.0f, 0.0f, 1.0f);
const LinearColor LinearColor::Red = LinearColor(1.0f, 0.0f, 0.0f, 1.0f);
const LinearColor LinearColor::Green = LinearColor(0.0f, 1.0f, 0.0f, 1.0f);
const LinearColor LinearColor::Blue = LinearColor(0.0f, 0.0f, 1.0f, 1.0f);
const LinearColor LinearColor::Yellow = LinearColor(1.0f, 1.0f, 0.0f, 1.0f);
const LinearColor LinearColor::Cyan = LinearColor(0.0f, 1.0f, 1.0f, 1.0f);
const LinearColor LinearColor::Magenta = LinearColor(1.0f, 0.0f, 1.0f, 1.0f);
const LinearColor LinearColor::Light_Gray = LinearColor(0.878f, 0.878f, 0.878f, 1.0f);
const LinearColor LinearColor::Dark_Gray = LinearColor(0.25f, 0.25f, 0.25f, 1.0f);
const LinearColor LinearColor::Pink = LinearColor(1.0f, 0.37f, 0.8156f, 1.0f);
const LinearColor LinearColor::Purple = LinearColor(0.6274f, 0.1254f, 1.0f, 1.0f);
const LinearColor LinearColor::Light_Blue = LinearColor(0.313f, 0.8156f, 1.0f, 1.0f);
const LinearColor LinearColor::Yellow_Green = LinearColor(0.375f, 1.0f, 0.5f, 1.0f);
const LinearColor LinearColor::Orange = LinearColor(1.0f, 0.625f, 0.0627f, 1.0f);
const LinearColor LinearColor::Brown = LinearColor(0.625f, 0.5f, 0.375f, 1.0f);
const LinearColor LinearColor::Pale_Pink = LinearColor(0.8945f, 0.7265f, 0.9375f, 1.0f);

/*
 * Constructor from 8-bit per channel Color
 */
constexpr LinearColor::LinearColor(const Color& color)
 : r(static_cast<float>(color.r) / 255.0f),
   g(static_cast<float>(color.g) / 255.0f),
   b(static_cast<float>(color.b) / 255.0f),
   a(static_cast<float>(color.a) / 255.0f) {}

/*
*/
Color
LinearColor::toColor() const {
  return Color(static_cast<uint8>(Math::clamp(r * 255.0f, 0.0f, 255.0f)),
               static_cast<uint8>(Math::clamp(g * 255.0f, 0.0f, 255.0f)),
               static_cast<uint8>(Math::clamp(b * 255.0f, 0.0f, 255.0f)),
               static_cast<uint8>(Math::clamp(a * 255.0f, 0.0f, 255.0f)));
}

/*
 * Create a random color
 */
LinearColor
LinearColor::makeRandomColor() {
  static Random randomGenerator(0x8BC987C1); // Arbitrary seed value

  return LinearColor(randomGenerator.getRandom01(), randomGenerator.getRandom01(),
                     randomGenerator.getRandom01(), 1.0f);
}

/*
 * Convert from HSV to RGB
 */
LinearColor
LinearColor::fromHSV(float h, float s, float v, float a) {
  // Ensure hue is in the valid range [0, 360)
  h = Math::fmod(h, 360.0f);
  if (h < 0.0f) {
    h += 360.0f;
  }

  // Clamp saturation and value to [0, 1]
  s = Math::clamp(s, 0.0f, 1.0f);
  v = Math::clamp(v, 0.0f, 1.0f);

  // HSV to RGB conversion algorithm
  const float c = v * s;
  const float x = c * (1.0f - Math::abs(Math::fmod(h / 60.0f, 2.0f) - 1.0f));
  const float m = v - c;

  float r = 0.0f, g = 0.0f, b = 0.0f;

  if (h < 60.0f) {
    r = c;
    g = x;
    b = 0.0f;
  }
  else if (h < 120.0f) {
    r = x;
    g = c;
    b = 0.0f;
  }
  else if (h < 180.0f) {
    r = 0.0f;
    g = c;
    b = x;
  }
  else if (h < 240.0f) {
    r = 0.0f;
    g = x;
    b = c;
  }
  else if (h < 300.0f) {
    r = x;
    g = 0.0f;
    b = c;
  }
  else {
    r = c;
    g = 0.0f;
    b = x;
  }

  return LinearColor(r + m, g + m, b + m, a);
}

} // namespace chEngineSDK
