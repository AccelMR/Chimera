/************************************************************************/
/**
 * @file chSkyLightItem.h
 * @author AccelMR
 * @date 2026/10/15
 * @brief What the renderer reads of a sky light.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chMatrix4.h"

namespace chEngineSDK {

/**
 * Exists so the renderer reads the environment of a scene without looking at components:
 * a SkyLightComponent keeps one up to date while it is registered.
 */
struct SkyLightItem
{
  // Equirectangular HDR image; null when the sky light has none or it is not loaded.
  const ITexture* texture = nullptr;
  // Turns a world direction into a direction of the image: the inverse of the rotation of
  // the GameObject, so turning the object turns the environment.
  Matrix4 worldToSky = Matrix4::IDENTITY;
  float intensity = 1.0f;
  // Draws the image behind the scene.
  bool bShowSky = true;
  GameObject* owner = nullptr;
  // When several sky lights are registered, the one with the highest order is used.
  uint32 order = 0;
  // False marks a free slot of the scene.
  bool bInUse = false;
};

} // namespace chEngineSDK
