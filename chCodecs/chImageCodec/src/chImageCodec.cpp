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
  uint8* data = stbi_load(filePath.toString().c_str(), &width, &height, &channels,
                          STBI_rgb_alpha);
  Vector<uint8> imageData = ImageImpoterHelpers::takeImage(data, width, height);
  if (imageData.empty()) {
    CH_LOG_ERROR(ImageCodecLog, "Failed to load image from path: {0}", filePath.toString());
    return nullptr;
  }

  const Path importedPath = FileSystem::absolutePath(filePath);
  return createTextureAsset(std::move(imageData), width, height, assetName, assetFolder,
                            importedPath.toString(), settings);
}

/*
*/
SPtr<IAsset>
ImageCodec::importAssetFromMemory(Span<const uint8> data,
                                  const String& assetName,
                                  const Path& assetFolder,
                                  StringView importedPath,
                                  const ImportSettings& settings) {
  int32 width = 0;
  int32 height = 0;
  int32 channels = 0;
  uint8* pixels = stbi_load_from_memory(data.data(), static_cast<int32>(data.size()), &width,
                                        &height, &channels, STBI_rgb_alpha);
  Vector<uint8> imageData = ImageImpoterHelpers::takeImage(pixels, width, height);
  if (imageData.empty()) {
    CH_LOG_ERROR(ImageCodecLog, "Failed to read image {0}: {1}", importedPath,
                 stbi_failure_reason());
    return nullptr;
  }

  return createTextureAsset(std::move(imageData), width, height, assetName, assetFolder,
                            importedPath, settings);
}

/*
*/
SPtr<IAsset>
ImageCodec::createTextureAsset(Vector<uint8> pixels,
                               int32 width,
                               int32 height,
                               const String& assetName,
                               const Path& assetFolder,
                               StringView importedPath,
                               const ImportSettings& settings) {
  CH_ASSERT(IGraphicsAPI::isStarted() && "Graphics API is not initialized");

  const AssetMetadata metadata =
      makeMetadata<TextureAsset>(assetName, importedPath, assetFolder);
  const uint32 textureWidth = static_cast<uint32>(width);
  const uint32 textureHeight = static_cast<uint32>(height);
  const uint32 mipLevels =
      settings.generateMips
          ? TextureMips::appendChainRGBA8(pixels, textureWidth, textureHeight, settings.srgb)
          : 1;
  const Format format = settings.srgb ? Format::R8G8B8A8_SRGB : Format::R8G8B8A8_UNORM;
  SPtr<TextureAsset> textureAsset = chMakeShared<TextureAsset>(
      metadata, std::move(pixels), textureWidth, textureHeight, format, mipLevels);

  if (!saveAndRegister(textureAsset)) {
    CH_LOG_ERROR(ImageCodecLog, "Failed to save texture asset: {0}", assetName);
    return nullptr;
  }

  CH_LOG_INFO(ImageCodecLog, "Imported image asset: {0} from {1}", textureAsset->getName(),
              importedPath);
  return textureAsset;
}

} // namespace chEngineSDK
