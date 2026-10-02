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

class MultiStageRenderer;

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
  loadCodecs();

  void
  setupSceneData();

 private:
  SPtr<NastyRenderer> m_nastyRenderer; ///< The renderer used by the editor
  SPtr<Scene> m_activeScene;

  SPtr<MultiStageRenderer> m_multiStageRenderer;
  UUID m_gbufferStageId;

  SPtr<ISampler> m_defaultSampler;
  Map<SPtr<ITextureView>, SPtr<IDescriptorSet>> m_textureDescriptorSets;

  UniquePtr<ContentAssetUI> m_contentAssetUI; ///< Content Asset UI instance
  UniquePtr<MainMenuBarUI> m_mainMenuBar; ///< Main menu bar instance
  UniquePtr<OutputLogUI> m_outputLogUI; ///< Output log UI instance
  UniquePtr<SceneGraphUI> m_sceneGraphUI; ///< Scene graph UI instance
  UniquePtr<InspectorUI> m_inspectorUI; ///< Inspector UI instance
  UniquePtr<GameObjectAssetUI> m_gameObjectAssetUI; ///< GameObject Asset UI instance

  uint32 width = 0; ///< Width of the editor window
  uint32 height = 0; ///< Height of the editor window
  HEvent m_updateInjection; ///< Event for updating ImGui with SDL events

  HEvent m_onKeyDownEvent; ///< Event for handling key down events
  HEvent m_onKeyUpEvent; ///< Event for handling key up events
};
} // namespace chEngineSDK
