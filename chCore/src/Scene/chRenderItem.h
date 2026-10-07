/************************************************************************/
/**
 * @file chRenderItem.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief
 *  One mesh placed in the world, as the renderer sees it.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chMatrix4.h"
#include "chSphereBoxBounds.h"

namespace chEngineSDK {

/**
 * Exists so the renderer reads a flat array instead of walking GameObjects and components.
 * Components add items to their Scene and keep them up to date; Scene::gatherRenderItems
 * returns the ones a camera sees.
 */
struct RenderItem
{
  Matrix4 worldMatrix = Matrix4::IDENTITY;
  SphereBoxBounds worldBounds{Vector3::ZERO, Vector3::ZERO, 0.0f};
  // Null marks a free slot of the scene.
  const Mesh* mesh = nullptr;
  // Null draws with the renderer's default texture.
  const ITexture* texture = nullptr;
};

} // namespace chEngineSDK
