/************************************************************************/
/**
 * @file chRenderer.h
 * @author AccelMR
 * @date 2025/04/09
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chGraphicsTypes.h"
#include "chModule.h"


namespace chEngineSDK {
class CH_CORE_EXPORT Renderer : public Module<Renderer> {
 public:
  virtual ~Renderer() = default;

  void 
  initialize(WeakPtr<DisplaySurface> displaySurface,
             uint32 width, 
             uint32 height, 
             bool vsync = false);

  void
  render();

 private:
  void
  createSyncObjects();

  void
  initializeRenderResources();

 private:
  SPtr<ISwapChain> m_swapChain;

  SPtr<ICommandPool> m_commandPool;
  Vector<SPtr<ICommandBuffer>> m_commandBuffers;
  SPtr<IPipeline> m_pipeline;
  SPtr<IBuffer> m_vertexBuffer;
  
  // Sincronización
  Vector<SPtr<ISemaphore>> m_imageAvailableSemaphores;
  Vector<SPtr<ISemaphore>> m_renderFinishedSemaphores;
  Vector<SPtr<IFence>> m_inFlightFences;
  
  uint32 m_currentFrame = 0;
  static constexpr uint32 MAX_FRAMES_IN_FLIGHT = 2;
};
} // namespace chEngineSDK