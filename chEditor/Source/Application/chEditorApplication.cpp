/************************************************************************/
/**
 * @file chEditorApplication.cpp
 * @author AccelMR
 * @date 2025/07/07
 * @details
 *  Chimera Editor application class.
 */
/************************************************************************/

#include "chEditorApplication.h"
#include "chAssetDragDrop.h"
#include "chAssetManager.h"
#include "chCameraComponent.h"
#include "chEditorCamera.h"
#include "chEditorScene.h"
#include "chEditorTheme.h"
#include "chConsoleVariable.h"
#include "chInputManager.h"
#include "chDynamicLibManager.h"
#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chGameObject.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chISwapChain.h"
#include "chITexture.h"
#include "chITextureView.h"
#include "chImGuiRenderer.h"
#include "chLightComponent.h"
#include "chLogger.h"
#include "chMath.h"
#include "chModelAsset.h"
#include "chPrefabAsset.h"
#include "chPath.h"
#include "chPlane.h"
#include "chProfiler.h"
#include "chProfilerUI.h"
#include "chQuaternion.h"
#include "chRayCast.h"
#include "chRotator.h"
#include "chUIHelpers.h"
#include "chViewportGrid.h"
#include "chViewportTools.h"
#include "chImportDialog.h"
#include "chPackageDialog.h"
#include "chProjectBrowserUI.h"
#include "chScene.h"
#include "chSceneManager.h"
#include "chScriptDefinitionsUI.h"
#include "chScriptManager.h"
#include "chStringUtils.h"

#include "chContentAssetUI.h"
#include "chEditorSelection.h"
#include "chMainMenuBarUI.h"
#include "chOutputLogUI.h"
#include "chSceneGraphUI.h"
#include "chInspectorUI.h"
#include "chAssetInspectorUI.h"
#include "chAnimationPreviewUI.h"
#include "chAssetThumbnails.h"

#if USING(CH_CODECS)
#include "chAssetCodec.h"
#include "chAssetCodecManager.h"
#endif // USING(CH_CODECS)

#include "imgui.h"
#include "imgui_internal.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"

#if USING(CH_DISPLAY_SDL3)
#include "imgui_impl_sdl3.h"
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_video.h>
#endif // USING(CH_DISPLAY_SDL3)

CH_LOG_DECLARE_STATIC(EditorApp, All);

