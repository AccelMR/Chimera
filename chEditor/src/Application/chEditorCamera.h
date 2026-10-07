/************************************************************************/
/**
 * @file chEditorCamera.h
 * @author AccelMR
 * @date 2026/10/07
 * @details
 *  Camera of the editor viewport, moved with the mouse and keyboard.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chCamera.h"
#include "chEventSystem.h"

namespace chEngineSDK {

/**
 * Exists because the viewport camera belongs to the editor, not to the scene: it is not a
 * GameObject and is never saved. It listens to input only while focused, so ImGui windows
 * keep their keys and clicks.
 */
class EditorCamera
{
 public:
  EditorCamera(float viewportWidth, float viewportHeight);

  EditorCamera(const EditorCamera&) = delete;

  EditorCamera&
  operator=(const EditorCamera&) = delete;

  /**
   * The listeners capture this object, so it must not move while they are bound.
   */
  void
  bindInputEvents();

  void
  unbindInputEvents();

  FORCEINLINE void
  setFocused(bool focused) { m_bFocused = focused; }

  void
  setViewportSize(float width, float height);

  /**
   * Looks at the center of bounds from far enough to see all of it, keeping the direction.
   */
  void
  focus(const AABox& bounds);

  NODISCARD FORCEINLINE const Camera&
  getCamera() const { return m_camera; }

 private:
  Camera m_camera;
  bool m_bFocused = false;

  HEvent m_keyDownListener;
  HEvent m_mouseWheelListener;
  HEvent m_mouseMoveListener;
};

} // namespace chEngineSDK
