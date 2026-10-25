/************************************************************************/
/**
 * @file chCameraPreviewUI.cpp
 * @author AccelMR
 * @date 2026/10/25
 * @brief What a camera of the open scene sees, shown in the editor.
 */
/************************************************************************/
#include "chCameraPreviewUI.h"

#include "chCamera.h"
#include "chCameraComponent.h"
#include "chConsoleVariable.h"
#include "chEditorScene.h"
#include "chEditorSelection.h"
#include "chEngineConfig.h"
#include "chGameObject.h"
#include "chIGraphicsAPI.h"
#include "chITexture.h"
#include "chITextureView.h"
#include "chImGuiRenderer.h"
#include "chMath.h"
#include "chScene.h"
#include "chSceneRenderer.h"
#include "chStringUtils.h"
#include "chUIHelpers.h"

#include "imgui.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"

namespace chEngineSDK {
namespace {
ConsoleVariable<bool> g_cvarShowCameraPreview("Editor.ShowCameraPreview",
                                              false,
                                              "Shows the Camera Preview window.",
                                              "ShowCameraPreview");
ConsoleVariable<bool> g_cvarPreviewShadows(
    "Editor.CameraPreviewShadows",
    false,
    "Draws shadows in the Object Inspector's camera preview (the window always does).");
ConsoleVariable<bool> g_cvarPreviewSSAO(
    "Editor.CameraPreviewSSAO",
    false,
    "Draws ambient occlusion in the Object Inspector's camera preview (the window always "
    "does).");
ConsoleVariable<float> g_cvarPreviewResolution(
    "Editor.CameraPreviewResolution",
    0.5f,
    "Fraction of its size in pixels the Object Inspector's camera preview is drawn at.");

// The tonemap writes encoded sRGB, the same as into the swap chain, so ImGui shows it as is.
constexpr Format kTargetFormat = Format::R8G8B8A8_UNORM;
constexpr uint32 kMinTargetSize = 16;
constexpr float kMinResolution = 0.25f;
constexpr float kMaxResolution = 1.0f;
// Size the window gets the first time it opens; ImGui remembers it after.
constexpr float kWindowWidth = 640.0f;
constexpr float kWindowHeight = 420.0f;

enum class PreviewImage
{
  None,
  Inspector,
  Window
};

struct PreviewTarget
{
  SPtr<ITexture> texture;
  SPtr<ITextureView> view;
  uint32 width = 0;
  uint32 height = 0;
  // Until it is drawn once its contents are undefined, so the image is not shown.
  bool bDrawn = false;
};

struct PreviewState
{
  // What the window shows: the selected camera, else the last one it showed.
  WeakPtr<GameObject> windowObject;
  WeakPtr<GameObject> pilotObject;

  UniquePtr<SceneRenderer> renderer;
  PreviewTarget inspectorTarget;
  PreviewTarget windowTarget;
  Camera camera;

