/************************************************************************/
/**
 * @file chAssetManager.cpp
 * @author AccelMR
 * @date 2025/04/26
 * @brief
 * AssetManager class for managing assets in the engine.
 * It handles loading, unloading, and saving assets.
 * It also provides functionality for managing asset references and states.
 */
/************************************************************************/

#include "chAssetManager.h"

#include "chAssetTypeTraits.h"
#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chSupportedImport.h"
#include "chLogger.h"
#include "chPath.h"

// Base asset types
#include "chModelAsset.h"

namespace chEngineSDK {
CH_LOG_DEFINE_CATEGORY_SHARED(AssetSystem, All);

/*
*/
void
AssetManager::initialize() {
  CH_LOG_DEBUG(AssetSystem, "Initializing AssetManager");

  m_assetRegister = chMakeShared<AssetRegister>();

  // Register asset creators
  m_assetRegister->registerAssetCreator<ModelAsset>(AssetTypeTraits<ModelAsset>::getTypeId());

  m_onRegisterAsset(m_assetRegister);
}
} // namespace chEngineSDK
