/************************************************************************/
/**
 * @file Nastyyyy!
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chCamera.h"
#include "chEventSystem.h"
#include "chIRenderer.h"
#include "chGraphicsTypes.h"
#include "chPipelineCache.h"

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

  FORCEINLINE void
  setClearColors(const Vector<LinearColor>& clearColors) override
  { m_clearColors = clearColors; }

  FORCEINLINE void
  setFocused(bool focused) { m_bIsfocused = focused; }

  void loadModel(const SPtr<Model>& model);
  void bindInputEvents();

  /**
   * Texture drawn on the model; null goes back to plain white.
   */
  void
  setTexture(const SPtr<ITexture>& texture);

 private:

  void
  createMeshBuffers();


  void
  createRenderTargets();

  void
  initializeRenderResources();

  void
  renderModel(ICommandBuffer& commandBuffer, float deltaTime);

  void
  cleanupModelResources();

  bool m_bIsfocused = false;

  SPtr<ITexture> m_colorTarget;
  SPtr<ITextureView> m_colorTargetView;
  SPtr<ITexture> m_depthTarget;
  SPtr<ITextureView> m_depthTargetView;

  Vector<LinearColor> m_clearColors;

  SPtr<ICommandPool> m_commandPool;
  SPtr<ICommandBuffer> m_commandBuffer;

  SPtr<IFence> m_renderFence;

  uint32 m_renderWidth = 1280;
  uint32 m_renderHeight = 720;

  SPtr<Camera> m_camera;
  SPtr<Model> m_currentModel;

  SPtr<Scene> m_activeScene;

  SPtr<IShader> m_vertexShader;
  SPtr<IShader> m_fragmentShader;
  PipelineCache m_pipelineCache;
  // Owned by m_pipelineCache; the targets never change format, so it is looked up once.
  const IPipeline* m_pipeline = nullptr;
  // Camera matrices read by the shader through the bindless heap.
  SPtr<IBuffer> m_cameraBuffer;

  Vector<SPtr<IBuffer>> m_meshVertexBuffers;
  Vector<SPtr<IBuffer>> m_meshIndexBuffers;
  Vector<uint32> m_meshIndexCounts;
  Vector<IndexType> m_meshIndexTypes;
  UnorderedMap<SPtr<Mesh>, uint32> m_meshToIndexMap;

  SPtr<ITexture> m_texture;
  // 1x1 white, used until a texture is set.
  SPtr<ITexture> m_defaultTexture;
  SPtr<ISampler> m_sampler;

  HEvent listenKeyDown;
  HEvent listenKeys;
  HEvent listenWheel;
  HEvent listenMouse;
};
} // namespace chEngineSDK
