/************************************************************************/
/**
 * @file chTypeTraits.h
 * @author AccelMR
 * @date 2025/07/12
 * @brief  Asset type traits for Chimera Core.
 */
/************************************************************************/
#pragma once

#include "chPlatformTypes.h"
#include "chUUID.h"

namespace chEngineSDK {

template <typename T> struct TypeTraits {
  static constexpr const ANSICHAR*
  getTypeName() {
    return "Unknown";
  }
  static const UUID&
  getTypeId() {
    return UUID::null();
  }
};

#define DECLARE_TYPE_TRAITS(TypeClass)                                                        \
  template <> struct TypeTraits<TypeClass> {                                                  \
    static constexpr const ANSICHAR*                                                          \
    getTypeName() {                                                                           \
      return #TypeClass;                                                                      \
    }                                                                                         \
    static const UUID&                                                                        \
    getTypeId() {                                                                             \
      static const UUID typeId = UUID::createFromName(#TypeClass);                            \
      return typeId;                                                                          \
    }                                                                                         \
  };

// Keep backward compatibility for assets
template <typename T> struct AssetTypeTraits {
  static constexpr const ANSICHAR*
  getTypeName() {
    return TypeTraits<T>::getTypeName();
  }
  static const UUID&
  getTypeId() {
    return TypeTraits<T>::getTypeId();
  }
};

#define DECLARE_ASSET_TYPE(AssetClass) DECLARE_TYPE_TRAITS(AssetClass)

} // namespace chEngineSDK
