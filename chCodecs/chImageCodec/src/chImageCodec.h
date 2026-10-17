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

  NODISCARD const Vector<String>&
  getSupportedExtensions() const override
  {
    return m_extensions;
  }

  /**
   * 8-bit images become RGBA8 (sRGB or not, as the settings say); HDR images (.hdr) become
   * R16G16B16A16_SFLOAT, linear, keeping values above 1.
   */
  NODISCARD UniquePtr<IImportTask>
  createImportTask(const Path& filePath,
                   const String& assetName,
                   const ImportSettings& settings) override;

  NODISCARD UniquePtr<IImportTask>
  createImportTaskFromMemory(Span<const uint8> data,
                             const String& assetName,
                             const Path& sourceFile,
                             StringView sourcePart,
                             const ImportSettings& settings) override;

 private:
  // The formats stb_image reads. It has no EXR support.
  const Vector<String> m_extensions = {"png", "jpg", "jpeg", "bmp", "tga", "hdr"};
};
DECLARE_ASSET_TYPE(ImageCodec);

} // namespace chEngineSDK
