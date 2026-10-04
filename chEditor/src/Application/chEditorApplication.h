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

#include "chNastyRenderer.h"

#include "chUUID.h"

struct ImVec4;

namespace chEngineSDK {
class ContentAssetUI;
class MainMenuBarUI;
class OutputLogUI;
class SceneGraphUI;
class InspectorUI;
class GameObjectAssetUI;

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
  NODISCARD virtual LinearColor
  getBackgroundColor() const override;

  virtual void
  onPostInitialize() override;

  virtual void
  destroyModules() override;

  virtual RendererOutput
  onRender(float deltaTime);

  virtual void
  onPresent(const RendererOutput& rendererOutput,
            const SPtr<ICommandBuffer>& commandBuffer,
            uint32 swapChainWidth, uint32 swapChainHeight);

 private:
  void
  initializeEditorComponents();

  void
  bindEvents();

  void
  initImGui(const SPtr<DisplaySurface>& display);

  void
  renderFullScreenRenderer(const RendererOutput& rendererOutput);

  void
  resizeViewport(uint32 viewportWidth, uint32 viewportHeight);

  void
  loadCodecs();

 private:
  SPtr<NastyRenderer> m_nastyRenderer; ///< The renderer used by the editor
  SPtr<Scene> m_activeScene;

  SPtr<ISampler> m_defaultSampler;
  // ImGui texture id (ImTextureID) of each renderer target shown in the viewport.
  Map<SPtr<ITextureView>, uint64> m_imguiTextures;

  // Pixel size of the viewport panel in the last UI frame; the renderer follows it.
  uint32 m_viewportWidth = 0;
  uint32 m_viewportHeight = 0;

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
