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

#include "chSceneRenderer.h"

#include "chUUID.h"

struct ImVec4;

namespace chEngineSDK {
class ContentAssetUI;
class MainMenuBarUI;
class OutputLogUI;
class SceneGraphUI;
class InspectorUI;
class GameObjectAssetUI;
class ImGuiRenderer;
class EditorCamera;
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
  update(const float deltaTime) override;

  virtual void
  onRender(ICommandList& commandList, const ISwapChain& swapChain, float deltaTime) override;

  virtual void
  onPostPresent() override;

 private:
  void
  initializeEditorComponents();

  void
  bindEvents();

  void
  initImGui(const SPtr<DisplaySurface>& display);

  void
  renderUI();

  void
  renderViewportDropTarget();

  void
  addModelAtMouse(const ModelAsset& modelAsset);

  void
  loadCodecs();

 private:
  UniquePtr<SceneRenderer> m_sceneRenderer;
  // Size of the swap chain the camera was last set up for.
  uint32 m_viewportWidth = 0;
  uint32 m_viewportHeight = 0;
  UniquePtr<EditorCamera> m_editorCamera;
  SPtr<Scene> m_activeScene;

  UniquePtr<ImGuiRenderer> m_imguiRenderer;

  UniquePtr<ContentAssetUI> m_contentAssetUI; ///< Content Asset UI instance
  UniquePtr<MainMenuBarUI> m_mainMenuBar; ///< Main menu bar instance
  UniquePtr<OutputLogUI> m_outputLogUI; ///< Output log UI instance
  UniquePtr<SceneGraphUI> m_sceneGraphUI; ///< Scene graph UI instance
  UniquePtr<InspectorUI> m_inspectorUI; ///< Inspector UI instance
  UniquePtr<GameObjectAssetUI> m_gameObjectAssetUI; ///< GameObject Asset UI instance

  HEvent m_updateInjection; ///< Event for updating ImGui with SDL events

  HEvent m_onKeyDownEvent; ///< Event for handling key down events
  HEvent m_onKeyUpEvent; ///< Event for handling key up events
};
} // namespace chEngineSDK