namespace chEngineSDK {
using namespace chEngineSDK::chUIHelpers;

namespace {
// A drop meets the ground at most this many look at distances away; farther is too close
// to the horizon to place anything usefully.
constexpr float kMaxDropDistanceScale = 10.0f;
// Parts of the main window the default layout gives the side panels and the bottom one.
constexpr float kLayoutLeftFraction = 0.18f;
constexpr float kLayoutRightFraction = 0.25f;
constexpr float kLayoutBottomFraction = 0.3f;
// A texture needs some pixels even while its panel is squeezed to nothing.
constexpr uint32 kMinViewportSize = 16;
constexpr ImVec4 kPlayingBorderColor(0.35f, 0.8f, 0.35f, 1.0f);
constexpr ImVec4 kPausedBorderColor(0.95f, 0.75f, 0.2f, 1.0f);

ConsoleVariable<float> g_cvarPlayBorderThickness(
    "Editor.PlayBorderThickness",
    3.0f,
    "Width in pixels of the border around the viewport while playing (green) or paused "
    "(yellow); 0 draws none.");
} // namespace


/*
 */
EditorApplication::EditorApplication() {
  CH_LOG_INFO(EditorApp, "Creating EditorApplication instance.");
}

/*
 */
EditorApplication::~EditorApplication() {}

/*
 */
void
EditorApplication::onPostInitialize() {
  CH_LOG_INFO(EditorApp, "Post-initialization of EditorApplication.");

  initializeEditorComponents();
  bindEvents();
}

/*
 */
void
EditorApplication::destroyModules()
{
  CH_LOG_INFO(EditorApp, "EditorApplication destroying modules.");
  saveWindowPlacement("Editor");
  // Its worker runs codec code and uses the codec manager.
  ImportDialog::shutDown();
  PackageDialog::shutDown();
  if (IGraphicsAPI::isStarted()) {
    IGraphicsAPI::instance().waitIdle();
  }

  m_onKeyPressedEvent.disconnect();
  m_onKeyReleasedEvent.disconnect();
  m_updateInjection.disconnect();

  // The renderer gives its textures back to the ImGui context, and the SDL backend uses the
  // window, which the parent destroys.
  m_imguiRenderer.reset();
  UIHelpers::shutDownImGui();

  m_inspectorUI.reset();
  AssetInspectorUI::shutDown();
  AnimationPreviewUI::shutDown();
  AssetThumbnails::shutDown();
  m_sceneGraphUI.reset();
  m_outputLogUI.reset();
  m_mainMenuBar.reset();
  m_contentAssetUI.reset();
  m_viewportTools.reset();

  m_editorCamera.reset();
  m_viewportGrid.reset();
  m_viewportTargetView.reset();
  m_viewportTarget.reset();
  m_sceneRenderer.reset();
  EditorSelection::setSelectedGameObject(nullptr);
  EditorScene::shutDown();

  if (SceneManager::isStarted()) {
    SceneManager::shutDown();
  }
  // Assets hold GPU buffers and textures, so they go while the graphics API still exists.
  if (AssetManager::isStarted()) {
    AssetManager::shutDown();
  }
  // After the assets: a scene asset owns its scene, whose script components run onDestroy
  // when it closes.
  if (ScriptManager::isStarted()) {
    ScriptManager::shutDown();
  }
#if USING(CH_CODECS)
  // The codec objects live in the codec libraries, which stay loaded until the parent
  // shuts down the library manager.
  if (AssetCodecManager::isStarted()) {
    AssetCodecManager::shutDown();
  }
#endif // USING(CH_CODECS)

  WindowedApplication::destroyModules();
}

/*
 */
void
EditorApplication::fixedUpdate(const float deltaTime)
{
  m_fixedStep = deltaTime;
  if (!EditorScene::isPaused()) {
    SceneManager::instance().fixedUpdate(deltaTime);
  }
}

/*
 */
void
EditorApplication::update(const float deltaTime)
{
  // A minimized window draws nothing, so no UI frame is built for it either.
  m_bViewportDrawn = false;
  m_bViewportHovered = false;
  m_bViewportFocused = false;
  m_bFrameHasFps = false;
  if (UIHelpers::bRenderImGui && !getDisplaySurface()->isMinimized()) {
    CH_PROFILE_SCOPE("BuildUI");
    renderUI();
  }
  else if (ProfilerUI::isFullscreenFpsVisible() && !getDisplaySurface()->isMinimized()) {
    // F11 hides every window; a frame with only the counter keeps the cost of the UI out of
    // what it measures.
    UIHelpers::newFrame();
    ProfilerUI::renderFullscreenFps();
    ImGui::Render();
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      ImGui::UpdatePlatformWindows();
    }
    m_bFrameHasFps = true;
  }

  // With the UI the scene fills the viewport panel, without it (F11) the whole window.
  const ISwapChain& swapChain = *getSwapChain();
  const bool bPanel = UIHelpers::bRenderImGui && m_bViewportDrawn;
  const uint32 width = bPanel ? m_viewportTarget->getWidth() : swapChain.getWidth();
  const uint32 height = bPanel ? m_viewportTarget->getHeight() : swapChain.getHeight();
  if (width != m_viewportWidth || height != m_viewportHeight) {
    m_viewportWidth = width;
    m_viewportHeight = height;
    m_editorCamera->setViewportSize(static_cast<float>(width), static_cast<float>(height));
  }
  ImportDialog::update();
  Path chosenProject;
  if (ProjectBrowserUI::takeChosenProject(chosenProject) &&
      !(FileSystem::absolutePath(chosenProject) ==
        FileSystem::absolutePath(EnginePaths::getProjectFile()))) {
    EditorScene::requestOpenProject(chosenProject);
  }
  // A drag or a text field still going on keeps its changes in one undo step.
  EditorScene::applyPending(UIHelpers::bRenderImGui &&
                            (ImGui::IsAnyItemActive() || m_viewportTools->isUsingMouse()));
  updateGameInput();
  if (EditorScene::shouldQuit()) {
    // Another project opens in a new run, once this one has closed.
    if (!EditorScene::getProjectToOpen().empty()) {
      requestRelaunch(ProjectBrowserUI::getLaunchArguments(EditorScene::getProjectToOpen()));
    }
    quit();
  }
  updateWindowTitle();
  Scene& scene = *EditorScene::getScene();

