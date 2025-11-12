/************************************************************************/
/**
 * @file chAssetManager.h
 * @author AccelMR
 * @date 2025/04/26
 * @brief
 * AssetManager class for managing assets in the engine.
 * It handles loading, unloading, and saving assets.
 * It also provides functionality for managing asset references and states.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chAssetRegister.h"
#include "chEnginePaths.h"
#include "chEventSystem.h"
#include "chFileSystem.h"
#include "chIAsset.h"
#include "chLogger.h"
#include "chModelAsset.h"
#include "chModule.h"
#include "chStringUtils.h"

#include "chSceneAsset.h"

namespace chEngineSDK {

CH_LOG_DECLARE_EXTERN(AssetSystem);

class CH_CORE_EXPORT AssetManager : public Module<AssetManager>
{
 public:
  /**
   * Default constructor
   */
  AssetManager() = default;

  /**
   * Destructor
   */
  ~AssetManager() = default;

  void
  initialize();

  bool
  syncLoadAsset(const SPtr<IAsset>& asset);

  bool
  unloadAsset(const SPtr<IAsset>& asset);

  bool
  syncLoadAsset(const UUID& assetUUID);

  bool
  unloadAsset(const UUID& assetUUID);

  bool
  renameAsset(const UUID& assetUUID, const ANSICHAR* newName);

  bool
  renameAsset(const SPtr<IAsset>& asset, const ANSICHAR* newName);

  void
  lazyLoadAssetsFromDirectory(const Path& directory);

  NODISCARD bool
  saveAsset(const SPtr<IAsset>& asset){
    if (!asset) {
      CH_LOG_ERROR(AssetSystem, "Cannot save null asset");
      return false;
    }
    if (!asset->save()) {
      CH_LOG_ERROR(AssetSystem, "Failed to save asset {0}", asset->getName());
      return false;
    }
    CH_LOG_DEBUG(AssetSystem, "Saved asset {0}", asset->getName());
    return true;
  }

  /*
   * Load a scene by its name.
   * !important: This loads a scene that is already registered as an asset. There's no way to load a scene directly from a file path.
   * and that's intentional.
   * @param name Name of the scene to load.
   * @return Weak pointer to the loaded scene, or nullptr if loading failed.
   */
  WeakPtr<SceneAsset>
  loadSceneByName(const String& name);

  WeakPtr<SceneAsset>
  getSceneByName(const String& name) const{
    for (const auto& [uuid, asset] : m_sceneAssets) {
      if (chString::compare(asset->getName(), name)) {
        return asset->as<SceneAsset>();
      }
    }
    return WeakPtr<SceneAsset>();
  }

  WeakPtr<SceneAsset>
  getSceneByUUID(const UUID& uuid) const{
    auto it = m_sceneAssets.find(uuid);
    if (it != m_sceneAssets.end()) {
      return it->second->as<SceneAsset>();
    }
    return WeakPtr<SceneAsset>();
  }

  /*
   * Check if a scene with the given name exists.
   * @param name Name of the scene to check.
   * @return True if the scene exists, false otherwise.
   */
  FORCEINLINE bool
  doesSceneExist(const String& name) const{
    for (const auto& [uuid, asset] : m_sceneAssets) {
      if (chString::compare(asset->getName(), name)) {
        return true;
      }
    }
    return false;
  }

#if USING(CH_EDITOR)
  bool
  removeAsset(const UUID& assetUUID);
