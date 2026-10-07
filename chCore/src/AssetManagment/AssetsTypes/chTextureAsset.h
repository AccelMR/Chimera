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
  static constexpr uint32 VERSION = 1;

  uint32 version = VERSION;
  uint32 width = 0;
  uint32 height = 0;
  Format format = Format::Unknown;
};
static_assert(sizeof(TextureAssetHeader) == 16, "TextureAssetHeader must have no padding");

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
  TextureAsset(const AssetMetadata& metadata,
               Vector<uint8> textureData,
               uint32 width,
               uint32 height)
    : IAsset(metadata), m_textureData(std::move(textureData)), m_width(width), m_height(height) {
    createTextureFromData();
  }

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
};
DECLARE_ASSET_TYPE(TextureAsset);

} // namespace chEngineSDK
