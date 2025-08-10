/************************************************************************/
/**
 * @file chTextureAsset.cpp
 * @author AccelMR
 * @date 2025/07/20
 * @brief
 */
/************************************************************************/
#include "chTextureAsset.h"

#include "chIGraphicsAPI.h"
#include "chFileStream.h"
#include "chLogger.h"

namespace chEngineSDK {
  namespace FormatUtils {
/** * @brief Get the size of the format in bytes
 * @param format The format to check
 * @return The size of the format in bytes
 */
NODISCARD FORCEINLINE uint32
getFormatSize(Format format) {
  switch (format) {
    case Format::R8G8B8A8_UNORM:
      return 4;
    case Format::B8G8R8A8_SRGB:
      return 4;
    case Format::R16G16B16A16_SFLOAT:
      return 8;
    case Format::D32_SFLOAT:
      return 4;
    case Format::D24_UNORM_S8_UINT:
      return 4; // 24 bits for depth, 8 bits for stencil
    default:
      return 0; // Unknown format size
  }
}
} // namespace FormatUtils

CH_LOG_DECLARE_STATIC(TextureAssetLog, All);

/*
 */
bool
TextureAsset::serialize(SPtr<DataStream> stream) {
  if (!stream) {
    CH_LOG_ERROR(TextureAssetLog, "Failed to serialize texture asset: stream is null");
    return false;
  }

  if (m_textureData.empty()) {
    CH_LOG_ERROR(TextureAssetLog, "Failed to serialize texture asset: texture data is empty");
    return false;
  }
  TextureAssetMetadata metadata;
  metadata.width = m_width;
  metadata.height = m_height;
  metadata.format = Format::R8G8B8A8_UNORM;
  stream << metadata;

  stream->write(m_textureData.data(), m_textureData.size());
  CH_LOG_DEBUG(TextureAssetLog, "Serialized texture asset {0} with size {1}",
                                 getName(),
                                 m_textureData.size() * sizeof(uint8));
  return true;
}

/*
 */
bool
TextureAsset::deserialize(SPtr<DataStream> stream) {
  if (!stream) {
    CH_LOG_ERROR(TextureAssetLog, "Failed to deserialize texture asset: stream is null");
    return false;
  }

  // Deserialize texture data
  m_textureData.clear();

  TextureAssetMetadata metadata;
  stream >> metadata;
  m_width = metadata.width;
  m_height = metadata.height;
  m_textureData.resize(m_width * m_height *
                       FormatUtils::getFormatSize(metadata.format));
  if (m_textureData.empty()) {
    CH_LOG_ERROR(TextureAssetLog, "Failed to deserialize texture asset: texture data is empty");
    return false;
  }
  if (stream->read(m_textureData.data(), m_textureData.size()) != m_textureData.size()) {
    CH_LOG_ERROR(TextureAssetLog, "Failed to read texture data from stream");
    return false;
  }

  createTextureFromData();

  return true;
}

/*
 */
void
TextureAsset::clearAssetData() {
  m_textureData.clear();
}

/*
*/
void
TextureAsset::createTextureFromData() {
  if (m_textureData.empty()) {
    CH_LOG_ERROR(TextureAssetLog, "Cannot create texture: texture data is empty");
    return;
  }
  if (m_texture) {
    CH_LOG_DEBUG(TextureAssetLog, "Texture already created, skipping creation");
    return;
  }

  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  TextureCreateInfo textureCreateInfo{.type = TextureType::Texture2D,
                                      .format = Format::R8G8B8A8_UNORM,
                                      .width = static_cast<uint32>(m_width),
                                      .height = static_cast<uint32>(m_height),
                                      .depth = 1,
                                      .mipLevels = 1,
                                      .arrayLayers = 1,
                                      .samples = SampleCount::Count1,
                                      .usage = TextureUsage::Sampled |
                                               TextureUsage::TransferDst,
                                      .initialData = m_textureData.data(),
                                      .initialDataSize = m_textureData.size()};
  m_texture = graphicsAPI.createTexture(textureCreateInfo);
}

} // namespace chEngineSDK
