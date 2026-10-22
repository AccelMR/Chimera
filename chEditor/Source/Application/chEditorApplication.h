/************************************************************************/
/**
 * @file chEditorApplication.h
 * @author AccelMR
 * @date 2025/07/07
 * @details
 *  Chimera Editor application class.
 */
/************************************************************************/
#pragma once

#include "chWindowedApplication.h"

#include "chCamera.h"
#include "chDebugDraw.h"
#include "chSceneRenderer.h"
#include "chViewportTools.h"

#include "chUUID.h"

struct ImVec4;

namespace chEngineSDK {
class ContentAssetUI;
class MainMenuBarUI;
class OutputLogUI;
class SceneGraphUI;
class InspectorUI;
class ImGuiRenderer;
class EditorCamera;
class ViewportTools;
class ViewportGrid;
class ModelAsset;

class EditorApplication : public WindowedApplication
{
 public:
  /**
   * @brief Default constructor.
   */
  EditorApplication();

  /**
   * @brief Default destructor.
   */
  virtual ~EditorApplication();

 protected:
  virtual void
  onPostInitialize() override;

  virtual void
  destroyModules() override;

  virtual void
  fixedUpdate(const float deltaTime) override;

  virtual void
  update(const float deltaTime) override;

  virtual void
  onRender(ICommandList& commandList, const ISwapChain& swapChain) override;

  virtual void
  onPostPresent() override;

  NODISCARD virtual bool
  onCloseRequested() override;

  NODISCARD virtual StringView
  getDefaultApplicationId() const override { return "chimera-editor"; }

 private:
  void
  initializeEditorComponents();

  void
  bindEvents();

  void
  initImGui(const SPtr<DisplaySurface>& display);

  void
  renderUI();

  /**
   * Fills the main window with a dockspace, docking the panels in their default places the
   * first time (nothing in imgui.ini) and after View > Reset Layout.
   */
  void
  renderDockSpace();

  /**
   * The panel that shows the scene, with the viewport tools over it; also takes the models
   * and prefabs dropped on it.
   */
  void
  renderViewportPanel();

  /**
   * Makes the scene image again when the panel changed size, before the UI shows it.
   */
  void
  updateViewportTarget(uint32 width, uint32 height);

  /**
   * Draws this frame's ImGui draw data into the back buffer.
   */
  void
  addImGuiPass(RenderGraph& graph, RGTextureHandle backBuffer, LoadOp loadOp, Format format);

  void
  onCameraFlightChanged(bool bFlying);

  /**
   * Moves a just dropped object under the mouse, on the ground when it can.
   */
  void
  placeAtMouse(const SPtr<GameObject>& gameObject);

  void
  updateGameInput();

  /**
   * Picks the camera the viewport shows this frame: the scene's while playing (unless
   * ejected or the scene has none), else the editor's.
   */
  void
  updateViewCamera(const Scene& scene);

  NODISCARD const Camera&
  getViewCamera() const;

  void
  handlePlayShortcut(const KeyEvent& event);

  /**
   * Shows the scene name, and whether it has unsaved changes, after the window title.
   */
  void
  updateWindowTitle();

  void
  loadCodecs();

 private:
  UniquePtr<SceneRenderer> m_sceneRenderer;
  UniquePtr<ViewportGrid> m_viewportGrid;
  // Size in pixels the camera was last set up for: the viewport panel's, or the swap
  // chain's while F11 hides the UI.
  uint32 m_viewportWidth = 0;
  uint32 m_viewportHeight = 0;
  // What the viewport panel shows; the scene is drawn into it while the UI is shown.
  SPtr<ITexture> m_viewportTarget;
  SPtr<ITextureView> m_viewportTargetView;
  ViewportArea m_viewportArea;
  // The panel showed the scene image this frame; false while it is behind another tab.
  bool m_bViewportDrawn = false;
  bool m_bViewportHovered = false;
  // The panel or one of its child windows has the keyboard focus.
  bool m_bViewportFocused = false;
  // Play just started, so the panel takes the focus and the game its keys.
  bool m_bFocusViewport = false;
  // Play state the input system was last set for.
  bool m_bInputPlaying = false;
  // Copied in the update, so drawing does not read UI state.
  bool m_bFrameDrawsViewport = false;
  UniquePtr<EditorCamera> m_editorCamera;
  // While playing and not ejected, the view of the scene's active camera.
  Camera m_gameCamera;
  // The viewport looked through m_gameCamera in the last update.
  bool m_bGameView = false;
  bool m_bWasPlaying = false;
  // The editor camera's view when Play started, given back at Stop.
  Vector3 m_positionBeforePlay = Vector3::ZERO;
  Vector3 m_lookAtBeforePlay = Vector3::ZERO;
  // Time of one fixed step, which also runs a paused game one frame.
  float m_fixedStep = 1.0f / 60.0f;
  UniquePtr<ViewportTools> m_viewportTools;
  // Lines of the editor (selection bounds), filled again every frame; kept so it does not
  // allocate.
  DebugDraw m_editorLines;
  // Whether the frame the update decided draws the editor UI (F11 hides it), so drawing
  // does not read the toggle.
  bool m_bFrameHasUI = true;
  // With the UI hidden, the frame still draws the frames per second counter.
  bool m_bFrameHasFps = false;
  // The title the window was made with, before the scene name.
  String m_windowTitle;
  uint32 m_titleRevision = ~0u;

  UniquePtr<ImGuiRenderer> m_imguiRenderer;

  UniquePtr<ContentAssetUI> m_contentAssetUI; ///< Content Asset UI instance
  UniquePtr<MainMenuBarUI> m_mainMenuBar; ///< Main menu bar instance
  UniquePtr<OutputLogUI> m_outputLogUI; ///< Output log UI instance
  UniquePtr<SceneGraphUI> m_sceneGraphUI; ///< Scene graph UI instance
  UniquePtr<InspectorUI> m_inspectorUI; ///< Inspector UI instance

  HEvent m_updateInjection; ///< Event for updating ImGui with SDL events

  HEvent m_onKeyPressedEvent;
  HEvent m_onKeyReleasedEvent;
};
} // namespace chEngineSDK
