/************************************************************************/
/**
 * @file chFlags.h
 * @author AccelMR
 * @date 2022/08/27
 * @brief A set of bits taken from an enum class.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesUtilities.h"

#include <type_traits>

namespace chEngineSDK {

/**
 * A set of values of an enum class used as bits. An enum class has no | or &, and a
 * separate type keeps "one bit" (the enum) apart from "any mix of bits" (Flags), so a
 * function that takes one value cannot get a mix by mistake.
 *
 * Sample usage:
 * enum class AccessMode : uint16 { None = 0, Read = 1 << 0, Write = 1 << 1 };
 * CH_FLAGS(AccessMode);
 *
 * AccessModeFlags mode = AccessMode::Read | AccessMode::Write;
 * mode.isSet(AccessMode::Read);
 */
template<typename Enum>
class Flags
{
  static_assert(std::is_enum_v<Enum>, "Flags needs an enum");

 public:
  using Storage = std::underlying_type_t<Enum>;
  static_assert(std::is_unsigned_v<Storage>, "The enum of Flags needs an unsigned type");

  constexpr Flags() noexcept = default;

  constexpr Flags(Enum value) noexcept
   : m_bits(static_cast<Storage>(value))
  {}

  NODISCARD static constexpr Flags
  fromBits(Storage bits) noexcept
  {
    Flags flags;
    flags.m_bits = bits;
    return flags;
  }

  /**
   *   True when every bit of value is set, so an empty value is always set.
   **/
  NODISCARD FORCEINLINE constexpr bool
  isSet(Flags value) const noexcept
  {
    return (m_bits & value.m_bits) == value.m_bits;
  }

  NODISCARD FORCEINLINE constexpr bool
  isSetAny(Flags value) const noexcept
  {
    return (m_bits & value.m_bits) != 0;
  }

  FORCEINLINE constexpr Flags&
  set(Flags value) noexcept
  {
    m_bits |= value.m_bits;
    return *this;
  }

  FORCEINLINE constexpr Flags&
  unset(Flags value) noexcept
  {
    m_bits &= static_cast<Storage>(~value.m_bits);
    return *this;
  }

  NODISCARD FORCEINLINE constexpr Storage
  getBits() const noexcept
  {
    return m_bits;
  }

  NODISCARD FORCEINLINE constexpr explicit
  operator bool() const noexcept
  {
    return m_bits != 0;
  }

  NODISCARD FORCEINLINE constexpr bool
  operator==(Flags rhs) const noexcept
  {
    return m_bits == rhs.m_bits;
  }

  FORCEINLINE constexpr Flags&
  operator|=(Flags rhs) noexcept
  {
    m_bits |= rhs.m_bits;
    return *this;
  }

  FORCEINLINE constexpr Flags&
  operator&=(Flags rhs) noexcept
  {
    m_bits &= rhs.m_bits;
    return *this;
  }

  FORCEINLINE constexpr Flags&
  operator^=(Flags rhs) noexcept
  {
    m_bits ^= rhs.m_bits;
    return *this;
  }

  NODISCARD FORCEINLINE constexpr Flags
  operator~() const noexcept
  {
    return fromBits(static_cast<Storage>(~m_bits));
  }

  NODISCARD friend FORCEINLINE constexpr Flags
  operator|(Flags lhs, Flags rhs) noexcept
  {
    return lhs |= rhs;
  }

  NODISCARD friend FORCEINLINE constexpr Flags
  operator&(Flags lhs, Flags rhs) noexcept
  {
    return lhs &= rhs;
  }

  NODISCARD friend FORCEINLINE constexpr Flags
  operator^(Flags lhs, Flags rhs) noexcept
  {
    return lhs ^= rhs;
  }

 private:
  Storage m_bits = 0;
};

/**
 * Declares <Enum>Flags and the operators that mix two values of the enum. Use it right
 * after the enum, in the same namespace.
 */
#define CH_FLAGS(Enum)                                                                  \
  using Enum##Flags = ::chEngineSDK::Flags<Enum>;                                       \
  NODISCARD constexpr Enum##Flags operator|(Enum lhs, Enum rhs) noexcept                \
  {                                                                                     \
    return Enum##Flags(lhs) | rhs;                                                      \
  }                                                                                     \
  NODISCARD constexpr Enum##Flags operator&(Enum lhs, Enum rhs) noexcept                \
  {                                                                                     \
    return Enum##Flags(lhs) & rhs;                                                      \
  }                                                                                     \
  NODISCARD constexpr Enum##Flags operator^(Enum lhs, Enum rhs) noexcept                \
  {                                                                                     \
    return Enum##Flags(lhs) ^ rhs;                                                      \
  }                                                                                     \
  NODISCARD constexpr Enum##Flags operator~(Enum value) noexcept                        \
  {                                                                                     \
    return ~Enum##Flags(value);                                                         \
  }                                                                                     \
  static_assert(true, "CH_FLAGS needs a semicolon")

} // namespace chEngineSDK
