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

  SPtr<IAsset>
  importAsset(const Path& filePath,
              const String& assetName,
              const Path& assetFolder,
              const ImportSettings& settings) override;

  SPtr<IAsset>
  importAssetFromMemory(Span<const uint8> data,
                        const String& assetName,
                        const Path& assetFolder,
                        const Path& sourceFile,
                        StringView sourcePart,
                        const ImportSettings& settings) override;

 private:
  /**
   * 8-bit images become RGBA8 (sRGB or not, as the settings say); HDR images (.hdr) become
   * R16G16B16A16_SFLOAT, linear, keeping values above 1.
   */
  SPtr<IAsset>
  createTextureAsset(Vector<uint8> pixels,
                     Vector<float> hdrPixels,
                     int32 width,
                     int32 height,
                     const String& assetName,
                     const Path& assetFolder,
                     const Path& sourceFile,
                     StringView sourcePart,
                     const ImportSettings& settings);

  // The formats stb_image reads. It has no EXR support.
  const Vector<String> m_extensions = {"png", "jpg", "jpeg", "bmp", "tga", "hdr"};
};
DECLARE_ASSET_TYPE(ImageCodec);

} // namespace chEngineSDK
