/************************************************************************/
/**
 * @file chAnimationPreviewUI.cpp
 * @author AccelMR
 * @date 2026/10/20
 * @brief Window that plays a model's animations away from the open scene.
 */
/************************************************************************/
#include "chAnimationPreviewUI.h"

#include "chAlgorithm.h"
#include "chAnimationClip.h"
#include "chAnimationClipAsset.h"
#include "chAnimationTimelineUI.h"
#include "chAnimatorComponent.h"
#include "chAssetDragDrop.h"
#include "chAssetManager.h"
#include "chConsoleVariable.h"
#include "chEngineConfig.h"
#include "chGameObject.h"
#include "chLogger.h"
#include "chMath.h"
#include "chModel.h"
#include "chModelAsset.h"
#include "chModelPreview.h"
#include "chSkeletonAsset.h"
#include "chStringUtils.h"

#include "imgui.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"

namespace chEngineSDK {
CH_LOG_DECLARE_STATIC(AnimationPreviewLog, All);

namespace {
ConsoleVariable<bool> g_cvarShowAnimationPreview("Editor.ShowAnimationPreview",
                                                 false,
                                                 "Shows the Animation Preview window.",
                                                 "ShowAnimationPreview");

// Size the window gets the first time it opens; ImGui remembers it after.
constexpr float kWindowWidth = 640.0f;
constexpr float kWindowHeight = 600.0f;

struct PreviewState
{
  // What the window shows; either may be null.
  SPtr<AnimationClipAsset> clip;
  // The models and clips the combos offer: made for this skeleton, or with a null one, in
  // this folder.
  UUID skeletonId = UUID::null();
  String folder;
  Vector<SPtr<IAsset>> models;
  Vector<SPtr<IAsset>> clips;

  ModelPreview preview{"AnimationPreview"};
  AnimatorComponent* animator = nullptr;
  bool bFocus = false;
};

PreviewState&
getState()
{
  static PreviewState state;
  return state;
}

/*
 */
void
sortByName(Vector<SPtr<IAsset>>& assets)
{
  Algorithm::sort(assets, [](const SPtr<IAsset>& a, const SPtr<IAsset>& b) {
    return StringView(a->getName()) < StringView(b->getName());
  });
}

/*
 */
bool
ensureLoaded(const SPtr<IAsset>& asset)
{
  if (asset->isLoaded() || AssetManager::instance().syncLoadAsset(asset)) {
    return true;
  }
  CH_LOG_ERROR(AnimationPreviewLog, "Failed to load asset: {0}", asset->getName());
  return false;
}

/*
 */
void
collectChoices(PreviewState& state)
{
  AnimationPreviewUI::findModels(state.skeletonId, state.folder, state.models);
  AnimationPreviewUI::findClips(state.skeletonId, state.folder, state.clips);
}

/*
 * Puts the model in the preview scene again, playing the clip from its start.
 */
void
setModel(PreviewState& state, const SPtr<IAsset>& asset)
{
  state.preview.setModel(asset && ensureLoaded(asset) ? asset->as<ModelAsset>() : nullptr);
  GameObject* object = state.preview.getModelObject();
  state.animator = object ? &object->addComponent<AnimatorComponent>(state.clip) : nullptr;
}

/*
 */
void
setClip(PreviewState& state, const SPtr<IAsset>& asset)
{
  state.clip = asset && ensureLoaded(asset) ? asset->as<AnimationClipAsset>() : nullptr;
  if (state.animator) {
    state.animator->play(state.clip);
    state.animator->setPlaying(true);
  }
}

/*
 */
void
acceptDrop(PreviewState& state)
{
  if (!ImGui::BeginDragDropTarget()) {
    return;
  }
  if (const SPtr<ModelAsset> model = AssetDragDrop::acceptModel()) {
    setModel(state, model);
  }
  if (const SPtr<AnimationClipAsset> clip = AssetDragDrop::acceptAnimation()) {
    setClip(state, clip);
  }
  ImGui::EndDragDropTarget();
}

/*
 */
void
renderToolbar(PreviewState& state)
{
  const float comboWidth = Math::max(ImGui::GetContentRegionAvail().x * 0.35f, 80.0f);
  SPtr<IAsset> picked;
  bool bPicked = false;
  ImGui::SetNextItemWidth(comboWidth);
  AnimationPreviewUI::renderAssetCombo(ICON_FA_CUBE "##Model", state.preview.getModel().get(),
                                       "No model", state.models, picked, bPicked);
  ImGui::SetItemTooltip("Model the clip plays on; drop any model on the view too");
  if (bPicked) {
    setModel(state, picked);
  }

  ImGui::SameLine();
  bPicked = false;
  ImGui::SetNextItemWidth(comboWidth);
  AnimationPreviewUI::renderAssetCombo(ICON_FA_FILM "##Clip", state.clip.get(), "Rest pose",
                                       state.clips, picked, bPicked);
  ImGui::SetItemTooltip("Clip to play; drop any clip on the view too");
  if (bPicked) {
    setClip(state, picked);
  }

  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_EXPAND "##Frame")) {
    state.preview.requestFrame();
  }
  ImGui::SetItemTooltip("Frame the model (F over the view)");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_COG "##Settings")) {
    ImGui::OpenPopup("PreviewSettings");
  }
  ImGui::SetItemTooltip("Preview settings");
  if (ImGui::BeginPopup("PreviewSettings")) {
    ModelPreview::renderSettings();
    ImGui::EndPopup();
  }
}