  // A flight only starts with the mouse over the scene image. The keyboard is not checked: a
  // panel keeps the keyboard focus after any click, and the camera only reads keys while
  // flying.
  // Looking through the game's camera, the editor camera stays where it is.
  m_editorCamera->setFocused(!m_bGameView &&
                             (!UIHelpers::bRenderImGui ||
                              (m_bViewportHovered && !m_viewportTools->isUsingMouse())));
  const bool bWasFlying = m_editorCamera->isFlying();
  m_editorCamera->update(deltaTime);
  if (m_editorCamera->isFlying() != bWasFlying) {
    onCameraFlightChanged(m_editorCamera->isFlying());
  }

  {
    CH_PROFILE_SCOPE("SceneUpdate");
    // A step runs the paused game as one fixed step, so stepping gives the same frames
    // whatever the editor's frame rate.
    SceneManager& sceneManager = SceneManager::instance();
    if (EditorScene::takeStep()) {
      sceneManager.fixedUpdate(m_fixedStep);
      sceneManager.update(m_fixedStep);
    }
    else if (!EditorScene::isPaused()) {
      sceneManager.update(deltaTime);
    }
  }

  // After the UI and the scene update, which may have moved or added objects.
  {
    CH_PROFILE_SCOPE("UpdateTransforms");
    scene.updateTransforms();
  }
  updateViewCamera(scene);

  // Like the grid, the editor's lines hide with the rest of the UI, and while the viewport
  // shows the game.
  m_bFrameHasUI = UIHelpers::bRenderImGui;
  m_bFrameDrawsViewport = bPanel;
  m_editorLines.clear();
  if (m_bFrameHasUI && !m_bGameView) {
    ViewportTools::addSelectionBounds(scene, m_editorLines);
    ViewportTools::addSelectionSkeletons(m_editorLines);
  }

  m_sceneRenderer->extract(scene, getViewCamera(), UIHelpers::rendererColor, &m_editorLines);
  AnimationPreviewUI::update(deltaTime);
  AssetThumbnails::update();
}

/*
 */
void
EditorApplication::onRender(ICommandList& commandList, const ISwapChain& swapChain)
{
  // First, so their textures are drawn before the UI pass shows them.
  AnimationPreviewUI::recordFrame(commandList);
  AssetThumbnails::recordFrame(commandList);
  RenderGraph& graph = m_sceneRenderer->beginFrame();
  const RGTextureHandle backBuffer =
      graph.importTexture("BackBuffer", swapChain.getCurrentTexture(),
                          swapChain.getCurrentTextureView(), ResourceState::Undefined,
                          ResourceState::Present);

  // F11 hides the editor, grid included, and the scene fills the window like the game.
  if (!m_bFrameHasUI) {
    m_sceneRenderer->addScenePasses(backBuffer);
    if (m_bFrameHasFps) {
      addImGuiPass(graph, backBuffer, LoadOp::Load, swapChain.getFormat());
    }
    m_sceneRenderer->execute(commandList);
    return;
  }

  // Behind another tab the panel shows no scene, so none is drawn.
  if (m_bFrameDrawsViewport) {
    const RGTextureHandle sceneImage =
        graph.importTexture("Viewport", *m_viewportTarget, *m_viewportTargetView,
                            ResourceState::Undefined, ResourceState::ShaderRead);
    const RGTextureHandle sceneDepth = m_sceneRenderer->addScenePasses(sceneImage);
    if (ViewportGrid::isVisible() && !m_bGameView) {
      m_viewportGrid->addPass(graph, m_sceneRenderer->getFrame().getCamera(), sceneImage,
                              sceneDepth);
    }
  }
  // The dockspace covers the whole window, so nothing of the clear shows.
  addImGuiPass(graph, backBuffer, LoadOp::Clear, swapChain.getFormat());

  m_sceneRenderer->execute(commandList);

  CH_PROFILE_SCOPE("FloatingWindows");
  m_imguiRenderer->renderFloatingWindows(commandList);
}

/*
 */
