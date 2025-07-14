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
  loadAsset(const SPtr<IAsset>& asset);

  /*
   *
   */
  template <typename TAsset = IAsset>
  WeakPtr<TAsset>
  createAsset(const String& name,
              const Path& assetPath); // Must be relative to the asset directory

 private:
  Map<UUID, SPtr<IAsset>> m_assets; ///< Map of all assets by UUID, both loaded and unloaded
  Map<UUID, SPtr<IAsset>> m_loadedAssets; ///< Map of currently loaded assets by UUID

  Event<bool(const SPtr<IAsset>&)>
      m_onAssetLoaded; ///< Event triggered when an asset is loaded
  Event<bool(const SPtr<IAsset>&)> m_onAssetUnloaded; ///< Event triggered

  SPtr<AssetRegister> m_assetRegister; ///< Asset registry for creating assets
  Event<bool(const SPtr<AssetRegister>&)>
      m_onRegisterAsset; ///< Event triggered when an asset is registered
}; // class AssetManager

/******************************************************************************************* */

/*
 */
template <typename TAsset>
WeakPtr<TAsset>
AssetManager::createAsset(const String& name, const Path& assetPath) {

  bool validationPassed = true;
  auto validateAsset = [&validationPassed](bool condition, const String& errorMessage) {
    if (!condition) {
      CH_LOG(AssetSystem, Error, errorMessage);
      validationPassed = false;
    }
  };
  validateAsset(!name.empty(), "Asset name cannot be empty");
  validateAsset(!assetPath.empty(), "Asset path cannot be empty");
  validateAsset(FileSystem::exists(assetPath),
                "Asset path does not exist: " + assetPath.toString());
  validateAsset(FileSystem::arePathsRelative(chEnginePaths::ASSETS_PATH, assetPath),
                chString::format("Asset path must be relative to the asset directory: {0}",
                                 chEnginePaths::ASSETS_PATH));
  if (!validationPassed) {
    CH_LOG(AssetSystem, Error, "Failed to create asset due to validation errors");
    return WeakPtr<TAsset>();
  }

  const String AssetTypeName = AssetTypeTraits<TAsset>::getTypeName();
  const UUID assetUUID = UUID::createFromName(AssetTypeName);
  AssetCreatorFunc assetCreator = m_assetRegister->getAssetCreator(assetUUID);
  if (!assetCreator) {
    CH_LOG(AssetSystem, Error, "No asset creator found for UUID: {0}", assetUUID.toString());
    return WeakPtr<TAsset>();
  }

  const UUID refUUID = UUID::createRandom();
  AssetMetadata metadata = AssetMetadataUtils::createAssetMetadata(
      &refUUID,
      assetUUID,
      0, // std::chrono::system_clock::now().time_since_epoch().count(),
      AssetTypeTraits<TAsset>::getTypeName(), CH_ENGINE_VERSION_STRING, name.c_str(),
      assetPath.toString().c_str(), assetPath.toString().c_str());
  SPtr<IAsset> asset = std::static_pointer_cast<IAsset>(assetCreator(metadata));

  if (!asset) {
    CH_LOG_ERROR(AssetSystem, "Failed to create asset of type {0} with UUID {1}",
                 AssetTypeName, assetUUID.toString());
    return WeakPtr<TAsset>();
  }

  CH_LOG(AssetSystem, Debug, "Asset {0} created with UUID {1} at path {2}", name,
         assetUUID.toString(), assetPath.toString());

  return std::static_pointer_cast<TAsset>(asset);
}

} // namespace chEngineSDK
