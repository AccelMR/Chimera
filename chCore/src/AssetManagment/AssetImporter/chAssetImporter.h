/************************************************************************/
/**
 * @file chAssetImporter.h
 * @author AccelMR
 * @date 2025/07/12
 * @brief  Asset importer interface for Chimera Core.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#if USING(CH_EDITOR)
#include "chUUID.h"
#include "chIAsset.h"
#include "chPath.h"

namespace chEngineSDK {

class IAsset;
class CH_CORE_EXPORT IAssetImporter {
 public:
  IAssetImporter() = default;
  virtual ~IAssetImporter() = default;

  virtual UUID
  getImportType() const = 0;

  virtual Vector<String>
  getSupportedExtensions() const = 0;

  virtual SPtr<IAsset>
  importAsset(const Path& filePath, const String& assetName) = 0;

  virtual bool
  canImport(const String& extension) const = 0;

  FORCEINLINE virtual void
  setOriginalPath(const SPtr<IAsset>& asset, const Path& importedPath) {
    CH_ASSERT(asset);
    asset->setOriginalPath(importedPath.toString().c_str());
  }
};

} // namespace chEngineSDK

#endif // USING(CH_EDITOR)
