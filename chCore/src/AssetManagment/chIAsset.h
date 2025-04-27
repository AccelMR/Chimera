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

#include "chUUID.h"
#include "chPath.h"

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

struct AssetMetadata {
  AssetType type = AssetType::None;
  String name = "";
  UUID uuid = UUID::null();
  uint64 creationTime = ~0;
  Path originalPath = "";
  Path cachedPath = "";
};

class CH_CORE_EXPORT IAsset : public std::enable_shared_from_this<IAsset>
{
 public:
  IAsset() = default;

  template<typename T>
  NODISCARD FORCEINLINE SPtr<T>
  as(){
    if (isTypeOf<T>()) {
      return std::static_pointer_cast<T>(shared_from_this());
    }
  }

  template<typename T>
  NODISCARD FORCEINLINE bool
  isTypeOf() const;

  NODISCARD FORCEINLINE bool
  isLoaded() const { return m_state == AssetState::Loaded; }

  NODISCARD FORCEINLINE bool
  isLoading() const { return m_state == AssetState::Loading; }

  NODISCARD FORCEINLINE bool
  isUnloading() const { return m_state == AssetState::Unloading; }

  NODISCARD FORCEINLINE bool
  isUnloaded() const { return m_state == AssetState::Unloaded; }

  NODISCARD FORCEINLINE bool
  isFailed() const { return m_state == AssetState::Failed; }

  uint32
  getReferenceCount() const { return m_refCount.load(); }

  NODISCARD FORCEINLINE const String&
  getName() const { return m_metadata.name; }

  void
  setName(const String& name) { m_metadata.name = name; }

  NODISCARD FORCEINLINE const Path&
  getOriginalPath() const { return m_metadata.originalPath; }

  void
  setOriginalPath(const Path& path) { m_metadata.originalPath = path; }

  NODISCARD FORCEINLINE const Path&
  getCachedPath() const { return m_metadata.cachedPath; }

  void
  setCachedPath(const Path& path) { m_metadata.cachedPath = path; }

  NODISCARD FORCEINLINE const UUID&
  getUUID() const { return m_metadata.uuid; }

  NODISCARD FORCEINLINE AssetState
  getState() const { return m_state; }

 protected:
  friend class AssetManager;

  virtual void
  clearAssetData() = 0;

  NODISCARD virtual bool
  load();

  NODISCARD virtual bool
  unload();

  NODISCARD bool
  save();

  virtual bool
  serialize(SPtr<DataStream> stream) = 0;

  virtual bool
  deserialize(SPtr<DataStream> stream) = 0;

  NODISCARD FORCEINLINE AssetType
  getType() const { return m_metadata.type; }

  bool
  validateMetadata(const AssetMetadata& ) const;

  AssetMetadata m_metadata; ///< Metadata for the asset
  AssetState m_state; ///< State of the asset
  Atomic<uint32> m_refCount; ///< Reference count for the asset

  Vector<UUID> m_referencedAssets; ///< List of referenced assets
}; // class IAsset

/*
*/
template<typename T>
requires std::is_base_of<IAsset, T>::value
bool IAsset::isTypeOf() const {
  return getType() == T::getStaticType();
}
} // namespace chEngineSDK