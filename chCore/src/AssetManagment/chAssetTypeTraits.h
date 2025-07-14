/************************************************************************/
/**
 * @file chAssetTypeTraits.h
 * @author AccelMR
 * @date 2025/07/12
 * @brief  Asset type traits for Chimera Core.
 */
/************************************************************************/
#pragma once

#include "chPlatformTypes.h"
#include "chUUID.h"

namespace chEngineSDK {
template <typename T> struct AssetTypeTraits {
  static constexpr const ANSICHAR* getTypeName() { return "Unknown"; }

  static const UUID& getTypeId() { return UUID::null(); }
};

#define DECLARE_ASSET_TYPE(AssetClass)                                                        \
  template <> struct AssetTypeTraits<AssetClass> {                                            \
    static constexpr const ANSICHAR*                                                          \
    getTypeName() {                                                                           \
      return #AssetClass;                                                                     \
    }                                                                                         \
    static const UUID&                                                                        \
    getTypeId() {                                                                             \
      static const UUID typeId = UUID::createFromName(#AssetClass);                           \
      return typeId;                                                                          \
    }                                                                                         \
  };
} // namespace chEngineSDK
