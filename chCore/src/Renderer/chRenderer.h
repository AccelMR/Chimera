/************************************************************************/
/**
 * @file chRenderer.h
 * @author AccelMR
 * @date 2020/12/11
 *   Renderer  clas that handles rendering of the engine.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesCore.h"
#include "chPrerequisitesGraphics.h"

#include "chModule.h"

namespace chEngineSDK
{
class Renderer : public Module<Renderer>
{
 public:
  Renderer() = default;
  virtual ~Renderer() = default;

  /**
   * @brief Initializes the renderer.
   * @param screen The screen to be used.
   */
  void
  initialize();

  void 
  render();

 private:
  SPtr<GPUCommandBuffer> m_commandBuffer;
  SPtr<GPUPipelineState> m_gBufferPipeline;
  SPtr<GPUPipelineState> m_lightingPipeline;
  SPtr<RenderPass> m_renderPass;
  SPtr<Framebuffer> m_frameBuffer;
  SPtr<Texture> m_albedo;
  SPtr<Texture> m_normal;
  SPtr<Texture> m_lightingOutput;
  SPtr<Sampler> m_sampler;

  SPtr<VertexBuffer> m_vertexBuffer;
  SPtr<IndexBuffer> m_indexBuffer;
  SPtr<GPUBuffer> m_mvpBuffer;

  SPtr<SwapChain> m_swapChain;
};
} // schEngineSDK
