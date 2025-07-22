/************************************************************************/
/**
 * @file chImageImporter.h
 * @author AccelMR
 * @date 2025/07/20
 * @brief
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chAssetImporter.h"
#include "chTypeTraits.h"

#include "chTextureAsset.h"

namespace chEngineSDK{
class TextureAsset;

/*
* @class ImageImporter
* @brief Importer for image assets
*
* This class handles importing image files into the engine as texture assets.
*/
class ImageImporter : public IAssetImporter {
 public:
  ImageImporter() = default;
  ~ImageImporter() = default;

  UUID
  getImporterType() const override {
    static UUID importType = UUID::createFromName("ImageImporter");
    return importType;
  }

  FORCEINLINE Vector<UUID>
  getSupportedAssetTypes() const override {
    return {AssetTypeTraits<TextureAsset>::getTypeId()};
  }

  Vector<String>
  getSupportedExtensions() const override;

  SPtr<IAsset>
  importAsset(const Path& filePath, const String& assetName) override;

  bool
  canImport(const String& extension) const override;

 private:

};
DECLARE_ASSET_TYPE(ImageImporter);

} // namespace chEngineSDK
