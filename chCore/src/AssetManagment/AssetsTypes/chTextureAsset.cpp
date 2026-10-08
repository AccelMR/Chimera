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
#include "chMath.h"
#include "chTextureMips.h"

namespace chEngineSDK {
CH_LOG_DECLARE_STATIC(TextureAssetLog, All);

namespace {
SIZE_T
getChainSize(Format format, uint32 width, uint32 height, uint32 mipLevels)
{
  SIZE_T size = 0;
  for (uint32 mip = 0; mip < mipLevels; ++mip) {
    size += FormatUtils::getMipSize(format, Math::max(width >> mip, 1u),
                                    Math::max(height >> mip, 1u));
  }
  return size;
}
} // namespace

/*
 */
TextureAsset::TextureAsset(const AssetMetadata& metadata,
                           Vector<uint8> textureData,
                           uint32 width,
                           uint32 height,
                           Format format,
                           uint32 mipLevels)
  : IAsset(metadata),
    m_textureData(std::move(textureData)),
    m_width(width),
    m_height(height),
    m_format(format),
    m_mipLevels(mipLevels)
{
  CH_ASSERT(m_textureData.size() == getChainSize(format, width, height, mipLevels));
  createTextureFromData();
}

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
                                  .format = m_format,
                                  .mipLevels = m_mipLevels};
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
    CH_LOG_ERROR(TextureAssetLog,
                 "Failed to deserialize texture asset {0}: bad header or old version, import "
                 "it again",
                 getName());
    return false;
  }
  // The values come from the file, so they are checked before they index the format table
  // or size the data.
  if (static_cast<uint32>(header.format) >= static_cast<uint32>(Format::COUNT) ||
      header.mipLevels == 0 ||
      header.mipLevels > TextureMips::getMipCount(header.width, header.height)) {
    CH_LOG_ERROR(TextureAssetLog,
                 "Failed to deserialize texture asset {0}: format {1} with {2} mips",
                 getName(), header.format, header.mipLevels);
    return false;
  }
  m_width = header.width;
  m_height = header.height;
  m_format = header.format;
  m_mipLevels = header.mipLevels;
  m_textureData.resize(getChainSize(m_format, m_width, m_height, m_mipLevels));
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
                                      .format = m_format,
                                      .width = m_width,
                                      .height = m_height,
                                      .depth = 1,
                                      .mipLevels = m_mipLevels,
                                      .arrayLayers = 1,
                                      .samples = SampleCount::Count1,
                                      .usage = TextureUsage::Sampled |
                                               TextureUsage::TransferDst,
                                      .initialData = m_textureData.data(),
                                      .initialDataSize = m_textureData.size()};
  m_texture = graphicsAPI.createTexture(textureCreateInfo);
}

} // namespace chEngineSDK