#endif // Editor-specific functionality

  /*
   *
   */
  template <typename TAsset = IAsset>
  WeakPtr<TAsset>
  createAsset(const String& name,
              const Path& assetPath); // Must be relative to the asset directory

  FORCEINLINE Vector<SPtr<IAsset>>
  getAllAssets() const {
    Vector<SPtr<IAsset>> assets;
    assets.reserve(m_assets.size());
    for (const auto& pair : m_assets) {
      assets.push_back(pair.second);
    }
    return assets;
  }

  NODISCARD FORCEINLINE const String&
  getAssetTypeName(const UUID& assetUUID) const {
    CH_ASSERT(m_assetRegister && "AssetRegister must be initialized before accessing asset types.");
    return m_assetRegister->getAssetTypeName(assetUUID);
  }

 private:
  friend class IAssetCodec;

  void
  registerNewAsset(const SPtr<IAsset>& asset) {
    CH_ASSERT(asset && "Asset cannot be null");
    m_assets[asset->getUUID()] = asset;
  }

  SPtr<IAsset>
  lazyDeserialize(const SPtr<DataStream>& stream);

  void
  cacheSceneAssets();

 private:
  Map<UUID, SPtr<IAsset>> m_assets; ///< Map of all assets by UUID, both loaded and unloaded
  Map<UUID, SPtr<IAsset>> m_loadedAssets; ///< Map of currently loaded assets by UUID
  Map<UUID, SPtr<IAsset>> m_sceneAssets; ///< Map of scene assets by UUID

  Event<bool(const SPtr<IAsset>&)> m_onAssetLoaded; ///< Event triggered when an asset is loaded
  Event<bool(const SPtr<IAsset>&)> m_onAssetUnloaded; ///< Event triggered

  SPtr<AssetRegister> m_assetRegister; ///< Asset registry for creating assets
}; // class AssetManager

/******************************************************************************************* */

/*
 */
template <typename TAsset>
WeakPtr<TAsset>
AssetManager::createAsset(const String& name, const Path& assetPath) {

  bool validationPassed = true;
  auto validateInfo = [&validationPassed](bool condition, const String& errorMessage) {
    if (!condition) {
      CH_LOG(AssetSystem, Error, errorMessage);
      validationPassed = false;
    }
  };
  validateInfo(!name.empty(), "Asset name cannot be empty");
  validateInfo(!assetPath.empty(), "Asset path cannot be empty");
  validateInfo(FileSystem::exists(assetPath),
                "Asset path does not exist: " + assetPath.toString());
  validateInfo(FileSystem::arePathsRelative(EnginePaths::getGameAssetDirectory(), assetPath),
                chString::format("Asset path must be relative to the asset directory: {0}",
                                 EnginePaths::getGameAssetDirectory().toString()));
  if (!validationPassed) {
    CH_LOG(AssetSystem, Error, "Failed to create asset due to validation errors");
    return WeakPtr<TAsset>();
  }
  const UUID assetUUID = AssetTypeTraits<TAsset>::getTypeId();
  AssetCreatorFunc assetCreator = m_assetRegister->getAssetCreator(assetUUID);
  if (assetCreator == nullptr) {
    CH_LOG(AssetSystem, Error, "No asset creator found for UUID: {0}", assetUUID.toString());
    return WeakPtr<TAsset>();
  }

  const String AssetTypeName = AssetTypeTraits<TAsset>::getTypeName();

  const UUID refUUID = UUID::createRandom();
  AssetMetadata metadata;
  metadata.uuid = refUUID;
  metadata.assetType = assetUUID;
  metadata.creationTime = std::chrono::system_clock::now().time_since_epoch().count();
  chString::copyToANSI(metadata.typeName, AssetTypeName, AssetTypeName.size() + 1);
  chString::copyToANSI(metadata.engineVersion, CH_ENGINE_VERSION_STRING, sizeof(CH_ENGINE_VERSION_STRING));
  chString::copyToANSI(metadata.name, name, name.size() + 1);
  chString::copyToANSI(metadata.assetPath, assetPath.toString(),
                        assetPath.toString().size() + 1);

  SPtr<IAsset> asset = assetCreator(metadata);

  if (!asset) {
    CH_LOG_ERROR(AssetSystem, "Failed to create asset of type {0} with UUID {1}",
                 AssetTypeName, assetUUID.toString());
    return WeakPtr<TAsset>();
  }

  CH_LOG(AssetSystem, Debug, "Asset {0} created with UUID {1} at path {2}", name,
         assetUUID.toString(), assetPath.toString());

  // DeleteMe
  m_loadedAssets[refUUID] = asset;
  m_assets[refUUID] = asset;

  return std::static_pointer_cast<TAsset>(asset);
}

} // namespace chEngineSDK
