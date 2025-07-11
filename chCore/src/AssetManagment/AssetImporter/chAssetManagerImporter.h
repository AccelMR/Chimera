/************************************************************************/
/**
 * @file chAssetManagerImporter.h
 * @author AccelMR
 * @date 2025/07/10
 * @brief  Class for importing assets into the Chimera Engine.
 * /
/************************************************************************/
#pragma once

#if USING(CH_EDIOR)
#include "chPrerequisitesCore.h"

#include "chModule.h"

namespace chEngineSDK {
class CH_CORE_EXPORT AssetManagerImporter : public Module<AssetManagerImporter> {
 public:
  /**
   * @brief Imports an asset from the specified import path to the asset path.
   * @param importPath The path to the asset to be imported.
   * @param assetPath The destination path for the imported asset.
   * @return True if the import was successful, false otherwise.
   */
  bool
  importAsset(const Path& importPath, const Path& assetPath);

 private:

}; // class AssetManagerImporter
} // namespace chEngineSDK
#endif // USING(CH_EDIOR)
