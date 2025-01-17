/************************************************************************/
/**
 * @file chFlags.h
 * @author AccelMR
 * @date 2022/08/27
 *   Basic class for Flags to be used along the engine.
 */
 /************************************************************************/
#pragma once

#include "chPrerequisitesUtilities.h"

namespace chEngineSDK {

/*
 * Description: 
 *     Flags type to be used along the engine.
 * 
 *  Since Chimera uses enum classes as standard, there's no | or & operator overloads 
 *    this class is a solution for that.
 * 
 * Sample usage:
 *
 * enum class ACCESS_MODE: FlagAccesMode {
 *   kINVALID = 0x00,
 *   kREAD = 0x01,
 *   kWRITE = 0x02
 * };
 * using FileAccessModeFlag = Flags<ACCESS_MODE>;
 * CH_FLAGS_OPERATORS(ACCESS_MODE);
 *
 * FileAccessModeFlag f;
 * f.isSet(ACCESS_MODE::kINVALID);
 *
 */
template<typename Enum, typename Storage = uint32>
class Flags
{
 public:
  using InternalType = Storage;

  Flags() = default;

  Flags(Enum value) : m_bits(static_cast<Storage>(value)) {}
  Flags(const Flags<Enum, Storage>& value) : m_bits(value.m_bits) {}
  explicit Flags(Storage bits) : m_bits(bits) {}

  /**
   * @brief Checks whether all of the provided bits are set
   */
  FORCEINLINE bool
  isSet(Enum value) const {
    return (m_bits & static_cast<Storage>(value)) == static_cast<Storage>(value);
  }

  /**
   * @brief Checks whether any of the provided bits are set
   */
  FORCEINLINE bool
  isSetAny(Enum value) const {
    return (m_bits & static_cast<Storage>(value)) != 0;
  }

  /**
   * @brief Checks whether any of the provided bits are set
   */
  FORCEINLINE bool
  isSetAny(const Flags<Enum, Storage>& value) const {
    return (m_bits & value.m_bits) != 0;
  }

  /**
   * @brief Activates all of the provided bits.
   */
  FORCEINLINE Flags<Enum, Storage>&
  set(Enum value) {
    m_bits |= static_cast<Storage>(value);
    return *this;
  }

  /**
   * @brief Deactivates all of the provided bits.
   */
  FORCEINLINE void
  unset(Enum value) {
    m_bits &= ~static_cast<Storage>(value);
  }

  FORCEINLINE bool
  operator==(Enum rhs) const {
    return m_bits == static_cast<Storage>(rhs);
  }

  FORCEINLINE bool
  operator==(const Flags<Enum, Storage>& rhs) const {
    return m_bits == rhs.m_bits;
  }

  FORCEINLINE bool
  operator==(bool rhs) const {
    return (static_cast<bool>(*this)) == rhs;
  }

  FORCEINLINE bool
  operator!=(Enum rhs) const {
    return m_bits != static_cast<Storage>(rhs);
  }

  FORCEINLINE bool
  operator!=(const Flags<Enum, Storage>& rhs) const {
    return m_bits != rhs.m_bits;
  }

  FORCEINLINE Flags<Enum, Storage>&
  operator=(Enum rhs) {
    m_bits = static_cast<Storage>(rhs);
    return *this;
  }

  FORCEINLINE Flags<Enum, Storage>&
  operator=(const Flags<Enum, Storage>& rhs) {
    m_bits = rhs.m_bits;
    return *this;
  }

  FORCEINLINE Flags<Enum, Storage>&
  operator|=(Enum rhs) {
    m_bits |= static_cast<Storage>(rhs);
    return *this;
  }

  FORCEINLINE Flags<Enum, Storage>&
  operator|=(const Flags<Enum, Storage>& rhs) {
    m_bits |= rhs.m_bits;
    return *this;
  }

  FORCEINLINE Flags<Enum, Storage>
  operator|(Enum rhs) const {
    Flags<Enum, Storage> out(*this);
    out |= rhs;
    return out;
  }

  FORCEINLINE Flags<Enum, Storage>
  operator|(const Flags<Enum, Storage>& rhs) const {
    Flags<Enum, Storage> out(*this);
    out |= rhs;
    return out;
  }

  FORCEINLINE Flags<Enum, Storage>&
  operator&=(Enum rhs) {
    m_bits &= static_cast<Storage>(rhs);
    return *this;
  }

  FORCEINLINE Flags<Enum, Storage>&
  operator&=(const Flags<Enum, Storage>& rhs) {
    m_bits &= rhs.m_bits;
    return *this;
  }

  FORCEINLINE Flags<Enum, Storage>
  operator&(Enum rhs) const {
    Flags<Enum, Storage> out = *this;
    out.m_bits &= static_cast<Storage>(rhs);
    return out;
  }

  FORCEINLINE Flags<Enum, Storage>
  operator&(const Flags<Enum, Storage>& rhs) const {
    Flags<Enum, Storage> out = *this;
    out.m_bits &= rhs.m_bits;
    return out;
  }

  FORCEINLINE Flags<Enum, Storage>&
  operator^=(Enum rhs) {
    m_bits ^= static_cast<Storage>(rhs);
    return *this;
  }

  FORCEINLINE Flags<Enum, Storage>&
  operator^=(const Flags<Enum, Storage>& rhs) {
    m_bits ^= rhs.m_bits;
    return *this;
  }

  FORCEINLINE Flags<Enum, Storage>
  operator^(Enum rhs) const {
    Flags<Enum, Storage> out = *this;
    out.m_bits ^= static_cast<Storage>(rhs);
    return out;
  }

  FORCEINLINE Flags<Enum, Storage>
  operator^(const Flags<Enum, Storage>& rhs) const {
    Flags<Enum, Storage> out = *this;
    out.m_bits ^= rhs.m_bits;
    return out;
  }

  FORCEINLINE Flags<Enum, Storage>
  operator~() const {
    Flags<Enum, Storage> out;
    out.m_bits = (Storage)~m_bits;
    return out;
  }

  FORCEINLINE operator bool() const {
    return m_bits ? true : false;
  }

  FORCEINLINE operator uint8() const {
    return static_cast<uint8>(m_bits);
  }

  FORCEINLINE operator uint16() const {
    return static_cast<uint16>(m_bits);
  }

  FORCEINLINE operator uint32() const {
    return static_cast<uint32>(m_bits);
  }

  friend Flags<Enum, Storage>
  operator&(Enum a, Flags<Enum, Storage> &b) {
    Flags<Enum, Storage> out;
    out.m_bits = a & b.m_bits;
    return out;
  }

 private:
  Storage m_bits{0};
};

/**
* Defines global operators for a Flags<Enum, Storage> implementation.
*/
#define CH_FLAGS_OPERATORS(Enum) CH_FLAGS_OPERATORS_EXT(Enum, uint32)

/**
* Defines global operators for a Flags<Enum, Storage> implementation.
*/
#define CH_FLAGS_OPERATORS_EXT(Enum, Storage)                               \
inline Flags<Enum, Storage>                                                 \
operator|(Enum a, Enum b) {                                                 \
  Flags<Enum, Storage> r(a); r |= b; return r;                              \
}                                                                           \
                                                                            \
inline Flags<Enum, Storage>                                                 \
operator&(Enum a, Enum b) {                                                 \
  Flags<Enum, Storage> r(a); r &= b; return r;                              \
}                                                                           \
                                                                            \
inline Flags<Enum, Storage>                                                 \
operator~(Enum a) {                                                         \
  return ~Flags<Enum, Storage>(a);                                          \
}
}
