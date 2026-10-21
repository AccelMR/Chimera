/************************************************************************/
/**
 * @file chSkyLightComponent.cpp
 * @author AccelMR
 * @date 2026/10/15
 * @brief
 *  Component that lights the scene with an image of its surroundings.
 */
/************************************************************************/
#include "chSkyLightComponent.h"

#include "chAssetManager.h"
#include "chFileStream.h"
#include "chGameObject.h"
#include "chITexture.h"
#include "chMath.h"
#include "chScene.h"
#include "chSceneManager.h"
#include "chTextureAsset.h"

namespace chEngineSDK {

namespace {
struct SkyLightFileData
{
  UUID texture;
  float intensity;
  uint32 showSky;
};
static_assert(sizeof(SkyLightFileData) == 24, "SkyLightFileData must have no padding");
} // namespace

/*
 */
void
SkyLightComponent::setTexture(const SPtr<TextureAsset>& texture)
{
  m_texture = texture;
  m_textureId = texture ? texture->getUUID() : UUID::null();
  const ITexture* gpuTexture = texture ? texture->getTexture().get() : nullptr;
  if (gpuTexture && gpuTexture->getWidth() != gpuTexture->getHeight() * 2) {
    CH_LOG_WARNING(SceneManagerLog,
                   "Sky light image {0} is {1}x{2}; an equirectangular image is twice as "
                   "wide as it is tall, so the sky will look stretched.",
                   texture->getName(), gpuTexture->getWidth(), gpuTexture->getHeight());
  }
  updateSkyLightItem();
}

/*
 */
void
SkyLightComponent::setIntensity(float intensity)
{
  m_intensity = Math::max(intensity, 0.0f);
  updateSkyLightItem();
}

/*
 */
void
SkyLightComponent::setShowSky(bool bShowSky)
{
  m_bShowSky = bShowSky;
  updateSkyLightItem();
}

/*
 */
void
SkyLightComponent::serialize(DataStream& stream) const
{
  const uint32 version = SERIALIZATION_VERSION;
  const SkyLightFileData data{.texture = m_textureId,
                              .intensity = m_intensity,
                              .showSky = m_bShowSky ? 1u : 0u};
  stream.write(&version, sizeof(version));
  stream.write(&data, sizeof(data));
}

/*
 */
bool
SkyLightComponent::deserialize(DataStream& stream)
{
  uint32 version = 0;
  SkyLightFileData data;
  if (stream.read(&version, sizeof(version)) != sizeof(version) || version == 0 ||
      version > SERIALIZATION_VERSION || stream.read(&data, sizeof(data)) != sizeof(data)) {
    return false;
  }

  SPtr<TextureAsset> texture;
  if (!data.texture.isNull() && AssetManager::isStarted()) {
    const SPtr<IAsset> asset = AssetManager::instance().getAsset(data.texture);
    texture = asset ? asset->as<TextureAsset>() : nullptr;
  }
  setTexture(texture);
  m_textureId = data.texture;
  if (!texture && !data.texture.isNull()) {
    CH_LOG_WARNING(SceneManagerLog, "A sky light lost its image {0}.", data.texture);
  }
  setIntensity(data.intensity);
  setShowSky(data.showSky != 0);
  return true;
}

/*
 */
void
SkyLightComponent::collectReferences(Vector<UUID>& outReferences) const
{
  if (!m_textureId.isNull()) {
    outReferences.push_back(m_textureId);
  }
}

/*
 */
void
SkyLightComponent::onRegister()
{
  m_skyLightId = getOwner()->getScene()->addSkyLight(SkyLightItem{.owner = getOwner()});
  updateSkyLightItem();
}

/*
 */
void
SkyLightComponent::onUnregister()
{
  getOwner()->getScene()->removeSkyLight(m_skyLightId);
}

/*
 */
void
SkyLightComponent::onTransformChanged()
{
  updateSkyLightItem();
}

/*
 */
void
SkyLightComponent::updateSkyLightItem()
{
  if (!isRegistered()) {
    return;
  }

  SkyLightItem& item = getOwner()->getScene()->getSkyLight(m_skyLightId);
  item.texture = m_texture ? m_texture->getTexture().get() : nullptr;
  item.intensity = m_intensity;
  item.bShowSky = m_bShowSky;

  // Row vectors: row i of the world matrix is where local axis i ends up. Without scale
  // those rows are a rotation, whose inverse is its transpose.
  const Matrix4& world = getOwner()->getTransform().getWorldMatrix();
  Matrix4 worldToSky = Matrix4::IDENTITY;
  for (int32 axis = 0; axis < 3; ++axis) {
    Vector3 direction(world[axis][0], world[axis][1], world[axis][2]);
    if (!direction.normalize()) {
      item.worldToSky = Matrix4::IDENTITY;
      return;
    }
    worldToSky[0][axis] = direction.x;
    worldToSky[1][axis] = direction.y;
    worldToSky[2][axis] = direction.z;
  }
  item.worldToSky = worldToSky;
}

} // namespace chEngineSDK
