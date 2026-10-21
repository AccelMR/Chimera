/************************************************************************/
/**
 * @file chAnimationTimelineUI.cpp
 * @author AccelMR
 * @date 2026/10/20
 * @brief Controls that play the clip of a model.
 */
/************************************************************************/
#include "chAnimationTimelineUI.h"

#include "chAnimationClip.h"
#include "chEditorScene.h"
#include "chMath.h"
#include "chModelComponent.h"

#include "imgui.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"

namespace chEngineSDK {
namespace {
// A time this close to a key counts as on it, so rounding never skips a key when stepping.
constexpr float kOnKeyTolerance = 1e-3f;

void
recordChange(ModelComponent& component, bool bRecordUndo, const ANSICHAR* action)
{
  if (bRecordUndo) {
    EditorScene::recordChange(*component.getOwner(), action);
  }
}

/*
 * Jumping or stepping stops the clip, as in Unreal, so the chosen key stays on screen.
 */
void
jumpTo(ModelComponent& component, bool bRecordUndo, float time)
{
  if (component.isPlaying()) {
    recordChange(component, bRecordUndo, "Pause Animation");
    component.setPlaying(false);
  }
  component.setAnimationTime(time);
}
} // namespace

/*
 */
void
AnimationTimelineUI::render(ModelComponent& component, bool bRecordUndo)
{
  const SPtr<AnimationClip>& clip = component.getAnimation();
  if (!clip) {
    return;
  }

  const float duration = clip->getDuration();
  const float keyTime = clip->getSampleRate() > 0.0f ? 1.0f / clip->getSampleRate() : 0.0f;
  const uint32 lastKey = clip->getFrameCount() > 0 ? clip->getFrameCount() - 1 : 0;
  const float time = component.getAnimationTime();
  // The key at or before the time.
  uint32 key = 0;
  if (keyTime > 0.0f) {
    key = static_cast<uint32>(Math::max(Math::floor(time / keyTime + kOnKeyTolerance), 0.0f));
    key = Math::min(key, lastKey);
  }
  const bool bOnKey = Math::abs(time - static_cast<float>(key) * keyTime) <
                      kOnKeyTolerance * Math::max(keyTime, 1e-3f);

  if (ImGui::Button(ICON_FA_FAST_BACKWARD "##Start")) {
    jumpTo(component, bRecordUndo, 0.0f);
  }
  ImGui::SetItemTooltip("Go to the start");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_STEP_BACKWARD "##Back")) {
    // Between two keys the earlier one is the step back.
    const uint32 target = bOnKey && key > 0 ? key - 1 : key;
    jumpTo(component, bRecordUndo, static_cast<float>(target) * keyTime);
  }
  ImGui::SetItemTooltip("One key back");
  ImGui::SameLine();
  const bool bPlaying = component.isPlaying();
  if (ImGui::Button(bPlaying ? ICON_FA_PAUSE "##PlayPause" : ICON_FA_PLAY "##PlayPause")) {
    recordChange(component, bRecordUndo, bPlaying ? "Pause Animation" : "Play Animation");
    component.setPlaying(!bPlaying);
  }
  ImGui::SetItemTooltip(bPlaying ? "Pause" : "Play");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_STEP_FORWARD "##Forward")) {
    const uint32 target = Math::min(key + 1, lastKey);
    jumpTo(component, bRecordUndo, Math::min(static_cast<float>(target) * keyTime, duration));
  }
  ImGui::SetItemTooltip("One key forward");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_FAST_FORWARD "##End")) {
    jumpTo(component, bRecordUndo, duration);
  }
  ImGui::SetItemTooltip("Go to the end");
  ImGui::SameLine();
  bool bLooping = component.isLooping();
  if (ImGui::Checkbox("Loop", &bLooping)) {
    recordChange(component, bRecordUndo, "Edit Loop");
    component.setLooping(bLooping);
  }

  float speed = component.getPlaybackSpeed();
  if (ImGui::DragFloat("Speed", &speed, 0.01f, -10.0f, 10.0f, "%.2fx",
                       ImGuiSliderFlags_AlwaysClamp)) {
    recordChange(component, bRecordUndo, "Edit Speed");
    component.setPlaybackSpeed(speed);
  }

  float newTime = time;
  if (ImGui::SliderFloat("Time", &newTime, 0.0f, duration, "%.2f s")) {
    component.setAnimationTime(newTime);
  }
  ImGui::TextDisabled("Key %u of %u, %.2f of %.2f s", key, lastKey, time, duration);
}

} // namespace chEngineSDK
