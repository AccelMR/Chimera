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

#include "chAssetMeta.h"
#include "chPath.h"
#include "chSTDThreading.h"
#include "chTypeTraits.h"
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

/**
 * What identifies an asset in memory. Only uuid and assetType are stored in its file; the
 * name and the folder come from where the file is, so moving or renaming the file never
 * leaves them out of date.
 */
struct AssetMetadata
{
  UUID uuid = UUID::null();
  UUID assetType = UUID::null();
  ANSICHAR name[64] = "Unnamed";
  // Virtual folder of the file, such as /Game/Sponza.
  ANSICHAR assetPath[256] = "";
};

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
  getAssetTypeId() const { return m_metadata.assetType; }

  /**
   * Name of the asset type, from its registration ("Unknown" when it is not registered).
   */
  NODISCARD const ANSICHAR*
  getTypeName() const;

  NODISCARD FORCEINLINE const ANSICHAR*
  getName() const { return m_metadata.name; }

  NODISCARD FORCEINLINE const ANSICHAR*
  getAssetPath() const { return m_metadata.assetPath; }

  /**
   * The asset file: "<asset path>/<name>.chAss".
   */
  NODISCARD Path
  getFilePath() const;

#if USING(CH_EDITOR)
  NODISCARD FORCEINLINE const AssetEditorData&
  getEditorData() const { return m_editorData; }
#endif // USING(CH_EDITOR)

  /**
   * The assets this one points to, as saved in its file. They are loaded before it. Read
   * when the file is first found and kept while unloaded, so what uses an asset can be
   * found without loading anything.
   */
  NODISCARD FORCEINLINE const Vector<UUID>&
  getReferencedAssets() const { return m_referencedAssets; }

 protected:
  friend class AssetManager;
  friend class IAssetCodec;

  IAsset() {}
  IAsset(const AssetMetadata& metadata)
   : m_metadata(metadata), m_state(AssetState::Unloaded), m_refCount(0) {
    CH_ASSERT(validateMetadata(metadata));
  }

  NODISCARD bool
  save();

 protected:

  /**
   * Renames the asset file (and its .meta file in the editor).
   */
  NODISCARD bool
  rename(const ANSICHAR* newName);

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

  /**
   * Adds the UUIDs of the assets this one needs, called when it is saved.
   */
  virtual void
  collectReferences(Vector<UUID>& outReferences) const
  {
    CH_PARAMETER_UNUSED(outReferences);
  }

#if USING(CH_EDITOR)
  /**
   * Adds what only this asset type keeps in its .meta file, called when it is saved.
   */
  virtual void
  writeEditorValues(ConfigFile& file) const
  {
    CH_PARAMETER_UNUSED(file);
  }

  /**
   * Reads what writeEditorValues wrote, called when the asset file is first found.
   */
  virtual void
  readEditorValues(const ConfigFile& file)
  {
    CH_PARAMETER_UNUSED(file);
  }
#endif // USING(CH_EDITOR)

  NODISCARD static bool
  validateMetadata(const AssetMetadata& metadata);

  AssetMetadata m_metadata;  ///< Metadata for the asset
  AssetState m_state;        ///< State of the asset
  Atomic<uint32> m_refCount; ///< Reference count for the asset

  Vector<UUID> m_referencedAssets; ///< Filled by collectReferences on save, read on load

#if USING(CH_EDITOR)
  AssetEditorData m_editorData; ///< Saved in the .meta file, never in the asset file
#endif // USING(CH_EDITOR)
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