void
EditorApplication::addImGuiPass(RenderGraph& graph,
                                RGTextureHandle backBuffer,
                                LoadOp loadOp,
                                Format format)
{
  graph.addPass("EditorUI")
      .writeColor(backBuffer, loadOp)
      .setExecute([this, format](RenderPassContext& context) {
        m_imguiRenderer->render(context.getCommandList(), *ImGui::GetDrawData(), format,
                                context.getWidth(), context.getHeight());
      });
}

/*
 */
void
EditorApplication::onPostPresent()
{
  m_imguiRenderer->presentFloatingWindows();
}

/*
 */
bool
EditorApplication::onCloseRequested()
{
  return EditorScene::requestQuit();
}

/*
 */
void
EditorApplication::updateWindowTitle()
{
  const uint32 revision = EditorScene::getRevision();
  if (revision == m_titleRevision) {
    return;
  }
  m_titleRevision = revision;
#if USING(CH_DISPLAY_SDL3)
  const String title =
      StringUtils::format("{0} - {1}", m_windowTitle, EditorScene::getDisplayName());
  SDL_SetWindowTitle(getDisplaySurface()->getPlatformHandler(), title.c_str());
#endif // USING(CH_DISPLAY_SDL3)
}

/*
 */
void
EditorApplication::initializeEditorComponents() {
  CH_LOG_INFO(EditorApp, "Initializing editor components.");
  SPtr<DisplaySurface> display = getDisplaySurface();
  CH_ASSERT(display && "Display surface must not be null.");
  // Also projects opened with -project, so the browser lists every project used.
  if (!EnginePaths::getProjectFile().empty()) {
    ProjectBrowserUI::addRecentProject(EnginePaths::getProjectFile());
  }

#if USING(CH_CODECS)
  AssetCodecManager::startUp();
  AssetCodecManager::instance().initialize();
  loadCodecs();
#endif // USING(CH_CODECS)

  AssetManager::startUp();
  AssetManager& assetManager = AssetManager::instance();
  assetManager.initialize();
  assetManager.lazyLoadAssetsFromDirectory(EnginePaths::getGameAssetDirectory());
  ScriptManager::startUp();
  ScriptDefinitionsUI::requestUpdate();

  m_sceneRenderer = chMakeUnique<SceneRenderer>();
  m_viewportGrid = chMakeUnique<ViewportGrid>();

  const ISwapChain& swapChain = *getSwapChain();
  m_viewportWidth = swapChain.getWidth();
  m_viewportHeight = swapChain.getHeight();
  m_editorCamera = chMakeUnique<EditorCamera>(static_cast<float>(swapChain.getWidth()),
                                              static_cast<float>(swapChain.getHeight()));
  // The game reads input only while it plays; updateGameInput turns it on.
  InputManager::instance().setGameInputEnabled(false);

  // After the camera, which takes the view saved with the scene.
  SceneManager::startUp();
  EditorScene::initialize(*m_editorCamera);

#if USING(CH_DISPLAY_SDL3)
  m_windowTitle = SDL_GetWindowTitle(display->getPlatformHandler());
#endif // USING(CH_DISPLAY_SDL3)

  initImGui(display);

  m_contentAssetUI = chMakeUnique<ContentAssetUI>();
  m_mainMenuBar = chMakeUnique<MainMenuBarUI>();
  m_outputLogUI = chMakeUnique<OutputLogUI>();
  m_sceneGraphUI = chMakeUnique<SceneGraphUI>();
  m_inspectorUI = chMakeUnique<InspectorUI>();

  m_viewportTools = chMakeUnique<ViewportTools>(*m_editorCamera);
  m_sceneGraphUI->setEditorCamera(m_editorCamera.get());
  m_sceneGraphUI->setContentBrowser(m_contentAssetUI.get());
  m_outputLogUI->updateAvailableCategories();

  CH_LOG_INFO(EditorApp, "Editor components initialized successfully.");
}

/*
 */
