/************************************************************************/
/**
 * @file chMaterialAsset.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief
 *  A Material saved as an asset, with references to its textures.
 */
/************************************************************************/
#include "chMaterialAsset.h"

#include "chAssetManager.h"
#include "chFileStream.h"
#include "chLogger.h"
#include "chTextureAsset.h"

namespace chEngineSDK {

/*
 */
bool
MaterialAsset::serialize(SPtr<DataStream> stream)
{
  if (!stream) {
    return false;
  }
  m_material.serialize(*stream);
  return true;
}

/*
 */
bool
MaterialAsset::deserialize(SPtr<DataStream> stream)
{
  if (!stream || !m_material.deserialize(*stream)) {
    CH_LOG_ERROR(AssetSystem, "Failed to read material {0}", getName());
    return false;
  }

  // IAsset::load already loaded the references, so the texture is ready when it exists.
  const UUID& textureId = m_material.getBaseColorTextureId();
  if (textureId.isNull()) {
    return true;
  }
  const SPtr<IAsset> asset = AssetManager::instance().getAsset(textureId);
  SPtr<TextureAsset> texture = asset ? asset->as<TextureAsset>() : nullptr;
  if (!texture) {
    CH_LOG_WARNING(AssetSystem, "Material {0} lost its base color texture {1}", getName(),
                   textureId);
  }
  m_material.setBaseColorTexture(textureId, texture);
  return true;
}

/*
 */
void
MaterialAsset::clearAssetData()
{
  m_material = Material();
}

/*
 */
void
MaterialAsset::collectReferences(Vector<UUID>& outReferences) const
{
  m_material.collectReferences(outReferences);
}

} // namespace chEngineSDK
