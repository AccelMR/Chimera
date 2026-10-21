/************************************************************************/
/**
 * @file chLightComponent.h
 * @author AccelMR
 * @date 2026/10/13
 * @brief
 *  Components that light the scene from their GameObject.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chAngle.h"
#include "chComponent.h"
#include "chLightItem.h"
#include "chTypeTraits.h"

namespace chEngineSDK {

/**
 * Exists to share what every light has (color, intensity, its item in the scene) between
 * the light types. While registered it keeps one LightItem in the scene up to date with
 * the GameObject's place, so the renderer never looks at components. Lights shine along
 * the GameObject's forward axis.
 */
class CH_CORE_EXPORT LightComponent : public Component
{
 public:
  /**
   * Linear; the intensity scales it.
   */
  void
  setColor(const LinearColor& color);

  NODISCARD FORCEINLINE const LinearColor&
  getColor() const noexcept { return m_color; }

  /**
   * Has no physical unit; Renderer.Exposure scales the result.
   */
  void
  setIntensity(float intensity);

  NODISCARD FORCEINLINE float
  getIntensity() const noexcept { return m_intensity; }

  /**
   * Directional and spot lights draw shadows; point lights keep the flag for later.
   */
  void
  setCastShadows(bool bCastShadows);

  NODISCARD FORCEINLINE bool
  getCastShadows() const noexcept { return m_bCastShadows; }

  NODISCARD FORCEINLINE LightType
  getLightType() const noexcept { return m_type; }

  /**
   * Writes color, intensity and the shadow flag; the types with more values write them
   * after.
   */
  void
  serialize(DataStream& stream) const override;

  NODISCARD bool
  deserialize(DataStream& stream) override;

  static constexpr uint32 SERIALIZATION_VERSION = 2;

 protected:
  LightComponent(LightType type, float intensity, bool bCastShadows);

  void
  onRegister() override;

  void
  onUnregister() override;

  void
  onTransformChanged() override;

  /**
   * Writes what only this type has (range, cone) into the item.
   */
  virtual void
  writeTypeValues(LightItem& item) const;

  /**
   * Sends the values to the scene again; call it after changing one.
   */
  void
  updateLightItem();

 private:
  LightType m_type;
  LinearColor m_color{1.0f, 1.0f, 1.0f, 1.0f};
  float m_intensity;
  uint32 m_lightId = 0;
  bool m_bCastShadows;
};

/**
 * Exists to light the whole scene from one direction with parallel rays, like the sun.
 */
class CH_CORE_EXPORT DirectionalLightComponent : public LightComponent
{
 public:
  DirectionalLightComponent();
};

/**
 * Exists to light in every direction from one point, fading to nothing at its range.
 */
class CH_CORE_EXPORT PointLightComponent : public LightComponent
{
 public:
  PointLightComponent();

  void
  setRange(float range);

  NODISCARD FORCEINLINE float
  getRange() const noexcept { return m_range; }

  void
  serialize(DataStream& stream) const override;

  NODISCARD bool
  deserialize(DataStream& stream) override;

 protected:
  void
  writeTypeValues(LightItem& item) const override;

 private:
  float m_range = 10.0f;
};

/**
 * Exists to light a cone from one point, fading to nothing at its range and between its
 * inner and outer angles.
 */
class CH_CORE_EXPORT SpotLightComponent : public LightComponent
{
 public:
  SpotLightComponent();

  void
  setRange(float range);

  NODISCARD FORCEINLINE float
  getRange() const noexcept { return m_range; }

  /**
   * Half angles from the axis. The outer one is kept below 90 degrees and the inner one
   * between zero and the outer one.
   */
  void
  setConeAngles(const Degree& inner, const Degree& outer);

  NODISCARD FORCEINLINE const Degree&
  getInnerConeAngle() const noexcept { return m_innerConeAngle; }

  NODISCARD FORCEINLINE const Degree&
  getOuterConeAngle() const noexcept { return m_outerConeAngle; }

  void
  serialize(DataStream& stream) const override;

  NODISCARD bool
  deserialize(DataStream& stream) override;

 protected:
  void
  writeTypeValues(LightItem& item) const override;

 private:
  float m_range = 10.0f;
  Degree m_innerConeAngle{30.0f};
  Degree m_outerConeAngle{45.0f};
};

DECLARE_TYPE_TRAITS(DirectionalLightComponent)
DECLARE_TYPE_TRAITS(PointLightComponent)
DECLARE_TYPE_TRAITS(SpotLightComponent)

} // namespace chEngineSDK
