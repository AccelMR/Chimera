/************************************************************************/
/**
 * @file chIAsset.h
 * @author AccelMR
 * @date 2025/04/26
 * @brief
 *  This is a base class for all assets in the engine.
 *   It provides a common interface for loading, saving, and managing assets.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chAssetTypeTraits.h"
#include "chUUID.h"

namespace chEngineSDK {

enum class AssetState : uint16 {
  None = 0,
  Loading,
  Loaded,
  Unloading,
  Unloaded,
  Failed,

  COUNT
};

// Align AssetMetadata to 8 bytes for optimal memory access on most platforms
struct MS_ALIGN(8) AssetMetadata {
  UUID uuid = UUID::null();      // 16 bytes
  UUID assetType = UUID::null(); // 16 bytes
  uint64 creationTime = ~0;      // 8 bytes

  ANSICHAR typeName[32] = "Unknown";    // 32 bytes
  ANSICHAR engineVersion[16] = "x.x.x"; // 16 bytes
  ANSICHAR name[64] = "Unnamed";        // 64 bytes
  ANSICHAR importedPath[256] = "";      // 256 bytes
  ANSICHAR assetPath[256] = "";         // 256 bytes

  // Total size: 16 + 16 + 8 + 32 + 16 + 64 + 256 + 256 = 664 bytes
} GCC_PACK(8);
static_assert(sizeof(AssetMetadata) % 8 == 0, "AssetMetadata is not 8-byte aligned!");

class CH_CORE_EXPORT IAsset : public std::enable_shared_from_this<IAsset>
{
 public:
  virtual ~IAsset() = default;

  template <typename T>
  NODISCARD FORCEINLINE SPtr<T>
  as() {
    if (isTypeOf<T>()) {
      return std::static_pointer_cast<T>(shared_from_this());
    }
    return nullptr;
  }

  template <typename T>
    requires std::is_base_of<IAsset, T>::value
  NODISCARD FORCEINLINE bool
  isTypeOf() const;

  NODISCARD FORCEINLINE bool
  isLoaded() const {
    return m_state == AssetState::Loaded;
  }

  NODISCARD FORCEINLINE bool
  isLoading() const {
    return m_state == AssetState::Loading;
  }

  NODISCARD FORCEINLINE bool
  isUnloading() const {
    return m_state == AssetState::Unloading;
  }

  NODISCARD FORCEINLINE bool
  isUnloaded() const {
    return m_state == AssetState::Unloaded;
  }

  NODISCARD FORCEINLINE bool
  isFailed() const {
    return m_state == AssetState::Failed;
  }

  uint32
  getReferenceCount() const {
    return m_refCount.load();
  }

  NODISCARD FORCEINLINE const UUID&
  getUUID() const {
    return m_metadata.uuid;
  }

  NODISCARD FORCEINLINE AssetState
  getState() const {
    return m_state;
  }

  NODISCARD FORCEINLINE const UUID&
  getAssetType() const { return m_metadata.assetType; }

  NODISCARD FORCEINLINE const ANSICHAR*
  getTypeName() const { return m_metadata.typeName; }

  NODISCARD FORCEINLINE const ANSICHAR*
  getName() const { return m_metadata.name; }

  NODISCARD FORCEINLINE const ANSICHAR*
  getImportedPath() const { return m_metadata.importedPath; }

  NODISCARD FORCEINLINE const ANSICHAR*
  getAssetPath() const { return m_metadata.assetPath; }

  NODISCARD FORCEINLINE uint64
  getCreatedAt() const { return m_metadata.creationTime; }

  NODISCARD bool
  save();

 protected:
  friend class AssetManager;
  friend class IAssetImporter;

  IAsset() {}
  IAsset(const AssetMetadata& metadata)
   : m_metadata(metadata), m_state(AssetState::Unloaded), m_refCount(0) {
    CH_ASSERT(validateMetadata(metadata));
  }

 protected:

  NODISCARD bool
  rename(const ANSICHAR* newName);

  bool
  updateMetadata(const AssetMetadata& newMetadata);

  FORCEINLINE void
  setMetadata(const AssetMetadata& metadata) { m_metadata = metadata; }

  void
  setOriginalPath(const ANSICHAR* importedPath);

  void
  setAssetPath(const ANSICHAR* assetPath);

  virtual void
  clearAssetData() = 0;

  NODISCARD bool
  load();

  NODISCARD bool
  unload();

  virtual bool
  serialize(SPtr<DataStream>) = 0;

  virtual bool
  deserialize(SPtr<DataStream>) = 0;

  bool
  validateMetadata(const AssetMetadata&) const;

  AssetMetadata m_metadata;  ///< Metadata for the asset
  AssetState m_state;        ///< State of the asset
  Atomic<uint32> m_refCount; ///< Reference count for the asset

  Vector<UUID> m_referencedAssets; ///< List of referenced assets
}; // class IAsset

/*
 */
template <typename T>
  requires std::is_base_of<IAsset, T>::value
bool
IAsset::isTypeOf() const {
  return AssetTypeTraits<T>::getTypeId() == m_metadata.assetType;
} // isTypeOf
} // namespace chEngineSDK
