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
class CH_EDITOR_EXPORT EditorApplication : public WindowedApplication {
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
            uint32 swapChainWidth,
            uint32 swapChainHeight);

 private:
  void
  initializeEditorComponents();

  void
  bindEvents();

  void
  initImGui(const SPtr<DisplaySurface>& display);

  void
  openFileExplorer(const Path& pathToOpen, const Vector<String>& filters = {});

  void
  renderFullScreenRenderer(const RendererOutput& rendererOutput);


  /**
   * Renders the improved Content Browser window with search and filtering capabilities
   */
  void
  renderContentBrowser();

  void
  renderDeleteConfirmation();

  /**
   * Handles asset selection and loading
   * @param asset The asset to handle
   */
  void
  handleAssetSelection(const SPtr<IAsset>& asset);

  /**
   * Renders the context menu for assets
   * @param asset The asset to show context menu for
   */
  void
  renderAssetContextMenu(const SPtr<IAsset>& asset);

  /**
   * Gets the color for asset state visualization
   * @param asset The asset to get state color for
   * @return ImVec4 color for the asset state
   */
  ImVec4
  getAssetStateColor(const SPtr<IAsset>& asset);

  /**
   * Gets the string representation of asset state
   * @param asset The asset to get state string for
   * @return String representation of the asset state
   */
  String
  getAssetStateString(const SPtr<IAsset>& asset);

 private:
  SPtr<NastyRenderer> m_nastyRenderer; ///< The renderer used by the editor

  SPtr<ISampler> m_defaultSampler;
  Map<SPtr<ITextureView>, SPtr<IDescriptorSet>> m_textureDescriptorSets;

  Vector<SPtr<IAsset>> m_assets;
  SPtr<IAsset> m_assetToDelete; ///< Asset to delete, set when delete is requested

  bool m_showDeleteConfirmation = false; ///< Flag to show delete confirmation popup
};
} // namespace chEngineSDK
