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

#include "chModule.h"

#include "chLogDeclaration.h"


namespace chEngineSDK {

CH_LOG_DECLARE_EXTERN(AssetSystem);

enum class AssetType : uint32 {
  None = 0,
  Mesh,
  Model,
  Material,
  Texture,
  Shader,
  Prefab,

  COUNT
};

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
  importAsset(const Path& importPath, const Path& assetPath);
  
 private:

  bool
  importModel(const Path& importPath, const Path& fullAssetPath);

}; // class AssetManager
} // namespace chEngineSDK