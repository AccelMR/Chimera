/************************************************************************/
/**
 * @file chModelExpand.h
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  Splits an object that draws a whole model into one object per node or per mesh.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {

/**
 * How a model becomes GameObjects. Stored in files later, so the values are fixed.
 */
enum class ModelExpandMode : uint32
{
  // One object draws the whole model.
  Single = 0,
  // One object per model node, in the model's hierarchy.
  PerNode = 1,
  // One child per mesh, with its pivot at the center of the mesh.
  PerMesh = 2
};

/**
 * Exists so each part of a model can be moved, selected and given its own materials: it
 * swaps the whole-model ModelComponent of an object for child objects that each draw one
 * part. The object itself stays, with its name, transform and place in the tree.
 */
class CH_CORE_EXPORT ModelExpand
{
 public:
  /**
   * False, with nothing changed, when the object has no ModelComponent drawing a whole
   * model or the mode is Single. Material overrides and the visible flag go to the parts.
   */
  static bool
  expand(Scene& scene, GameObject& gameObject, ModelExpandMode mode);
};

} // namespace chEngineSDK
