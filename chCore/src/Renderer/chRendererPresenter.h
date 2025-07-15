/************************************************************************/
/**
 * @file chRendererPresenter.h
 * @author AccelMR
 * @date 2025/07/14
 * @brief  Utility class for presenting renderer output to swap chain.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
/**
 * @brief Utility class for presenting renderer output to swap chain
 */
class CH_CORE_EXPORT RendererPresenter {
public:
  RendererPresenter();
  ~RendererPresenter();

  /**
   * @brief Initialize presenter with swap chain format
   */
  void initialize(Format swapChainFormat);

  /**
   * @brief Present renderer output to swap chain target
   * @param rendererOutput The output from renderer
   * @param targetView The swap chain texture view to present to
   * @param commandBuffer Command buffer to record into
   */
  void present(const RendererOutput& rendererOutput,
               SPtr<ITextureView> targetView,
               const SPtr<ICommandBuffer>& commandBuffer);

  /**
   * @brief Present with optional post-processing effects
   */
  void presentWithEffects(const RendererOutput& rendererOutput,
                         SPtr<ITextureView> targetView,
                         const SPtr<ICommandBuffer>& commandBuffer,
                         const LinearColor& clearColor = LinearColor::Black);

private:
  void createPresentationPipeline(Format targetFormat);

  // Presentation resources
  SPtr<IShader> m_presentVertexShader;
  SPtr<IShader> m_presentFragmentShader;
  SPtr<IPipeline> m_presentPipeline;
  SPtr<IRenderPass> m_presentRenderPass;
  SPtr<ISampler> m_presentSampler;
  SPtr<IDescriptorSetLayout> m_presentDescriptorLayout;
  SPtr<IDescriptorPool> m_presentDescriptorPool;
  SPtr<IDescriptorSet> m_presentDescriptorSet;

  // Fullscreen quad (for presenting)
  SPtr<IBuffer> m_fullscreenQuadBuffer;
};

} // namespace chEngineSDK
