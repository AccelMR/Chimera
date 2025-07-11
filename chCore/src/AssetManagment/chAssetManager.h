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

#include "chIAsset.h"
#include "chModule.h"
#include "chLogger.h"

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

  bool
  loadAsset(const SPtr<IAsset>& asset);

  WeakPtr<IAsset>
  createAsset(AssetType type, const String& name = "", const Path& originalPath = Path::EMPTY);

 private:
  Map<UUID, SPtr<IAsset>> m_assets;
  Map<UUID, SPtr<IAsset>> m_loadedAssets;

}; // class AssetManager
} // namespace chEngineSDK