  // The image shown this frame and whose camera it is; update copies the scene for it and
  // recordFrame draws it. The window wins when both ask.
  PreviewImage shown = PreviewImage::None;
  WeakPtr<GameObject> shownObject;
  PreviewImage extracted = PreviewImage::None;
  // The object whose image the window drew last; the Inspector, drawn before the window,
  // reads the last frame's and does not draw the same camera again.
  const GameObject* windowShown = nullptr;
  bool bFocusWindow = false;
};

PreviewState&
getState()
{
  static PreviewState state;
  return state;
}

/*
 * The object while it is in the open scene, so a camera of a scene that was closed, or of
 * the edited scene during Play, is never drawn.
 */
SPtr<GameObject>
findInOpenScene(const WeakPtr<GameObject>& object)
{
  SPtr<GameObject> locked = object.lock();
  if (!locked || locked->getScene() != EditorScene::getScene().get()) {
    return nullptr;
  }
  return locked;
}

/*
 */
void
saveBool(StringView key, bool bValue)
{
  EngineConfig::setUserValue("Editor", "Editor", key, bValue ? "true" : "false");
}

/*
 * A game starts in a window of the editor's shape, so both images keep it.
 */
float
getAspectRatio()
{
  const ImVec2 size = ImGui::GetMainViewport()->Size;
  return size.y > 0.0f ? Math::max(size.x / size.y, 0.1f) : 16.0f / 9.0f;
}

/*
 */
void
updateTarget(PreviewTarget& target, uint32 width, uint32 height)
{
  width = Math::max(width, kMinTargetSize);
  height = Math::max(height, kMinTargetSize);
  if (target.texture && target.width == width && target.height == height) {
    return;
  }
  // While a splitter or the window is dragged the size changes every frame; the old image is
  // stretched until the button is released.
  if (target.texture && target.bDrawn && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    return;
  }

  // The old texture is freed by the deferred deletion once the GPU is done with it, and
  // the view points at it, so the view goes first.
  target.view.reset();
  target.texture = IGraphicsAPI::instance().createTexture(
      {.format = kTargetFormat,
       .width = width,
       .height = height,
       .usage = TextureUsageFlags(TextureUsage::ColorAttachment) | TextureUsage::Sampled});
  target.view = target.texture->createView();
  target.width = width;
  target.height = height;
  target.bDrawn = false;
}

/*
 * Draws the image at the cursor, size in ImGui units, and asks for it to be drawn this
 * frame.
 */
void
renderImage(PreviewState& state,
            PreviewImage image,
            const SPtr<GameObject>& object,
            const ImVec2& size,
            float resolution)
{
  PreviewTarget& target =
      image == PreviewImage::Window ? state.windowTarget : state.inspectorTarget;
  const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
  updateTarget(target, static_cast<uint32>(size.x * scale.x * resolution),
               static_cast<uint32>(size.y * scale.y * resolution));

  ImGui::Dummy(size);
  const ImVec2 min = ImGui::GetItemRectMin();
  const ImVec2 max = ImGui::GetItemRectMax();
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  const uint64 textureId = target.bDrawn ? ImGuiRenderer::getTextureId(*target.texture) : 0;
  if (textureId != 0) {
    drawList->AddImage(static_cast<ImTextureID>(textureId), min, max);
  }
  else {
    drawList->AddRectFilled(min, max, ImGui::GetColorU32(ImGuiCol_FrameBg));
  }

  if (image == PreviewImage::Window || state.shown != PreviewImage::Window) {
    state.shown = image;
    state.shownObject = object;
  }
}

/*
 */
void
renderSettings()
{
  ImGui::TextUnformatted("Object Inspector preview");
  bool bShadows = g_cvarPreviewShadows.get();
  if (ImGui::Checkbox("Shadows", &bShadows)) {
    g_cvarPreviewShadows.set(bShadows, ConsoleVariableSource::Console);
    saveBool("CameraPreviewShadows", bShadows);
  }
  ImGui::SetItemTooltip("%s", g_cvarPreviewShadows.getHelp().c_str());
  bool bSSAO = g_cvarPreviewSSAO.get();
  if (ImGui::Checkbox("Ambient Occlusion", &bSSAO)) {
    g_cvarPreviewSSAO.set(bSSAO, ConsoleVariableSource::Console);
    saveBool("CameraPreviewSSAO", bSSAO);
  }
  ImGui::SetItemTooltip("%s", g_cvarPreviewSSAO.getHelp().c_str());
  float resolution = g_cvarPreviewResolution.get();
  ImGui::SetNextItemWidth(160.0f);
  if (ImGui::SliderFloat("Resolution", &resolution, kMinResolution, kMaxResolution, "%.2f")) {
    g_cvarPreviewResolution.set(resolution, ConsoleVariableSource::Console);
  }
  ImGui::SetItemTooltip("%s", g_cvarPreviewResolution.getHelp().c_str());
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    ANSICHAR buffer[StringUtils::MAX_FLOAT_CHARS];
    EngineConfig::setUserValue("Editor", "Editor", "CameraPreviewResolution",
                               StringUtils::toChars(buffer, g_cvarPreviewResolution.get()));
  }
}

/*
 */
void
renderPilotButton(const SPtr<GameObject>& object)
{
  const bool bPiloting = CameraPreviewUI::getPilotedCamera() &&
                         CameraPreviewUI::getPilotedCamera()->getOwner() == object.get();
  if (bPiloting) {
    if (ImGui::Button(ICON_FA_STOP "  Stop Piloting")) {
      CameraPreviewUI::stopPiloting();
    }
    ImGui::SetItemTooltip("Gives the viewport back to the editor camera (Esc over it)");
    return;
  }
  ImGui::BeginDisabled(EditorScene::isPlaying());
  if (ImGui::Button(ICON_FA_CROSSHAIRS "  Pilot")) {
    CameraPreviewUI::startPiloting(object);
  }
  ImGui::EndDisabled();
  ImGui::SetItemTooltip(
      "The viewport looks through this camera; flying the editor camera moves it");
}

/*
 * Why a camera shows no image, or null when it can show one.
 */
const ANSICHAR*
getHiddenReason(const GameObject& object)
{
  if (EditorScene::isPlaying()) {
    return "Hidden during Play.";
  }
  const CameraComponent* piloted = CameraPreviewUI::getPilotedCamera();
  if (piloted && piloted->getOwner() == &object) {
    return "The viewport looks through this camera.";
  }
  return nullptr;
}
} // namespace

