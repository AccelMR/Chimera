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
 * GameObject and is never saved. Holding the right button flies it like a first person
 * camera (WASD along the view, E up, Q down, Shift faster); it starts
 * only while focused, so ImGui windows keep their keys and clicks.
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

  /**
   * Moves and turns the camera from the input of this frame. Call it once per frame.
   */
  void
  update(float deltaTime);

  FORCEINLINE void
  setFocused(bool focused) { m_bFocused = focused; }

  /**
   * True from the frame the right button starts a flight until it is released, even if
   * the cursor goes over a window meanwhile.
   */
  NODISCARD FORCEINLINE bool
  isFlying() const noexcept { return m_bFlying; }

  void
  setViewportSize(float width, float height);

  /**
   * Looks at the center of bounds from far enough to see all of it, keeping the direction.
   */
  void
  focus(const AABox& bounds);

  void
  setView(const Vector3& position, const Vector3& lookAt);

  NODISCARD FORCEINLINE const Camera&
  getCamera() const { return m_camera; }

  /**
   * Units per second.
   */
  NODISCARD float
  getMoveSpeed() const;

  void
  setMoveSpeed(float speed);

  /**
   * Degrees per pixel the mouse moves.
   */
  NODISCARD float
  getLookSensitivity() const;

  void
  setLookSensitivity(float degreesPerPixel);

  /**
   * How many times faster the camera flies while Shift is held.
   */
  NODISCARD float
  getSprintMultiplier() const;

  void
  setSprintMultiplier(float multiplier);

  /**
   * Writes the speed, sensitivity and sprint to /Saved/Config/Editor.ini, so the next
   * session starts with them.
   */
  void
  saveSettings() const;

  static constexpr float MIN_MOVE_SPEED = 0.05f;
  static constexpr float MAX_MOVE_SPEED = 2000.0f;

 private:
  void
  look(float yawDegrees, float pitchDegrees);

  void
  fly(float deltaTime);

  Camera m_camera;
  bool m_bFocused = false;
  bool m_bFlying = false;
  // The wheel changed the speed during the flight, so it is saved when the flight ends.
  bool m_bSpeedChanged = false;

  // Mouse motion of this frame, applied in update.
  float m_lookDeltaX = 0.0f;
  float m_lookDeltaY = 0.0f;

  HEvent m_mouseWheelListener;
  HEvent m_mouseMoveListener;
};

} // namespace chEngineSDK