/*
 */
void
renderView(PreviewState& state)
{
  // The timeline stays under the view, so the view takes what is left above it.
  const ImGuiStyle& style = ImGui::GetStyle();
  const bool bTimeline = state.animator && state.animator->getClip();
  const float timelineHeight =
      bTimeline ? ImGui::GetFrameHeightWithSpacing() * 3.0f +
                      ImGui::GetTextLineHeightWithSpacing() + style.ItemSpacing.y
                : 0.0f;
  const ImVec2 available = ImGui::GetContentRegionAvail();
  state.preview.renderView(ImVec2(available.x, available.y - timelineHeight),
                           "Drop a model or a clip here.");
  acceptDrop(state);

  if (bTimeline) {
    AnimationTimelineUI::render(*state.animator, false);
    if (!state.animator->hasBoundTracks()) {
      ImGui::TextDisabled("No node of this model has the name of a track of the clip.");
    }
  }
}
} // namespace

/*
 */
void
AnimationPreviewUI::render()
{
  if (!g_cvarShowAnimationPreview.get()) {
    return;
  }

  PreviewState& state = getState();
  ImGui::SetNextWindowSize(ImVec2(kWindowWidth, kWindowHeight), ImGuiCond_FirstUseEver);
  if (state.bFocus) {
    ImGui::SetNextWindowFocus();
    state.bFocus = false;
  }
  bool bVisible = true;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
  const bool bOpen =
      ImGui::Begin(ICON_FA_RUNNING "  Animation Preview###AnimationPreview", &bVisible);
  ImGui::PopStyleVar();
  if (bOpen) {
    renderToolbar(state);
    renderView(state);
  }
  ImGui::End();

  if (!bVisible) {
    setWindowVisible(false);
  }
}

/*
 */
void
AnimationPreviewUI::update(float deltaTime)
{
  getState().preview.update(deltaTime);
}

/*
 */
void
AnimationPreviewUI::recordFrame(ICommandList& commandList)
{
  getState().preview.recordFrame(commandList);
}

/*
 */
bool
AnimationPreviewUI::canOpen(const IAsset& asset)
{
  return asset.isTypeOf<ModelAsset>() || asset.isTypeOf<AnimationClipAsset>() ||
         asset.isTypeOf<SkeletonAsset>();
}

/*
 */
void
AnimationPreviewUI::open(const SPtr<IAsset>& asset)
{
  if (!asset || !canOpen(*asset) || !ensureLoaded(asset)) {
    return;
  }

  PreviewState& state = getState();
  state.folder = asset->getAssetPath();
  if (const SPtr<ModelAsset> model = asset->as<ModelAsset>()) {
    state.skeletonId = model->getModel() ? model->getModel()->getSkeletonId() : UUID::null();
    state.clip.reset();
    collectChoices(state);
    setModel(state, model);
  }
  else if (const SPtr<AnimationClipAsset> clip = asset->as<AnimationClipAsset>()) {
    state.skeletonId = clip->getClip() ? clip->getClip()->getSkeletonId() : UUID::null();
    state.clip = clip;
    collectChoices(state);
    // The model shown stays when the clip is made for it too.
    const IAsset* shown = state.preview.getModel().get();
    if (!shown || !Algorithm::containsIf(state.models, [shown](const SPtr<IAsset>& choice) {
          return choice.get() == shown;
        })) {
      setModel(state, state.models.empty() ? nullptr : state.models.front());
    }
    else {
      setClip(state, clip);
    }
  }
  else {
    state.skeletonId = asset->getUUID();
    state.clip.reset();
    collectChoices(state);
    setModel(state, state.models.empty() ? nullptr : state.models.front());
  }

  state.bFocus = true;
  if (!isWindowVisible()) {
    setWindowVisible(true);
  }
}

