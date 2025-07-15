/************************************************************************/
/**
 * @file Nastyyyy!
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chCamera.h"
#include "chIRenderer.h"
#include "chGraphicsTypes.h"

namespace chEngineSDK {
class ModelNode;
class CH_CORE_EXPORT NastyRenderer : public IRenderer {
public:
  NastyRenderer();
  virtual ~NastyRenderer();

  void
  initialize(uint32 width, uint32 height) override;

  RendererOutput
  onRender(float deltaTime) override;
  void
  resize(uint32 width, uint32 height) override;

  void
  cleanup() override;

  uint32
  getWidth() const override { return m_renderWidth; }

  uint32
  getHeight() const override { return m_renderHeight; }

  void
  setClearColors(const Vector<LinearColor>& clearColors) override{
    m_clearColors = clearColors;
  }

  void loadModel(const SPtr<Model>& model);
  void bindInputEvents();

 private:

  void
  createMeshBuffers();

  void
  createNodeDescriptorResources();

  void
  createRenderTargets();

  void
  createRenderPass();

  void
  initializeRenderResources();

  void
  renderModel(const SPtr<ICommandBuffer>& commandBuffer, float deltaTime);

  void
  cleanupModelResources();

  SPtr<ITexture> m_colorTarget;
  SPtr<ITextureView> m_colorTargetView;
  SPtr<ITexture> m_depthTarget;
  SPtr<ITextureView> m_depthTargetView;

  Vector<LinearColor> m_clearColors;

  SPtr<IRenderPass> m_renderPass;
  SPtr<IFrameBuffer> m_framebuffer;

  SPtr<ICommandPool> m_commandPool;
  SPtr<ICommandBuffer> m_commandBuffer;

  SPtr<IFence> m_renderFence;

  uint32 m_renderWidth = 1280;
  uint32 m_renderHeight = 720;

  SPtr<Camera> m_camera;
  SPtr<Model> m_currentModel;

  SPtr<IShader> m_vertexShader;
  SPtr<IShader> m_fragmentShader;
  SPtr<IPipeline> m_pipeline;

  Vector<SPtr<IBuffer>> m_meshVertexBuffers;
  Vector<SPtr<IBuffer>> m_meshIndexBuffers;
  Vector<uint32> m_meshIndexCounts;
  Vector<IndexType> m_meshIndexTypes;
  UnorderedMap<SPtr<Mesh>, uint32> m_meshToIndexMap;

  SPtr<ITexture> m_texture;
  SPtr<ITextureView> m_textureView;
  SPtr<ISampler> m_sampler;
  SPtr<IDescriptorSetLayout> m_descriptorSetLayout;
  SPtr<IDescriptorPool> m_descriptorPool;

  struct NodeRenderResources {
    SPtr<IBuffer> uniformBuffer;
    SPtr<IDescriptorSet> descriptorSet;
  };
  UnorderedMap<ModelNode*, NodeRenderResources> m_nodeResources;
};
} // namespace chEngineSDK