/*
 */
void
CameraPreviewUI::render()
{
  PreviewState& state = getState();
  state.windowShown = nullptr;
  if (!g_cvarShowCameraPreview.get()) {
    return;
  }

  // Follows the selection, and keeps its camera while something else is selected.
  const SPtr<GameObject>& selected = EditorSelection::getSelectedGameObject();
  if (selected && selected->getComponent<CameraComponent>()) {
    state.windowObject = selected;
  }

  ImGui::SetNextWindowSize(ImVec2(kWindowWidth, kWindowHeight), ImGuiCond_FirstUseEver);
  if (state.bFocusWindow) {
    ImGui::SetNextWindowFocus();
    state.bFocusWindow = false;
  }
  bool bVisible = true;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
  const bool bOpen =
      ImGui::Begin(ICON_FA_VIDEO "  Camera Preview###CameraPreview", &bVisible);
  ImGui::PopStyleVar();
  if (bOpen) {
    SPtr<GameObject> object = findInOpenScene(state.windowObject);
    if (object && !object->getComponent<CameraComponent>()) {
      object.reset();
    }

    if (object) {
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted(object->getName().c_str());
      ImGui::SameLine();
      renderPilotButton(object);
    }
    else {
      ImGui::AlignTextToFramePadding();
      ImGui::TextDisabled("Select a camera to see what it sees.");
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_COG "##CameraPreviewSettings")) {
      ImGui::OpenPopup("CameraPreviewSettings");
    }
    ImGui::SetItemTooltip("Settings");
    if (ImGui::BeginPopup("CameraPreviewSettings")) {
      renderSettings();
      ImGui::EndPopup();
    }

    if (object) {
      if (const ANSICHAR* reason = getHiddenReason(*object)) {
        ImGui::TextDisabled("%s", reason);
      }
      else {
        // Letterboxed to the editor window's shape, centered.
        const ImVec2 available = ImGui::GetContentRegionAvail();
        const float aspect = getAspectRatio();
        ImVec2 size(available.x, available.x / aspect);
        if (size.y > available.y) {
          size = ImVec2(available.y * aspect, available.y);
        }
        size = ImVec2(Math::max(size.x, 1.0f), Math::max(size.y, 1.0f));
        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + (available.x - size.x) * 0.5f,
                                   ImGui::GetCursorPosY() + (available.y - size.y) * 0.5f));
        renderImage(state, PreviewImage::Window, object, size, 1.0f);
        state.windowShown = object.get();
      }
    }
  }
  ImGui::End();

  if (!bVisible) {
    setWindowVisible(false);
  }
}

/*
 */
void
CameraPreviewUI::renderInspectorView(const SPtr<GameObject>& object)
{
  PreviewState& state = getState();
  if (const ANSICHAR* reason = getHiddenReason(*object)) {
    ImGui::TextDisabled("%s", reason);
  }
  else if (state.windowShown == object.get()) {
    ImGui::TextDisabled("Shown in the Camera Preview window.");
  }
  else {
    const float width = Math::max(ImGui::GetContentRegionAvail().x, 1.0f);
    const float resolution =
        Math::clamp(g_cvarPreviewResolution.get(), kMinResolution, kMaxResolution);
    renderImage(state, PreviewImage::Inspector, object,
                ImVec2(width, Math::max(width / getAspectRatio(), 1.0f)), resolution);
  }

  renderPilotButton(object);
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_EXTERNAL_LINK_ALT "  Open Preview")) {
    open(object);
  }
  ImGui::SetItemTooltip("Shows this camera big, with the viewport's quality");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_COG "##CameraPreviewSettings")) {
    ImGui::OpenPopup("CameraPreviewSettings");
  }
  ImGui::SetItemTooltip("Settings");
  if (ImGui::BeginPopup("CameraPreviewSettings")) {
    renderSettings();
    ImGui::EndPopup();
  }
}

