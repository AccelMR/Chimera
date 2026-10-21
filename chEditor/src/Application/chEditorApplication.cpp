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
#include "chEditorCamera.h"
#include "chEditorScene.h"
#include "chEditorTheme.h"
#include "chConsoleVariable.h"
#include "chEventDispatcherManager.h"
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
#include "chScene.h"
#include "chSceneManager.h"
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
  // Its worker runs codec code and uses the codec manager.
  ImportDialog::shutDown();
  if (IGraphicsAPI::isStarted()) {
    IGraphicsAPI::instance().waitIdle();
  }

  m_onKeyDownEvent.disconnect();
  m_onKeyUpEvent.disconnect();
  m_updateInjection.disconnect();

  // The renderer gives its textures back to the ImGui context, and the SDL backend uses the
  // window, which the parent destroys.
  m_imguiRenderer.reset();
  if (ImGui::GetCurrentContext()) {
#if USING(CH_DISPLAY_SDL3)
    if (ImGui::GetIO().BackendPlatformUserData) {
      ImGui_ImplSDL3_Shutdown();
    }
#endif // USING(CH_DISPLAY_SDL3)
    ImGui::DestroyContext();
  }

  m_inspectorUI.reset();
  AssetInspectorUI::shutDown();
  AnimationPreviewUI::shutDown();
  AssetThumbnails::shutDown();
  m_sceneGraphUI.reset();
  m_outputLogUI.reset();
  m_mainMenuBar.reset();
  m_contentAssetUI.reset();
  m_viewportTools.reset();

  if (m_editorCamera) {
    m_editorCamera->unbindInputEvents();
    m_editorCamera.reset();
  }
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
  SceneManager::instance().fixedUpdate(deltaTime);
}

/*
 */
