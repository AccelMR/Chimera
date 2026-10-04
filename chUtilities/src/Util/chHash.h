/************************************************************************/
/**
 * @file chHash.h
 * @author AccelMR
 * @date 2026/10/03
 * @brief
 * 64-bit FNV-1a hashing of bytes and plain values.
 */
/************************************************************************/
#pragma once

#include <bit>

#include "chPrerequisitesUtilities.h"

namespace chEngineSDK {

/**
 * Builds stable 64-bit keys for caches (pipelines, samplers) from the fields of a
 * description. Hash fields one by one with combine(); hashing a whole struct with
 * hashBytes() would also hash its padding, whose bytes are undefined.
 */
class HashUtils
{
 public:
  static constexpr uint64 FNV_OFFSET_BASIS = 14695981039346656037ull;
  static constexpr uint64 FNV_PRIME = 1099511628211ull;

  NODISCARD static constexpr uint64
  hashBytes(const uint8* bytes, SIZE_T size, uint64 seed = FNV_OFFSET_BASIS) noexcept
  {
    uint64 hash = seed;
    for (SIZE_T i = 0; i < size; ++i) {
      hash ^= bytes[i];
      hash *= FNV_PRIME;
    }
    return hash;
  }

  /**
   * Mixes a number, enum, bool or pointer into the hash.
   */
  template<typename T>
  NODISCARD static FORCEINLINE uint64
  combine(uint64 hash, T value) noexcept
  {
    static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_pointer_v<T>,
                  "combine takes numbers, enums and pointers; hash structs field by field");
    uint64 bits = 0;
    if constexpr (std::is_pointer_v<T>) {
      bits = reinterpret_cast<SIZE_T>(value);
    }
    else if constexpr (std::is_enum_v<T>) {
      bits = static_cast<uint64>(static_cast<std::underlying_type_t<T>>(value));
    }
    else if constexpr (std::is_floating_point_v<T>) {
      // 0.0 and -0.0 compare equal, so they must hash the same.
      bits = value == T(0) ? 0 : std::bit_cast<std::conditional_t<sizeof(T) == 8, uint64,
                                                                   uint32>>(value);
    }
    else {
      bits = static_cast<uint64>(value);
    }

    for (SIZE_T i = 0; i < sizeof(T); ++i) {
      hash ^= (bits >> (i * 8)) & 0xFF;
      hash *= FNV_PRIME;
    }
    return hash;
  }
};

} // namespace chEngineSDK