void
EditorApplication::bindEvents()
{
  CH_LOG_INFO(EditorApp, "Binding editor events.");
  CH_ASSERT(InputManager::isStarted());

  InputManager& input = InputManager::instance();
  m_onKeyPressedEvent = input.OnKeyPressed.connect([this](const KeyEvent& event) {
    if (!event.hasControl()) {
      handlePlayShortcut(event);
      return;
    }

    const bool bShift = event.hasShift();
    if (event.key == InputKey::S) {
      if (bShift) {
        EditorScene::requestSaveAs();
      }
      else {
        EditorScene::requestSave();
      }
    }
    else if (event.key == InputKey::N) {
      EditorScene::requestNew();
    }
    // A text field being typed in has its own undo.
    else if (!ImGui::GetIO().WantTextInput) {
      if (event.key == InputKey::Z && !bShift) {
        EditorScene::requestUndo();
      }
      else if (event.key == InputKey::Y || (event.key == InputKey::Z && bShift)) {
        EditorScene::requestRedo();
      }
      else if (event.key == InputKey::D) {
        EditorScene::requestDuplicate();
      }
      else if (event.key == InputKey::C) {
        EditorScene::copySelection();
      }
      else if (event.key == InputKey::V) {
        EditorScene::requestPaste();
      }
      else if (event.key == InputKey::A && EditorScene::getScene()) {
        EditorSelection::selectAll(*EditorScene::getScene(),
                                   EditorScene::getPrefabRoot().get());
      }
    }

    if (event.key == InputKey::F5) {
      CH_LOG_DEBUG(EditorApp, "Ctrl+F5 pressed, reloading the current document.");
      m_outputLogUI->updateAvailableCategories();
    }
  });

  m_onKeyReleasedEvent = input.OnKeyReleased.connect([](const KeyEvent& event) {
    if (event.key != InputKey::F11) {
      return;
    }
    UIHelpers::bRenderImGui = !UIHelpers::bRenderImGui;
    if (UIHelpers::bRenderImGui) {
      // ImGui got no events while hidden, so keys and buttons released meanwhile would
      // still count as held.
      ImGuiIO& io = ImGui::GetIO();
      io.ClearInputKeys();
      io.ClearInputMouse();
    }
  });
}

/*
 */
void
EditorApplication::initImGui(const SPtr<DisplaySurface>& display)
{
  CH_LOG_INFO(EditorApp, "Initializing ImGui for the editor.");

  // ImGui keeps the pointer, not a copy, so the string must outlive the context.
  static const String iniFilePath =
      FileSystem::absolutePath(EnginePaths::getConfigDirectory().join(Path("imgui.ini")))
          .toString();
  SPtr<DisplayEventHandle> eventHandler = getEventHandler();
  CH_ASSERT(eventHandler && "Display event handler must not be null.");
  m_updateInjection = UIHelpers::initializeImGui(display, eventHandler, iniFilePath.c_str());
  CH_ASSERT(m_updateInjection.isValid() && "Update injection event must be valid.");
  m_imguiRenderer = chMakeUnique<ImGuiRenderer>();
}

/*
 */
void
EditorApplication::renderUI()
{
  UIHelpers::newFrame();
  m_viewportTools->beginFrame();

  // The menu bar first, so the dockspace takes the space under it.
  m_mainMenuBar->renderMainMenuBar();
  renderDockSpace();
  renderViewportPanel();
  m_contentAssetUI->renderContentAssetUI();
  m_outputLogUI->renderOutputLogUI();
  m_sceneGraphUI->renderSceneGraphUI();
  m_inspectorUI->renderInspectorUI();
  AssetInspectorUI::render();
  AnimationPreviewUI::render();
  ProfilerUI::render(m_sceneRenderer->getLastFrameStats());
  ImportDialog::render();
  PackageDialog::render();
  ScriptDefinitionsUI::render();
  ProjectBrowserUI::renderPopup();
  EditorScene::render(m_contentAssetUI->getCurrentFolderPath());

  ImGui::Render();
  // Opens, closes and resizes the windows outside the main one before they are drawn.
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
  }
}

/*
 */
void
EditorApplication::onCameraFlightChanged(bool bFlying)
{
  // Hidden and held in place, so turning never stops at the edge of the screen.
  SDL_SetWindowRelativeMouseMode(getDisplaySurface()->getPlatformHandler(), bFlying);
  if (bFlying) {
    // Otherwise the focused panel also gets the flight keys.
    ImGui::SetWindowFocus(nullptr);
  }
}

/*
 */
