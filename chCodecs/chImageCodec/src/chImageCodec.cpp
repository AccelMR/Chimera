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

/**
 * Exists so the image is decoded and its mips built on the import's worker thread, and only
 * the texture asset is made on the main thread.
 */
class ImageImportTask final : public IImportTask
{
 public:
  ImageImportTask(const Path& filePath,
                  Span<const uint8> data,
                  const String& assetName,
                  const Path& sourceFile,
                  StringView sourcePart,
                  const ImportSettings& settings)
    : m_filePath(filePath),
      m_data(data),
      m_assetName(assetName),
      m_sourceFile(sourceFile),
      m_sourcePart(sourcePart),
      m_settings(settings)
  {}

  NODISCARD bool
  readSource(ImportProgress& progress) override;

  SPtr<IAsset>
  createAssets(const Path& assetFolder) override;

 private:
  NODISCARD bool
  decode(Vector<float>& outHdrPixels);

  // Empty when the image is read from m_data.
  Path m_filePath;
  Span<const uint8> m_data;
  String m_assetName;
  Path m_sourceFile;
  String m_sourcePart;
  ImportSettings m_settings;

  Vector<uint8> m_pixels;
  uint32 m_width = 0;
  uint32 m_height = 0;
  uint32 m_mipLevels = 1;
  Format m_format = Format::Unknown;
};

/*
 * Fills m_pixels for an 8-bit image or outHdrPixels for an HDR one.
 */
bool
ImageImportTask::decode(Vector<float>& outHdrPixels)
{
  int32 width = 0;
  int32 height = 0;
  int32 channels = 0;
  // Each load goes into a local before the take, which reads the size the load writes.
  if (m_filePath.empty()) {
    const int32 size = static_cast<int32>(m_data.size());
    if (stbi_is_hdr_from_memory(m_data.data(), size)) {
      float* pixels = stbi_loadf_from_memory(m_data.data(), size, &width, &height, &channels,
                                             STBI_rgb_alpha);
      outHdrPixels = takeHdrImage(pixels, width, height);
    }
    else {
      uint8* pixels = stbi_load_from_memory(m_data.data(), size, &width, &height, &channels,
                                            STBI_rgb_alpha);
      m_pixels = takeImage(pixels, width, height);
    }
  }
  else {
    const String& path = m_filePath.toString();
    if (stbi_is_hdr(path.c_str())) {
      float* pixels = stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
      outHdrPixels = takeHdrImage(pixels, width, height);
    }
    else {
      uint8* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
      m_pixels = takeImage(pixels, width, height);
    }
  }

  if (m_pixels.empty() && outHdrPixels.empty()) {
    // stb keeps the failure reason per thread.
    CH_LOG_ERROR(ImageCodecLog, "Failed to read image {0} {1}: {2}", m_sourceFile,
                 m_sourcePart, stbi_failure_reason());
    return false;
  }
  m_width = static_cast<uint32>(width);
  m_height = static_cast<uint32>(height);
  return true;
}

/*
 */
bool
ImageImportTask::readSource(ImportProgress& progress)
{
  if (!m_filePath.empty() && !FileSystem::isFile(m_filePath)) {
    CH_LOG_ERROR(ImageCodecLog, "File not found: {0}", m_filePath);
    return false;
  }

  Vector<float> hdrPixels;
  if (!decode(hdrPixels) || progress.isCancelled()) {
    return false;
  }

  if (!hdrPixels.empty()) {
    // HDR files hold linear light, so the sRGB setting does not apply.
    m_mipLevels = TextureMips::buildChainRGBA16F(std::move(hdrPixels), m_width, m_height,
                                                 m_settings.generateMips, m_pixels);
    m_format = Format::R16G16B16A16_SFLOAT;
  }
  else {
    m_mipLevels = m_settings.generateMips
                      ? TextureMips::appendChainRGBA8(m_pixels, m_width, m_height,
                                                      m_settings.srgb)
                      : 1;
    m_format = m_settings.srgb ? Format::R8G8B8A8_SRGB : Format::R8G8B8A8_UNORM;
  }
  return true;
}

/*
 */
SPtr<IAsset>
ImageImportTask::createAssets(const Path& assetFolder)
{
  CH_ASSERT(IGraphicsAPI::isStarted() && "Graphics API is not initialized");

  const AssetMetadata metadata =
      IAssetCodec::makeMetadata<TextureAsset>(m_assetName, assetFolder);
  SPtr<TextureAsset> textureAsset = chMakeShared<TextureAsset>(
      metadata, std::move(m_pixels), m_width, m_height, m_format, m_mipLevels);

  if (!IAssetCodec::saveAndRegister(textureAsset, m_sourceFile, m_sourcePart, m_settings)) {
    CH_LOG_ERROR(ImageCodecLog, "Failed to save texture asset: {0}", m_assetName);
    return nullptr;
  }

  CH_LOG_INFO(ImageCodecLog, "Imported image asset: {0} from {1}", textureAsset->getName(),
              m_sourceFile);
  return textureAsset;
}
} // namespace ImageImpoterHelpers

/*
*/
UniquePtr<IImportTask>
ImageCodec::createImportTask(const Path& filePath,
                             const String& assetName,
                             const ImportSettings& settings)
{
  return chMakeUnique<ImageImpoterHelpers::ImageImportTask>(
      filePath, Span<const uint8>(), assetName, FileSystem::absolutePath(filePath),
      StringView(), settings);
}

/*
*/
UniquePtr<IImportTask>
ImageCodec::createImportTaskFromMemory(Span<const uint8> data,
                                       const String& assetName,
                                       const Path& sourceFile,
                                       StringView sourcePart,
                                       const ImportSettings& settings)
{
  return chMakeUnique<ImageImpoterHelpers::ImageImportTask>(Path(), data, assetName,
                                                            sourceFile, sourcePart, settings);
}

} // namespace chEngineSDK
