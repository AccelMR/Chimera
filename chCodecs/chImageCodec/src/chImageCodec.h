/************************************************************************/
/**
 * @file chImageCodec.h
 * @author AccelMR
 * @date 2025/07/20
 * @brief
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chAssetCodec.h"
#include "chTypeTraits.h"

#include "chTextureAsset.h"

namespace chEngineSDK{
class TextureAsset;

/*
* @class ImageCodec
* @brief Codec for image assets
*
* This class handles importing image files into the engine as texture assets.
*/
class ImageCodec : public IAssetCodec {
 public:
  ImageCodec() = default;
  ~ImageCodec() = default;

  UUID
  getCodecType() const override {
    static UUID importType = UUID::createFromName("ImageCodec");
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
DECLARE_ASSET_TYPE(ImageCodec);

} // namespace chEngineSDK
