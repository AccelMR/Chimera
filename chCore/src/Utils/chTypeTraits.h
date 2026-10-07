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

/**
 * Stable name and UUID of a type, built from its name, so the id is the same in every module
 * and in every run (saved files can store it). A type gets them with DECLARE_TYPE_TRAITS;
 * DECLARED tells templates at compile time whether it did.
 */
template <typename T> struct TypeTraits {
  static constexpr bool DECLARED = false;

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
    static constexpr bool DECLARED = true;                                                    \
                                                                                              \
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

  static UUID
  getNamespacedTypeId() {
    static const UUID namespacedTypeId =
        UUID::createFromName("chAssets");
    return namespacedTypeId;
  }
};

#define DECLARE_TYPE_TRAITS_NAMESPACE_ID(TypeClass, NameSpaceIdExpr)                          \
  template <> struct TypeTraits<TypeClass> {                                                  \
    static constexpr bool DECLARED = true;                                                    \
                                                                                              \
    static constexpr const ANSICHAR*                                                          \
    getTypeName() {                                                                           \
      return #TypeClass;                                                                      \
    }                                                                                         \
    static const UUID&                                                                        \
    getTypeId() {                                                                             \
      static const UUID typeId = UUID::createFromName(#TypeClass, NameSpaceIdExpr);           \
      return typeId;                                                                          \
    }                                                                                         \
  };

#define DECLARE_ASSET_TYPE(AssetClass)                                                        \
  DECLARE_TYPE_TRAITS_NAMESPACE_ID(AssetClass, AssetTypeTraits<AssetClass>::getNamespacedTypeId())

} // namespace chEngineSDK
