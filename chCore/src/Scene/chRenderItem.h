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
class Material;

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
  // Null draws with the renderer's default look.
  const Material* material = nullptr;
  // The object the item draws for, so a hit on the item (picking) finds it.
  GameObject* owner = nullptr;
  // Meshes with a skin: one matrix per entry of the mesh's MeshSkin::boneNodes, taking a
  // vertex from the mesh's space to the model's in the current pose; worldMatrix then only
  // places the model. Null for meshes drawn as stored. The scene's items point into their
  // component, the copies of a RenderFrame into the frame.
  const Matrix4* skinMatrices = nullptr;
  // Changes whenever skinMatrices change and is never reused, so the renderer skins the
  // vertices again only then.
  uint64 skinRevision = 0;
  // Written by RenderFrame::extract on its copies: the item's slot in the scene, so the
  // renderer can keep data per item across frames.
  uint32 sceneSlot = 0;
  // Written by the renderer on a frame's copies of skinned items: the vertices after
  // skinning. A skinned item without them is not drawn.
  const IBuffer* skinnedVertices = nullptr;
};

} // namespace chEngineSDK