/*
 */
void
CameraPreviewUI::update()
{
  PreviewState& state = getState();
  const PreviewImage shown = state.shown;
  state.shown = PreviewImage::None;
  const SPtr<GameObject> object = findInOpenScene(state.shownObject);
  state.shownObject.reset();
  if (shown == PreviewImage::None || !object) {
    return;
  }
  const CameraComponent* camera = object->getComponent<CameraComponent>();
  if (!camera) {
    return;
  }

  if (!state.renderer) {
    // Lit always, what the game shows whatever the viewport's view mode; the main renderer
    // times the GPU.
    state.renderer = chMakeUnique<SceneRenderer>(
        SceneRendererDesc{.bProfileGpu = false, .viewMode = ViewMode::Lit});
  }
  const bool bWindow = shown == PreviewImage::Window;
  const PreviewTarget& target = bWindow ? state.windowTarget : state.inspectorTarget;
  state.renderer->setShadowsEnabled(bWindow || g_cvarPreviewShadows.get());
  state.renderer->setScreenSpaceAOEnabled(bWindow || g_cvarPreviewSSAO.get());
  camera->fillCamera(state.camera, static_cast<float>(target.width),
                     static_cast<float>(target.height));
  state.renderer->extract(*EditorScene::getScene(), state.camera,
                          chUIHelpers::UIHelpers::rendererColor);
  state.extracted = shown;
}

/*
 */
void
CameraPreviewUI::recordFrame(ICommandList& commandList)
{
  PreviewState& state = getState();
  if (state.extracted == PreviewImage::None) {
    return;
  }
  PreviewTarget& target =
      state.extracted == PreviewImage::Window ? state.windowTarget : state.inspectorTarget;
  state.extracted = PreviewImage::None;

  RenderGraph& graph = state.renderer->beginFrame();
  // Drawn again every frame, so nothing of the last one is kept.
  const RGTextureHandle output =
      graph.importTexture("CameraPreview", *target.texture, *target.view,
                          ResourceState::Undefined, ResourceState::ShaderRead);
  state.renderer->addScenePasses(output);
  state.renderer->execute(commandList);
  target.bDrawn = true;
}

/*
 */
void
CameraPreviewUI::open(const SPtr<GameObject>& gameObject)
{
  PreviewState& state = getState();
  state.windowObject = gameObject;
  state.bFocusWindow = true;
  if (!isWindowVisible()) {
    setWindowVisible(true);
  }
}

/*
 */
void
CameraPreviewUI::startPiloting(const SPtr<GameObject>& gameObject)
{
  // Objects inside a prefab instance come from the prefab and cannot be moved.
  const GameObject* instance = gameObject->getOutermostPrefabInstance();
  if (EditorScene::isPlaying() || !gameObject->getComponent<CameraComponent>() ||
      (instance && instance != gameObject.get())) {
    return;
  }
  getState().pilotObject = gameObject;
}

/*
 */
void
CameraPreviewUI::stopPiloting()
{
  getState().pilotObject.reset();
}

/*
 */
CameraComponent*
CameraPreviewUI::getPilotedCamera()
{
  PreviewState& state = getState();
  const SPtr<GameObject> object = findInOpenScene(state.pilotObject);
  CameraComponent* camera = object ? object->getComponent<CameraComponent>() : nullptr;
  if (!camera) {
    state.pilotObject.reset();
  }
  return camera;
}

/*
 */
bool
CameraPreviewUI::isWindowVisible()
{
  return g_cvarShowCameraPreview.get();
}

/*
 */
void
CameraPreviewUI::setWindowVisible(bool bVisible)
{
  g_cvarShowCameraPreview.set(bVisible, ConsoleVariableSource::Console);
  saveBool("ShowCameraPreview", bVisible);
}

/*
 */
void
CameraPreviewUI::shutDown()
{
  PreviewState& state = getState();
  state.renderer.reset();
  state.inspectorTarget = {};
  state.windowTarget = {};
  state.windowObject.reset();
  state.pilotObject.reset();
  state.shownObject.reset();
  state.shown = PreviewImage::None;
  state.extracted = PreviewImage::None;
  state.windowShown = nullptr;
}

} // namespace chEngineSDK
