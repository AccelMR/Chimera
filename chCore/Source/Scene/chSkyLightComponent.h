/************************************************************************/
/**
 * @file chSkyLightComponent.h
 * @author AccelMR
 * @date 2026/10/15
 * @brief
 *  Component that lights the scene with an image of its surroundings.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chComponent.h"
#include "chTypeTraits.h"
#include "chUUID.h"

namespace chEngineSDK {
class TextureAsset;

/**
 * Exists to light the scene with an image of everything around it (an equirectangular HDR
 * image), so surfaces get light from every direction and not only from the light
 * components, and to draw that image behind the scene. The rotation of its GameObject
 * turns the image. While registered it keeps one SkyLightItem in the scene; when the scene
 * has several, the last one registered is used.
 */
class CH_CORE_EXPORT SkyLightComponent : public Component
{
 public:
  SkyLightComponent() = default;

  /**
   * Null removes the image. The image should be twice as wide as it is tall.
   */
  void
  setTexture(const SPtr<TextureAsset>& texture);

  NODISCARD FORCEINLINE const SPtr<TextureAsset>&
  getTexture() const noexcept { return m_texture; }

  /**
   * The image asked for, kept even when the asset is missing, so saving again keeps it.
   */
  NODISCARD FORCEINLINE const UUID&
  getTextureId() const noexcept { return m_textureId; }

  /**
   * Has no physical unit; Renderer.Exposure scales the result.
   */
  void
  setIntensity(float intensity);

  NODISCARD FORCEINLINE float
  getIntensity() const noexcept { return m_intensity; }

  /**
   * Draws the image behind the scene.
   */
  void
  setShowSky(bool bShowSky);

  NODISCARD FORCEINLINE bool
  getShowSky() const noexcept { return m_bShowSky; }

  void
  serialize(DataStream& stream) const override;

  NODISCARD bool
  deserialize(DataStream& stream) override;

  void
  collectReferences(Vector<UUID>& outReferences) const override;

  static constexpr uint32 SERIALIZATION_VERSION = 1;

 protected:
  void
  onRegister() override;

  void
  onUnregister() override;

  void
  onTransformChanged() override;

 private:
  /**
   * Sends the values to the scene again; call it after changing one.
   */
  void
  updateSkyLightItem();

  SPtr<TextureAsset> m_texture;
  UUID m_textureId = UUID::null();
  float m_intensity = 1.0f;
  uint32 m_skyLightId = 0;
  bool m_bShowSky = true;
};

DECLARE_TYPE_TRAITS(SkyLightComponent)

} // namespace chEngineSDK