void
EditorApplication::update(const float deltaTime)
{
  // A minimized window draws nothing, so no UI frame is built for it either.
  m_bViewportDrawn = false;
  m_bViewportHovered = false;
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
  // A drag or a text field still going on keeps its changes in one undo step.
  EditorScene::applyPending(UIHelpers::bRenderImGui &&
                            (ImGui::IsAnyItemActive() || m_viewportTools->isUsingMouse()));
  if (EditorScene::shouldQuit()) {
    quit();
  }
  updateWindowTitle();
  Scene& scene = *EditorScene::getScene();

  // A flight only starts with the mouse over the scene image. The keyboard is not checked: a
  // panel keeps the keyboard focus after any click, and the camera only reads keys while
  // flying.
  m_editorCamera->setFocused(!UIHelpers::bRenderImGui ||
                             (m_bViewportHovered && !m_viewportTools->isUsingMouse()));
  const bool bWasFlying = m_editorCamera->isFlying();
  m_editorCamera->update(deltaTime);
  if (m_editorCamera->isFlying() != bWasFlying) {
    onCameraFlightChanged(m_editorCamera->isFlying());
  }

  {
    CH_PROFILE_SCOPE("SceneUpdate");
    SceneManager::instance().update(deltaTime);
  }

  // After the UI and the scene update, which may have moved or added objects.
  {
    CH_PROFILE_SCOPE("UpdateTransforms");
    scene.updateTransforms();
  }

  // Like the grid, the editor's lines hide with the rest of the UI.
  m_bFrameHasUI = UIHelpers::bRenderImGui;
  m_bFrameDrawsViewport = bPanel;
  m_editorLines.clear();
  if (m_bFrameHasUI) {
    ViewportTools::addSelectionBounds(scene, m_editorLines);
    ViewportTools::addSelectionSkeletons(m_editorLines);
  }

  m_sceneRenderer->extract(scene, m_editorCamera->getCamera(), UIHelpers::rendererColor,
                           &m_editorLines);
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
    if (ViewportGrid::isVisible()) {
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

#if USING(CH_CODECS)
  AssetCodecManager::startUp();
  AssetCodecManager::instance().initialize();
  loadCodecs();
#endif // USING(CH_CODECS)

  AssetManager::startUp();
  AssetManager& assetManager = AssetManager::instance();
  assetManager.initialize();
  assetManager.lazyLoadAssetsFromDirectory(EnginePaths::getGameAssetDirectory());

  m_sceneRenderer = chMakeUnique<SceneRenderer>();
  m_viewportGrid = chMakeUnique<ViewportGrid>();

  const ISwapChain& swapChain = *getSwapChain();
  m_viewportWidth = swapChain.getWidth();
  m_viewportHeight = swapChain.getHeight();
  m_editorCamera = chMakeUnique<EditorCamera>(static_cast<float>(swapChain.getWidth()),
                                              static_cast<float>(swapChain.getHeight()));
  m_editorCamera->bindInputEvents();

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
EditorApplication::bindEvents() {
  CH_LOG_INFO(EditorApp, "Binding editor events.");
  CH_ASSERT(EventDispatcherManager::isStarted());

  EventDispatcherManager& eventDispatcher = EventDispatcherManager::instance();

  m_onKeyDownEvent = eventDispatcher.OnKeyDown.connect([this](const KeyBoardData& data) {
    if (data.hasModifier(KeyBoardModifier::LCTRL) ||
        data.hasModifier(KeyBoardModifier::RCTRL)) {
      const bool bShift = data.hasModifier(KeyBoardModifier::LSHIFT) ||
                          data.hasModifier(KeyBoardModifier::RSHIFT);
      if (data.key == chKeyBoard::Key::S) {
        if (bShift) {
          EditorScene::requestSaveAs();
        }
        else {
          EditorScene::requestSave();
        }
      }
      else if (data.key == chKeyBoard::Key::N) {
        EditorScene::requestNew();
      }
      // A text field being typed in has its own undo.
      else if (!ImGui::GetIO().WantTextInput) {
        if (data.key == chKeyBoard::Key::Z && !bShift) {
          EditorScene::requestUndo();
        }
        else if (data.key == chKeyBoard::Key::Y ||
                 (data.key == chKeyBoard::Key::Z && bShift)) {
          EditorScene::requestRedo();
        }
        else if (data.key == chKeyBoard::Key::D) {
          EditorScene::requestDuplicate();
        }
        else if (data.key == chKeyBoard::Key::C) {
          EditorScene::copySelection();
        }
        else if (data.key == chKeyBoard::Key::V) {
          EditorScene::requestPaste();
        }
        else if (data.key == chKeyBoard::Key::A && EditorScene::getScene()) {
          EditorSelection::selectAll(*EditorScene::getScene(),
                                     EditorScene::getPrefabRoot().get());
        }
      }

      if (data.key == chKeyBoard::Key::F5) {
        CH_LOG_DEBUG(EditorApp, "Ctrl+F5 pressed, reloading the current document.");
        m_outputLogUI->updateAvailableCategories();
      }
    }
  });

  m_onKeyUpEvent = eventDispatcher.OnKeyUp.connect([](const KeyBoardData& keyData) {
    switch (keyData.key) {
    case chKeyBoard::Key::F11: {
      UIHelpers::bRenderImGui = !UIHelpers::bRenderImGui;
      if (UIHelpers::bRenderImGui) {
        // ImGui got no events while hidden, so keys and buttons released meanwhile would
        // still count as held.
        ImGuiIO& io = ImGui::GetIO();
        io.ClearInputKeys();
        io.ClearInputMouse();
      }
    } break;

    default:
      break;
    }
  });

  // eventDispatcher.OnKeyPressed.connect([this](const KeyBoardData&) {
  //   // Handle key pressed events specific to the editor
  // });

  // eventDispatcher.OnMouseButtonDown.connect([this](const MouseButtonData&) {
  //   // Handle mouse button down events specific to the editor
  // });

  // eventDispatcher.OnMouseButtonUp.connect([this](const MouseButtonData&) {
  //   // Handle mouse button up events specific to the editor
  // });

  // eventDispatcher.OnMouseMove.connect([this](const MouseMoveData&) {
  //   // Handle mouse move events specific to the editor
  // });
  // eventDispatcher.OnMouseWheel.connect([this](const MouseWheelData&) {
  //   // Handle mouse wheel events specific to the editor
  // });
}

/*
 */
void
EditorApplication::initImGui(const SPtr<DisplaySurface>& display)
{
  CH_LOG_INFO(EditorApp, "Initializing ImGui for the editor.");

  ImGui::CreateContext();

  // ImGui keeps the pointer, not a copy, so the string must outlive the context.
  static const String iniFilePath =
      FileSystem::absolutePath(EnginePaths::getConfigDirectory().join(Path("imgui.ini")))
          .toString();
  ImGui::GetIO().IniFilename = iniFilePath.c_str();

  UIHelpers::initConfigFlags();
  EditorTheme::initialize();

#if USING(CH_DISPLAY_SDL3)
  // The windows ImGui opens need the same window flags the graphics API asked for, and the
  // backend only adds this one when it is started for it.
  const uint64 windowFlags = IGraphicsAPI::instance().getPlatformWindowFlags();
  if ((windowFlags & SDL_WINDOW_VULKAN) != 0) {
    ImGui_ImplSDL3_InitForVulkan(display->getPlatformHandler());
  }
  else {
    ImGui_ImplSDL3_InitForOther(display->getPlatformHandler());
  }
#endif // USING(CH_DISPLAY_SDL3)
  m_imguiRenderer = chMakeUnique<ImGuiRenderer>();

  SPtr<DisplayEventHandle> eventHandler = getEventHandler();
  CH_ASSERT(eventHandler && "Display event handler must not be null.");
  m_updateInjection = UIHelpers::bindEventWindow(eventHandler);
  CH_ASSERT(m_updateInjection.isValid() && "Update injection event must be valid.");
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
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  // The wheel moves the camera, not the panel; there is no close button, the scene is always
  // there.
  const bool bOpen = ImGui::Begin(ICON_FA_DESKTOP "  Viewport###Viewport", nullptr,
                                  ImGuiWindowFlags_NoScrollbar |
                                      ImGuiWindowFlags_NoScrollWithMouse |
                                      ImGuiWindowFlags_NoCollapse);
  ImGui::PopStyleVar();
  if (!bOpen) {
    ImGui::End();
    return;
  }

  const ImVec2 min = ImGui::GetCursorScreenPos();
  const ImVec2 available = ImGui::GetContentRegionAvail();
  const ImVec2 size(Math::max(available.x, 1.0f), Math::max(available.y, 1.0f));
  // Takes the clicks, so a floating panel does not move when the scene is dragged.
  ImGui::InvisibleButton("##Scene", size,
                         ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
                             ImGuiButtonFlags_MouseButtonMiddle);
  m_bViewportHovered = ImGui::IsItemHovered();
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

  m_viewportTools->render(*EditorScene::getScene(), m_viewportArea, m_bViewportHovered);
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
  const Camera& camera = m_editorCamera->getCamera();
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
