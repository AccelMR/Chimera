/************************************************************************/
/**
 * @file chMeshSplit.h
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  Cuts a mesh into the pieces that do not touch.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class Mesh;

/**
 * Exists so props that a source file merged into one mesh (every pot of a level in one mesh
 * per material) become separate meshes, each with its own bounds and so its own pivot when
 * the model is split into objects.
 */
class CH_CORE_EXPORT MeshSplit
{
 public:
  /**
   * Triangles that share a vertex position are one piece (vertices repeated along UV seams
   * still join), and pieces whose boxes overlap or touch are joined too, so the loose leaves
   * of one plant stay together. Each piece keeps the vertex layout and the material slot,
   * and gets its own vertices. Returns only the mesh itself when it is one piece or has no
   * positions.
   */
  NODISCARD static Vector<SPtr<Mesh>>
  splitParts(const SPtr<Mesh>& mesh);
};

} // namespace chEngineSDK