void
EditorApplication::renderDockSpace()
{
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  const ImGuiID dockSpaceId = ImGui::GetID("EditorDockSpace");
  // imgui.ini is read by the first NewFrame, so a saved layout is already known here.
  if (UIHelpers::bResetLayout || !ImGui::DockBuilderGetNode(dockSpaceId)) {
    UIHelpers::bResetLayout = false;
    ImGui::DockBuilderRemoveNode(dockSpaceId);
    ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockSpaceId, viewport->WorkSize);

    ImGuiID center = dockSpaceId;
    const ImGuiID left =
        ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, kLayoutLeftFraction, nullptr, &center);
    const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right,
                                                      kLayoutRightFraction, nullptr, &center);
    const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down,
                                                       kLayoutBottomFraction, nullptr, &center);
    // Names hash only from ###, so the ids find the panels whatever their titles say.
    ImGui::DockBuilderDockWindow("###Viewport", center);
    ImGui::DockBuilderDockWindow("###SceneGraph", left);
    ImGui::DockBuilderDockWindow("###Inspector", right);
    ImGui::DockBuilderDockWindow("###AssetInspector", right);
    ImGui::DockBuilderDockWindow("###AnimationPreview", right);
    ImGui::DockBuilderDockWindow("###ContentBrowser", bottom);
    ImGui::DockBuilderDockWindow("###OutputLog", bottom);
    ImGui::DockBuilderDockWindow("###Profiler", bottom);
    ImGui::DockBuilderFinish(dockSpaceId);
  }
  ImGui::DockSpaceOverViewport(dockSpaceId, viewport);
}

/*
 */
void
EditorApplication::renderViewportPanel()
{
  // ImGuizmo only starts a drag while no ImGui item is hovered, so the button that takes the
  // scene's clicks steps aside while the mouse is over the gizmo; the panel must not move
  // with that drag either.
  const bool bGizmoHasMouse = m_viewportTools->isGizmoHovered();
  ImGuiWindowFlags windowFlags =
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
      ImGuiWindowFlags_NoCollapse;
  if (bGizmoHasMouse) {
    windowFlags |= ImGuiWindowFlags_NoMove;
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  // The wheel moves the camera, not the panel; there is no close button, the scene is always
  // there.
  const bool bOpen =
      ImGui::Begin(ICON_FA_DESKTOP "  Viewport###Viewport", nullptr, windowFlags);
  ImGui::PopStyleVar();
  if (!bOpen) {
    ImGui::End();
    return;
  }
  if (m_bFocusViewport) {
    ImGui::SetWindowFocus();
    m_bFocusViewport = false;
  }
  // The toolbar and the stats are child windows of the panel, so they count as the panel.
  m_bViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

  const ImVec2 min = ImGui::GetCursorScreenPos();
  const ImVec2 available = ImGui::GetContentRegionAvail();
  const ImVec2 size(Math::max(available.x, 1.0f), Math::max(available.y, 1.0f));
  if (bGizmoHasMouse) {
    ImGui::Dummy(size);
    m_bViewportHovered =
        ImGui::IsWindowHovered() &&
        ImGui::IsMouseHoveringRect(min, ImVec2(min.x + size.x, min.y + size.y));
  }
  else {
    // Takes the clicks, so a floating panel does not move when the scene is dragged.
    ImGui::InvisibleButton("##Scene", size,
                           ImGuiButtonFlags_MouseButtonLeft |
                               ImGuiButtonFlags_MouseButtonRight |
                               ImGuiButtonFlags_MouseButtonMiddle);
    m_bViewportHovered = ImGui::IsItemHovered();
  }
  m_viewportArea = {.x = min.x, .y = min.y, .width = size.x, .height = size.y};

  if (ImGui::BeginDragDropTarget()) {
    GameObject* parent = EditorScene::getDefaultParent();
    Scene& scene = *EditorScene::getScene();
    if (const SPtr<ModelAsset> model = AssetDragDrop::acceptModel()) {
      placeAtMouse(AssetDragDrop::createModelObject(scene, model, parent));
    }
    if (const SPtr<PrefabAsset> prefab = AssetDragDrop::acceptPrefab()) {
      placeAtMouse(AssetDragDrop::createPrefabObject(scene, prefab, parent));
    }
    ImGui::EndDragDropTarget();
  }

  const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
  updateViewportTarget(static_cast<uint32>(size.x * scale.x),
                       static_cast<uint32>(size.y * scale.y));
  const uint64 textureId = ImGuiRenderer::getTextureId(*m_viewportTarget);
  if (textureId != 0) {
    ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(textureId), min,
                                         ImVec2(min.x + size.x, min.y + size.y));
    m_bViewportDrawn = true;
  }

  if (EditorScene::isPlaying() && g_cvarPlayBorderThickness.get() > 0.0f) {
    const ImVec4 color = EditorScene::isPaused() ? kPausedBorderColor : kPlayingBorderColor;
    const float thickness = g_cvarPlayBorderThickness.get();
    const float inset = thickness * 0.5f;
    ImGui::GetWindowDrawList()->AddRect(
        ImVec2(min.x + inset, min.y + inset),
        ImVec2(min.x + size.x - inset, min.y + size.y - inset),
        ImGui::ColorConvertFloat4ToU32(color), 0.0f, ImDrawFlags_None, thickness);
  }

  m_viewportTools->render(*EditorScene::getScene(), m_viewportArea, m_bViewportHovered,
                          !m_bGameView);
  // The scene of this frame is drawn after the UI, so these are the last frame's numbers.
  ProfilerUI::renderOverlay(m_sceneRenderer->getLastFrameStats(), min.x + size.x, min.y);
  ImGui::End();
}

