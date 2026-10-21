/************************************************************************/
/**
 * @file chAnimationTimelineUI.h
 * @author AccelMR
 * @date 2026/10/20
 * @brief Controls that play the clip of a model.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class ModelComponent;

/**
 * Exists so the Object Inspector and the Animation Preview play a clip with the same
 * controls: jump to the start or the end, one key back or forward, play and pause, loop,
 * speed, and a time slider that also shows the key under it.
 */
class AnimationTimelineUI
{
 public:
  /**
   * Draws nothing when the component plays no clip. With bRecordUndo, each change that is
   * saved with the scene is first recorded as an undo step of the component's GameObject;
   * the time is never saved, so moving it is no edit.
   */
  static void
  render(ModelComponent& component, bool bRecordUndo);
};

} // namespace chEngineSDK
