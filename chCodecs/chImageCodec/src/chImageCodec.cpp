/************************************************************************/
/**
 * @file chImageCodec.cpp
 * @author AccelMR
 * @date 2025/07/20
 * @brief
 */
/************************************************************************/

#include "chImageCodec.h"

#include <cstring>

#include "chAssetManager.h"
#include "chFileSystem.h"
#include "chIGraphicsAPI.h"
#include "chLogger.h"
#include "chTextureMips.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace chEngineSDK{

CH_LOG_DECLARE_STATIC(ImageCodecLog, All);

namespace ImageImpoterHelpers{
/*
 * Takes the pixels stb returned (RGBA, 8 bits per channel) and frees them.
 */
Vector<uint8>
takeImage(uint8* data, int32 width, int32 height) {
  Vector<uint8> imageData;
  if (data) {
    imageData.resize(static_cast<SIZE_T>(width) * height * STBI_rgb_alpha);
    memcpy(imageData.data(), data, imageData.size());
    stbi_image_free(data);
  }
  return imageData;
}

/*
 * Takes the pixels stb returned for an HDR image (RGBA, linear floats) and frees them.
 */
Vector<float>
takeHdrImage(float* data, int32 width, int32 height) {
  Vector<float> imageData;
  if (data) {
    imageData.resize(static_cast<SIZE_T>(width) * height * STBI_rgb_alpha);
    memcpy(imageData.data(), data, imageData.size() * sizeof(float));
    stbi_image_free(data);
  }
  return imageData;
}
} // namespace ImageImpoterHelpers

/*
*/
SPtr<IAsset>
ImageCodec::importAsset(const Path& filePath,
                        const String& assetName,
                        const Path& assetFolder,
                        const ImportSettings& settings) {
  CH_ASSERT(FileSystem::isFile(filePath) && "File does not exist");

  int32 width = 0;
  int32 height = 0;
  int32 channels = 0;
  const String path = filePath.toString();
  Vector<uint8> imageData;
  Vector<float> hdrData;
  // Loaded before the take, which reads the size the load writes.
  if (stbi_is_hdr(path.c_str())) {
    float* pixels = stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    hdrData = ImageImpoterHelpers::takeHdrImage(pixels, width, height);
  }
  else {
    uint8* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    imageData = ImageImpoterHelpers::takeImage(pixels, width, height);
  }
  if (imageData.empty() && hdrData.empty()) {
    CH_LOG_ERROR(ImageCodecLog, "Failed to load image from path {0}: {1}", path,
                 stbi_failure_reason());
    return nullptr;
  }

  return createTextureAsset(std::move(imageData), std::move(hdrData), width, height,
                            assetName, assetFolder, FileSystem::absolutePath(filePath),
                            StringView(), settings);
}

/*
*/
SPtr<IAsset>
ImageCodec::importAssetFromMemory(Span<const uint8> data,
                                  const String& assetName,
                                  const Path& assetFolder,
                                  const Path& sourceFile,
                                  StringView sourcePart,
                                  const ImportSettings& settings) {
  int32 width = 0;
  int32 height = 0;
  int32 channels = 0;
  const int32 size = static_cast<int32>(data.size());
  Vector<uint8> imageData;
  Vector<float> hdrData;
  // Loaded before the take, which reads the size the load writes.
  if (stbi_is_hdr_from_memory(data.data(), size)) {
    float* pixels =
        stbi_loadf_from_memory(data.data(), size, &width, &height, &channels, STBI_rgb_alpha);
    hdrData = ImageImpoterHelpers::takeHdrImage(pixels, width, height);
  }
  else {
    uint8* pixels =
        stbi_load_from_memory(data.data(), size, &width, &height, &channels, STBI_rgb_alpha);
    imageData = ImageImpoterHelpers::takeImage(pixels, width, height);
  }
  if (imageData.empty() && hdrData.empty()) {
    CH_LOG_ERROR(ImageCodecLog, "Failed to read image {0} of {1}: {2}", sourcePart,
                 sourceFile, stbi_failure_reason());
    return nullptr;
  }

  return createTextureAsset(std::move(imageData), std::move(hdrData), width, height,
                            assetName, assetFolder, sourceFile, sourcePart, settings);
}

/*
*/
SPtr<IAsset>
ImageCodec::createTextureAsset(Vector<uint8> pixels,
                               Vector<float> hdrPixels,
                               int32 width,
                               int32 height,
                               const String& assetName,
                               const Path& assetFolder,
                               const Path& sourceFile,
                               StringView sourcePart,
                               const ImportSettings& settings) {
  CH_ASSERT(IGraphicsAPI::isStarted() && "Graphics API is not initialized");

  const AssetMetadata metadata =
      makeMetadata<TextureAsset>(assetName, assetFolder);
  const uint32 textureWidth = static_cast<uint32>(width);
  const uint32 textureHeight = static_cast<uint32>(height);
  uint32 mipLevels = 1;
  Format format = Format::Unknown;
  if (!hdrPixels.empty()) {
    // HDR files hold linear light, so the sRGB setting does not apply.
    mipLevels = TextureMips::buildChainRGBA16F(std::move(hdrPixels), textureWidth,
                                               textureHeight, settings.generateMips, pixels);
    format = Format::R16G16B16A16_SFLOAT;
  }
  else {
    mipLevels = settings.generateMips ? TextureMips::appendChainRGBA8(
                                            pixels, textureWidth, textureHeight, settings.srgb)
                                      : 1;
    format = settings.srgb ? Format::R8G8B8A8_SRGB : Format::R8G8B8A8_UNORM;
  }
  SPtr<TextureAsset> textureAsset = chMakeShared<TextureAsset>(
      metadata, std::move(pixels), textureWidth, textureHeight, format, mipLevels);

  if (!saveAndRegister(textureAsset, sourceFile, sourcePart, settings)) {
    CH_LOG_ERROR(ImageCodecLog, "Failed to save texture asset: {0}", assetName);
    return nullptr;
  }

  CH_LOG_INFO(ImageCodecLog, "Imported image asset: {0} from {1}", textureAsset->getName(),
              sourceFile);
  return textureAsset;
}

} // namespace chEngineSDK