/*
 */
void
EditorApplication::updateViewportTarget(uint32 width, uint32 height)
{
  width = Math::max(width, kMinViewportSize);
  height = Math::max(height, kMinViewportSize);
  if (m_viewportTarget && m_viewportTarget->getWidth() == width &&
      m_viewportTarget->getHeight() == height) {
    return;
  }
  // While a splitter or the window is dragged the size changes every frame; the old image is
  // stretched until the button is released, so it is made once instead of per frame.
  if (m_viewportTarget && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    return;
  }

  // The old texture is freed by the deferred deletion once the GPU is done with it, and
  // the view points at it, so the view goes first. The swap chain's format keeps the
  // pipelines the same as when F11 draws into the window.
  m_viewportTargetView.reset();
  m_viewportTarget = IGraphicsAPI::instance().createTexture(
      {.format = getSwapChain()->getFormat(),
       .width = width,
       .height = height,
       .usage = TextureUsageFlags(TextureUsage::ColorAttachment) | TextureUsage::Sampled});
  m_viewportTargetView = m_viewportTarget->createView();
}

/*
 */
void
EditorApplication::placeAtMouse(const SPtr<GameObject>& gameObject)
{
  if (!gameObject) {
    return;
  }

  const ImVec2 mouse = ImGui::GetMousePos();
  const Vector2 screenPoint((mouse.x - m_viewportArea.x) / m_viewportArea.width,
                            (mouse.y - m_viewportArea.y) / m_viewportArea.height);
  const Camera& camera = getViewCamera();
  const Ray ray = camera.screenToWorldRay(screenPoint);

  // Models land on the ground (Z = 0) under the mouse. When the ray misses the ground, or
  // meets it near the horizon, they stay under the mouse at the look at distance instead.
  const float lookAtDistance = (camera.getLookAt() - camera.getPosition()).magnitude();
  float distance = 0.0f;
  if (!RayCast::plane(ray, Plane(Vector3::UP, 0.0f), distance) ||
      distance > lookAtDistance * kMaxDropDistanceScale) {
    distance = lookAtDistance;
  }
  gameObject->getTransform().setLocalPosition(ray.origin + ray.direction * distance);
}

/*
 * As in Unreal, the game gets the keys only while it runs and the viewport has the focus,
 * so typing in a panel or using the editor while ejected or paused never reaches it.
 */
void
EditorApplication::updateGameInput()
{
  InputManager& input = InputManager::instance();
  const bool bPlaying = EditorScene::isPlaying();
  if (bPlaying != m_bInputPlaying) {
    // Whatever the last run left (contexts, cursor, rumble) must not reach the next.
    input.resetGameState();
    // Like the scene, keys a script changed during Play and did not save are thrown away.
    if (!bPlaying) {
      input.loadBindings();
    }
    m_bInputPlaying = bPlaying;
    m_bFocusViewport = bPlaying;
  }

  const bool bViewportHasKeys =
      !UIHelpers::bRenderImGui || (m_bViewportFocused && !ImGui::GetIO().WantTextInput);
  input.setGameInputEnabled(bPlaying && !EditorScene::isPaused() &&
                            !EditorScene::isEjected() && bViewportHasKeys);
}

