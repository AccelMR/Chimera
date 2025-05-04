/************************************************************************/
/**
 * @file chColor.cpp
 * @author AccelMR
 * @date 2023/03/19
 *
 *    Color class that handles a color rgba 0-255 range.
 */
/************************************************************************/
#include "chColor.h"

#include "chLinearColor.h"
#include "chMath.h"
#include "chRandom.h"

namespace chEngineSDK {

// Predefined color constants
const Color Color::Transparent = Color(0, 0, 0, 0);
const Color Color::White = Color(255, 255, 255);
const Color Color::Gray = Color(128, 128, 128);
const Color Color::Black = Color(0, 0, 0);
const Color Color::Red = Color(255, 0, 0);
const Color Color::Green = Color(0, 255, 0);
const Color Color::Blue = Color(0, 0, 255);
const Color Color::Yellow = Color(255, 255, 0);
const Color Color::Cyan = Color(0, 255, 255);
const Color Color::Magenta = Color(255, 0, 255);
const Color Color::Light_Gray = Color(222, 222, 222);
const Color Color::Dark_Gray = Color(64, 64, 64);
const Color Color::Pink = Color(255, 95, 209);
const Color Color::Purple = Color(160, 32, 255);
const Color Color::Light_Blue = Color(80, 209, 255);
const Color Color::Yellow_Green = Color(96, 255, 128);
const Color Color::Orange = Color(255, 160, 16);
const Color Color::Brown = Color(160, 128, 96);
const Color Color::Pale_Pink = Color(227, 186, 240);

/*
 * Constructor from LinearColor
 */
Color::Color(const LinearColor& linearColor)
 : r(static_cast<uint8>(Math::clamp(linearColor.r * 255.0f, 0.0f, 255.0f))),
   g(static_cast<uint8>(Math::clamp(linearColor.g * 255.0f, 0.0f, 255.0f))),
   b(static_cast<uint8>(Math::clamp(linearColor.b * 255.0f, 0.0f, 255.0f))),
   a(static_cast<uint8>(Math::clamp(linearColor.a * 255.0f, 0.0f, 255.0f))) {}

/*
 * Convert to 16-bit color value
 */
uint16
Color::to16Bit(bool bIsRGB565) const {
  if (bIsRGB565) {
    // RGB565 format: 5 bits red, 6 bits green, 5 bits blue
    return static_cast<uint16>(((r >> 3) << 11) | // 5 bits for red
                               ((g >> 2) << 5) |  // 6 bits for green
                               (b >> 3)           // 5 bits for blue
    );
  }
  else {
    // RGB555 format: 5 bits red, 5 bits green, 5 bits blue
    return static_cast<uint16>(((r >> 3) << 10) | // 5 bits for red
                               ((g >> 3) << 5) |  // 5 bits for green
                               (b >> 3)           // 5 bits for blue
    );
  }
}

/*
 * Create a color from a 16-bit value
 */
Color
Color::from16Bit(uint16 value, bool bIsRGB565) {
  Color color;

  if (bIsRGB565) {
    // RGB565 format
    // Extract each component and scale to 0-255 range
    color.r = static_cast<uint8>(((value >> 11) & 0x1F) * 255 / 31);
    color.g = static_cast<uint8>(((value >> 5) & 0x3F) * 255 / 63);
    color.b = static_cast<uint8>((value & 0x1F) * 255 / 31);
  }
  else {
    // RGB555 format
    const float scale = 255.0f / 31.0f;
    color.r = static_cast<uint8>(((value >> 10) & 0x1F) * scale);
    color.g = static_cast<uint8>(((value >> 5) & 0x1F) * scale);
    color.b = static_cast<uint8>((value & 0x1F) * scale);
  }

  color.a = 255; // Full opacity
  return color;
}

/*
 * Create a random color
 */
Color
Color::makeRandomColor() {
  static Random randomGenerator(0x3A1C8951); // Arbitrary seed value

  return Color(static_cast<uint8>(randomGenerator.getPseudoRandom() % 256),
               static_cast<uint8>(randomGenerator.getPseudoRandom() % 256),
               static_cast<uint8>(randomGenerator.getPseudoRandom() % 256),
               255 // Full opacity
  );
}

/*
 * Create a color from a hex string
 */
Color
Color::fromHexString(const String& hexString) {
  // Remove # if present
  String cleanHex = hexString;
  if (!cleanHex.empty() && cleanHex[0] == '#') {
    cleanHex = cleanHex.substr(1);
  }

  // Check if valid length
  if (cleanHex.length() != 6 && cleanHex.length() != 8) {
    return Color::Black; // Invalid format, return black
  }

  // Parse hex values
  try {
    uint32 value = std::stoul(cleanHex, nullptr, 16);

    if (cleanHex.length() == 6) {
      // Format: RRGGBB
      return Color(static_cast<uint8>((value >> 16) & 0xFF),
                   static_cast<uint8>((value >> 8) & 0xFF), static_cast<uint8>(value & 0xFF),
                   255 // Full opacity
      );
    }
    else {
      // Format: RRGGBBAA
      return Color(static_cast<uint8>((value >> 24) & 0xFF),
                   static_cast<uint8>((value >> 16) & 0xFF),
                   static_cast<uint8>((value >> 8) & 0xFF), static_cast<uint8>(value & 0xFF));
    }
  }
  catch (const std::exception&) {
    return Color::Black; // Invalid hex format, return black
  }
}

/*
 * Convert to hex string
 */
String
Color::toHexString(bool includeAlpha) const {
  char buffer[10]; // #RRGGBBAA + null terminator

  if (includeAlpha) {
    std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X%02X", r, g, b, a);
  }
  else {
    std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X", r, g, b);
  }

  return String(buffer);
}

/*
*/
LinearColor
Color::toLinearColor() const {
  const float invByte = 1.0f / 255.0f;
  return LinearColor(r * invByte, g * invByte, b * invByte, a * invByte);
}

} // namespace chEngineSDK
