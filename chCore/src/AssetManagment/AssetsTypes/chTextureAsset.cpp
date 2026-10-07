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
  const TextureAssetHeader header{.width = m_width,
                                  .height = m_height,
                                  .format = Format::R8G8B8A8_UNORM};
  stream << header;

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

  TextureAssetHeader header;
  if (stream->read(&header, sizeof(header)) != sizeof(header) ||
      header.version != TextureAssetHeader::VERSION) {
    CH_LOG_ERROR(TextureAssetLog, "Failed to deserialize texture asset {0}: bad header",
                 getName());
    return false;
  }
  m_width = header.width;
  m_height = header.height;
  // The value comes from the file, so it is checked before it indexes the format table.
  if (static_cast<uint32>(header.format) >= static_cast<uint32>(Format::COUNT)) {
    CH_LOG_ERROR(TextureAssetLog, "Failed to deserialize texture asset: unknown format {0}",
                 header.format);
    return false;
  }
  m_textureData.resize(FormatUtils::getMipSize(header.format, m_width, m_height));
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