/*
 */
void
EditorApplication::updateViewCamera(const Scene& scene)
{
  // As in Unreal, Stop puts the editor camera back where it was before Play, whatever it
  // did while ejected.
  const bool bPlaying = EditorScene::isPlaying();
  const Camera& editorCamera = m_editorCamera->getCamera();
  if (bPlaying && !m_bWasPlaying) {
    m_positionBeforePlay = editorCamera.getPosition();
    m_lookAtBeforePlay = editorCamera.getLookAt();
  }
  else if (!bPlaying && m_bWasPlaying) {
    m_editorCamera->setView(m_positionBeforePlay, m_lookAtBeforePlay);
  }
  m_bWasPlaying = bPlaying;

  const CameraComponent* gameCamera =
      bPlaying && !EditorScene::isEjected() ? scene.getActiveCamera() : nullptr;
  const bool bGameView = gameCamera != nullptr;
  if (bGameView) {
    gameCamera->fillCamera(m_gameCamera, static_cast<float>(m_viewportWidth),
                           static_cast<float>(m_viewportHeight));
  }
  else if (m_bGameView && bPlaying) {
    // Ejected: the editor camera starts where the game's was.
    m_editorCamera->setView(m_gameCamera.getPosition(), m_gameCamera.getLookAt());
  }
  m_bGameView = bGameView;
}

/*
 */
const Camera&
EditorApplication::getViewCamera() const
{
  return m_bGameView ? m_gameCamera : m_editorCamera->getCamera();
}

/*
 * Plain keys without Ctrl, from the window's events, so they also work while F11 hides the
 * editor. Escape is left to popups and text fields that have it.
 */
void
EditorApplication::handlePlayShortcut(const KeyEvent& event)
{
  const bool bPlaying = EditorScene::isPlaying();
  if (event.hasAlt() && event.key == InputKey::P) {
    if (bPlaying) {
      EditorScene::requestStop();
    }
    else {
      EditorScene::requestPlay();
    }
    return;
  }
  if (!bPlaying) {
    return;
  }

  switch (event.key) {
  case InputKey::Escape:
    if (!ImGui::GetIO().WantTextInput &&
        !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) {
      EditorScene::requestStop();
    }
    break;
  case InputKey::Pause:
    EditorScene::setPaused(!EditorScene::isPaused());
    break;
  case InputKey::F10:
    EditorScene::requestStep();
    break;
  case InputKey::F8:
    EditorScene::setEjected(!EditorScene::isEjected());
    break;
  default:
    break;
  }
}

/*
*/
void
EditorApplication::loadCodecs() {
  CH_LOG_INFO(EditorApp, "Loading asset codecs.");
#if USING(CH_CODECS)
  //AssetCodecManager& codecManager = AssetCodecManager::instance();
  const Path& codecsPath = EnginePaths::getCodecDirectory();
  Vector<Path> files;
  Vector<Path> directories;
  FileSystem::getChildren(codecsPath, files, directories);
  for (const Path& file : files) {
    if (file.getExtension() == DynamicLibrary::EXTENSION) {
      WeakPtr<DynamicLibrary> library =
          DynamicLibraryManager::instance().loadDynLibrary(file.getFileName(), codecsPath);
      if (library.expired()) {
        CH_LOG_ERROR(EditorApp, "Failed to load codec library: {0}", file.toString());
        continue;
      }
      typedef void (*LoadPluginFunc)();
      LoadPluginFunc loadPlugin = library.lock()->getSymbol<LoadPluginFunc>("loadPlugin");
      if (!loadPlugin) {
        CH_LOG_ERROR(EditorApp, "Failed to find loadPlugin function in {0}", file.toString());
        continue;
      }
      loadPlugin(); // This should register the codec
      CH_LOG_INFO(EditorApp, "Loaded codec library: {0}", file.toString());
    }
  }
  #endif // USING(CH_CODECS)
}

} // namespace chEngineSDK