/*
 */
void
AnimationPreviewUI::close(const IAsset& asset)
{
  PreviewState& state = getState();
  const auto isAsset = [&asset](const SPtr<IAsset>& choice) {
    return choice.get() == &asset;
  };
  Algorithm::removeAllIf(state.models, isAsset);
  Algorithm::removeAllIf(state.clips, isAsset);
  if (state.clip.get() == &asset) {
    setClip(state, nullptr);
  }
  if (state.preview.getModel().get() == &asset) {
    setModel(state, nullptr);
  }
}

/*
 * Models know their skeleton from the references of their file, so finding them loads
 * nothing.
 */
void
AnimationPreviewUI::findModels(const UUID& skeletonId,
                               StringView folder,
                               Vector<SPtr<IAsset>>& outModels)
{
  outModels.clear();
  for (const SPtr<IAsset>& asset : AssetManager::instance().getAllAssets()) {
    if (!asset->isTypeOf<ModelAsset>()) {
      continue;
    }
    const bool bFits = skeletonId.isNull()
                           ? StringUtils::equals(asset->getAssetPath(), folder)
                           : Algorithm::contains(asset->getReferencedAssets(), skeletonId);
    if (bFits) {
      outModels.push_back(asset);
    }
  }
  sortByName(outModels);
}

/*
 */
void
AnimationPreviewUI::findClips(const UUID& skeletonId,
                              StringView folder,
                              Vector<SPtr<IAsset>>& outClips)
{
  outClips.clear();
  for (const SPtr<IAsset>& asset : AssetManager::instance().getAllAssets()) {
    if (!asset->isTypeOf<AnimationClipAsset>()) {
      continue;
    }
    if (skeletonId.isNull()) {
      if (StringUtils::equals(asset->getAssetPath(), folder)) {
        outClips.push_back(asset);
      }
      continue;
    }
    if (!asset->isLoaded() && !AssetManager::instance().syncLoadAsset(asset)) {
      continue;
    }
    const SPtr<AnimationClipAsset> clip = asset->as<AnimationClipAsset>();
    if (clip && clip->getClip() && clip->getClip()->getSkeletonId() == skeletonId) {
      outClips.push_back(asset);
    }
  }
  sortByName(outClips);
}

/*
 */
void
AnimationPreviewUI::renderAssetCombo(const ANSICHAR* label,
                                     const IAsset* current,
                                     const ANSICHAR* noneLabel,
                                     const Vector<SPtr<IAsset>>& choices,
                                     SPtr<IAsset>& outPicked,
                                     bool& outbPicked)
{
  if (!ImGui::BeginCombo(label, current ? current->getName() : noneLabel)) {
    return;
  }
  if (ImGui::Selectable(noneLabel, current == nullptr)) {
    outPicked.reset();
    outbPicked = true;
  }
  for (const SPtr<IAsset>& choice : choices) {
    ImGui::PushID(choice.get());
    if (ImGui::Selectable(choice->getName(), choice.get() == current)) {
      outPicked = choice;
      outbPicked = true;
    }
    ImGui::PopID();
  }
  ImGui::EndCombo();
}

/*
 */
bool
AnimationPreviewUI::isWindowVisible()
{
  return g_cvarShowAnimationPreview.get();
}

/*
 */
void
AnimationPreviewUI::setWindowVisible(bool bVisible)
{
  g_cvarShowAnimationPreview.set(bVisible, ConsoleVariableSource::Console);
  EngineConfig::setUserValue("Editor", "Editor", "ShowAnimationPreview",
                             bVisible ? "true" : "false");
}

/*
 */
void
AnimationPreviewUI::shutDown()
{
  PreviewState& state = getState();
  state.animator = nullptr;
  state.preview.shutDown();
  state.clip.reset();
  state.models.clear();
  state.clips.clear();
}

} // namespace chEngineSDK
