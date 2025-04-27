/************************************************************************/
/**
 * @file chRenderer.h
 * @author AccelMR
 * @date 2025/04/09
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chCamera.h"
#include "chGraphicsTypes.h"
#include "chModule.h"


namespace chEngineSDK {
class CH_CORE_EXPORT Renderer : public Module<Renderer> {
 public:
  virtual ~Renderer();

  void 
  initialize(uint32 width, 
             uint32 height, 
             bool vsync = false);

  void
  render(const float deltaTime);

 private:
  void
  createSyncObjects();

  void
  initializeRenderResources();

 private:
  void
  resize();

  void
  createRenderPass();

 private:
  SPtr<ISwapChain> m_swapChain;

  SPtr<ICommandPool> m_commandPool;
  Vector<SPtr<ICommandBuffer>> m_commandBuffers;
  SPtr<IPipeline> m_pipeline;
  SPtr<IRenderPass> m_renderPass;
  Vector<SPtr<IFrameBuffer>> m_framebuffers;
  SPtr<IBuffer> m_viewProjectionBuffer;
  SPtr<ISampler> m_sampler;
  SPtr<ITexture> m_texture;
  SPtr<ITextureView> m_textureView;
  SPtr<ITexture> m_depthTexture;
  SPtr<ITextureView> m_depthTextureView;

  SPtr<Model> m_currentModel;
  Vector<SPtr<IBuffer>> m_meshVertexBuffers;
  Vector<SPtr<IBuffer>> m_meshIndexBuffers;
  Vector<uint32> m_meshIndexCounts;
  Vector<IndexType> m_meshIndexTypes;
  
  SPtr<IDescriptorSetLayout> m_descriptorSetLayout;
  SPtr<IDescriptorPool> m_descriptorPool;
  SPtr<IDescriptorSet> m_descriptorSet;
  
  
  // Sincronización
  Vector<SPtr<ISemaphore>> m_imageAvailableSemaphores;
  Vector<SPtr<ISemaphore>> m_renderFinishedSemaphores;
  Vector<SPtr<IFence>> m_inFlightFences;

  SPtr<IShader> m_vertexShader;
  SPtr<IShader> m_fragmentShader;

  UniquePtr<Camera> m_camera;
  
  uint32 m_currentFrame = 0;
  static constexpr uint32 MAX_FRAMES_IN_FLIGHT = 2;

  uint32 m_width = 0;
  uint32 m_height = 0;
};
} // namespace chEngineSDK