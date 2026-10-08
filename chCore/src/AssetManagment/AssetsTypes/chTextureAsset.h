/************************************************************************/
/**
 * @file chTextureAsset.h
 * @author AccelMR
 * @date 2025/07/20
 * @brief
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chTypeTraits.h"
#include "chIAsset.h"
#include "chGraphicsTypes.h"
#include "chITexture.h"

namespace chEngineSDK {
class ITexture;

// Written after the common asset start, before the pixels.
struct TextureAssetHeader
{
  static constexpr uint32 VERSION = 2;

  uint32 version = VERSION;
  uint32 width = 0;
  uint32 height = 0;
  Format format = Format::Unknown;
  // The pixels hold every level, one after the other, largest first.
  uint32 mipLevels = 1;
};
static_assert(sizeof(TextureAssetHeader) == 20, "TextureAssetHeader must have no padding");

/**
 * @class TextureAsset
 * @brief Represents a texture asset in the engine.
 *
 * This class manages texture assets, providing functionality to load,
 * unload, and serialize texture data.
 */
class CH_CORE_EXPORT TextureAsset : public IAsset
{
 public:
  TextureAsset() = delete;
  TextureAsset(const AssetMetadata& metadata) : IAsset(metadata) {}

  /**
   * textureData holds mipLevels levels of the format, one after the other, largest first.
   */
  TextureAsset(const AssetMetadata& metadata,
               Vector<uint8> textureData,
               uint32 width,
               uint32 height,
               Format format,
               uint32 mipLevels);

  ~TextureAsset() = default;

  NODISCARD FORCEINLINE const SPtr<ITexture>&
  getTexture() const { return m_texture; }

 protected:
  bool
  serialize(SPtr<DataStream>) override;

  bool
  deserialize(SPtr<DataStream>) override;

  void
  clearAssetData() override;

  void
  createTextureFromData();

 private:
  SPtr<ITexture> m_texture;
  Vector<uint8> m_textureData; ///< Compressed texture data for serialization
  uint32 m_width = 0; ///< Width of the texture
  uint32 m_height = 0; ///< Height of the texture
  Format m_format = Format::Unknown;
  uint32 m_mipLevels = 1;
};
DECLARE_ASSET_TYPE(TextureAsset);

} // namespace chEngineSDK
