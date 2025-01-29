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
  SPtr<GPUPipelineState> m_pipeline;
  SPtr<SwapChain> m_swapChain;
};
} // schEngineSDK
