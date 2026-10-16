/************************************************************************/
/**
 * @file chFloat16.h
 * @author AccelMR
 * @date 2026/10/15
 * @brief Conversions between float and 16-bit floats.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesUtilities.h"

#include <bit>

namespace chEngineSDK {

/**
 * Exists because GPU formats store half floats (IEEE 754 binary16: 1 sign bit, 5 exponent
 * bits, 10 mantissa bits) that the CPU has no type for, so data meant for them is packed
 * and read here. Rounds to the nearest value (ties to even), keeps infinities and NaN, and
 * turns values too large for a half into infinity.
 */
class Float16
{
 public:
  NODISCARD static constexpr uint16
  fromFloat(float value) noexcept
  {
    const uint32 bits = std::bit_cast<uint32>(value);
    const uint16 sign = static_cast<uint16>((bits >> 16) & 0x8000u);
    const uint32 absolute = bits & 0x7FFFFFFFu;

    if (absolute >= 0x7F800000u) {
      // Infinity stays infinity; NaN keeps a mantissa bit so it stays NaN.
      return static_cast<uint16>(sign | 0x7C00u | (absolute > 0x7F800000u ? 0x200u : 0u));
    }
    if (absolute >= 0x477FF000u) {
      // At or above 65520 the rounding reaches the next power of two, past the largest half.
      return static_cast<uint16>(sign | 0x7C00u);
    }
    if (absolute < 0x38800000u) {
      // Below the smallest normal half: a subnormal, or zero. The implicit 1 of the float
      // is put back and the mantissa shifted down to the half's fixed exponent.
      if (absolute < 0x33000000u) {
        return sign;
      }
      const uint32 exponent = absolute >> 23;
      const uint32 mantissa = (absolute & 0x7FFFFFu) | 0x800000u;
      const uint32 shift = 126u - exponent;
      const uint32 half = mantissa >> shift;
      const uint32 remainder = mantissa & ((1u << shift) - 1u);
      const uint32 midpoint = 1u << (shift - 1u);
      const uint32 rounded =
          half + ((remainder > midpoint || (remainder == midpoint && (half & 1u))) ? 1u : 0u);
      return static_cast<uint16>(sign | rounded);
    }

    // Normal: rebias the exponent from 127 to 15 and round the 13 dropped mantissa bits. A
    // carry out of the mantissa moves to the exponent, which is still correct.
    const uint32 rebased = absolute - 0x38000000u;
    const uint32 half = rebased >> 13;
    const uint32 remainder = rebased & 0x1FFFu;
    const uint32 rounded =
        half + ((remainder > 0x1000u || (remainder == 0x1000u && (half & 1u))) ? 1u : 0u);
    return static_cast<uint16>(sign | rounded);
  }

  NODISCARD static constexpr float
  toFloat(uint16 value) noexcept
  {
    const uint32 sign = static_cast<uint32>(value & 0x8000u) << 16;
    const uint32 exponent = (value >> 10) & 0x1Fu;
    uint32 mantissa = value & 0x3FFu;

    if (exponent == 0x1Fu) {
      return std::bit_cast<float>(sign | 0x7F800000u | (mantissa << 13));
    }
    if (exponent != 0) {
      return std::bit_cast<float>(sign | ((exponent + 112u) << 23) | (mantissa << 13));
    }
    if (mantissa == 0) {
      return std::bit_cast<float>(sign);
    }

    // Subnormal half: normalize it, since every subnormal half is a normal float.
    uint32 floatExponent = 113u;
    while ((mantissa & 0x400u) == 0) {
      mantissa <<= 1;
      --floatExponent;
    }
    return std::bit_cast<float>(sign | (floatExponent << 23) | ((mantissa & 0x3FFu) << 13));
  }
};

} // namespace chEngineSDK
