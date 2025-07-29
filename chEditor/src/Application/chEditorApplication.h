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

#if USING(CH_PLATFORM_WIN32)
# if USING(CH_COMPILER_MSVC)
#   if defined( CH_STATIC_LIB )
#     define CH_EDITOR_EXPORT
#   else
#     if defined( CH_EDITOR_EXPORTS )
#       define CH_EDITOR_EXPORT __declspec( dllexport )
#     else
#       define CH_EDITOR_EXPORT __declspec( dllimport )
#     endif
#   endif
# else  //Any other Compiler
#   if defined( CH_STATIC_LIB )
#     define CH_EDITOR_EXPORT
#   else
#     if defined( CH_EDITOR_EXPORTS )
#       define CH_EDITOR_EXPORT __attribute__ ((dllexport))
#     else
#       define CH_EDITOR_EXPORT __attribute__ ((dllimport))
#     endif
#   endif
# endif
# define CH_CORE_HIDDEN
#else //Linux/Mac settings
# if defined( CH_EDITOR_EXPORTS )
#   define CH_EDITOR_EXPORT __attribute__ ((visibility ("default")))
# else
#   define CH_EDITOR_EXPORT __attribute__ ((visibility ("default")))
# endif

# define CH_CORE_HIDDEN __attribute__ ((visibility ("hidden")))
#endif

struct ImVec4;

namespace chEngineSDK {
class ContentAssetUI;
class MainMenuBarUI;
class OutputLogUI;

class MultiStageRenderer;

class CH_EDITOR_EXPORT EditorApplication : public WindowedApplication
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
  onPresent(const RendererOutput& rendererOutput, const SPtr<ICommandBuffer>& commandBuffer,
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
  loadImporters();

  void
  setupSceneData();

 private:
  //SPtr<NastyRenderer> m_nastyRenderer; ///< The renderer used by the editor

  SPtr<MultiStageRenderer> m_multiStageRenderer;
  UUID m_gbufferStageId;

  SPtr<ISampler> m_defaultSampler;
  Map<SPtr<ITextureView>, SPtr<IDescriptorSet>> m_textureDescriptorSets;

  UniquePtr<ContentAssetUI> m_contentAssetUI; ///< Content Asset UI instance
  UniquePtr<MainMenuBarUI> m_mainMenuBar; ///< Main menu bar instance
  UniquePtr<OutputLogUI> m_outputLogUI; ///< Output log UI instance

  uint32 width = 0; ///< Width of the editor window
  uint32 height = 0; ///< Height of the editor window
};
} // namespace chEngineSDK`
