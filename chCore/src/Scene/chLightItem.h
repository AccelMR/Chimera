/************************************************************************/
/**
 * @file chLightItem.h
 * @author AccelMR
 * @date 2026/10/13
 * @brief
 *  A light as the renderer sees it, kept in a flat list by the Scene.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chLinearColor.h"
#include "chVector3.h"

namespace chEngineSDK {

enum class LightType : uint32
{
  Directional,
  Point,
  Spot
};

/**
 * Exists so the renderer reads lights from a flat array instead of walking GameObjects and
 * components. Light components add one to their Scene and keep it up to date;
 * Scene::gatherLights returns the ones that can reach what a camera sees.
 */
struct LightItem
{
  LightType type = LightType::Directional;
  // Linear color already multiplied by the intensity.
  LinearColor color{0.0f, 0.0f, 0.0f, 1.0f};
  Vector3 position = Vector3::ZERO;
  // Where the light travels, unit length. Unused by point lights.
  Vector3 direction = Vector3::FORWARD;
  // Distance at which point and spot lights fade to nothing.
  float range = 0.0f;
  // Cosines of the spot cone half angles: full light inside the inner one, none outside the
  // outer one.
  float innerConeCos = 1.0f;
  float outerConeCos = 0.0f;
  // False marks a free slot of the scene.
  bool bInUse = false;
};

} // namespace chEngineSDK
