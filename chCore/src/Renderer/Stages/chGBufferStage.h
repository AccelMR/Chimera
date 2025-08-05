/************************************************************************/
/**
 * @file chGBufferStage.h
 * @author AccelMR
 * @date 2025/07/21
 * @brief G-Buffer generation stage for deferred rendering
 */
/************************************************************************/
#pragma once

#include "Data/chSceneData.h"
#include "Data/chTextureData.h"
#include "chRenderStage.h"

namespace chEngineSDK {

class GBufferStage;
// Declare type traits for this stage
DECLARE_TYPE_TRAITS(GBufferStage)

#if USING(CH_EDITOR)
enum class GBufferDisplayMode {
  Combined = 0,  // Normal G-Buffer output (all targets)
  AlbedoOnly,
  NormalOnly,
  MetallicOnly,
  RoughnessOnly,
  DepthOnly,
  MotionOnly,
  Count
};
#endif

/**
 * @brief G-Buffer generation stage
 * Takes camera and model data, outputs G-Buffer textures
 */
class CH_CORE_EXPORT GBufferStage : public IRenderStage
{
 public:
  GBufferStage();
  ~GBufferStage() override;

  // IRenderStage interface
  UUID
  getTypeId() const override {
    return TypeTraits<GBufferStage>::getTypeId();
  }

  String
  getName() const override {
    return "G-Buffer Stage";
  }

  Vector<UUID>
  getRequiredInputTypes() const override {
    return {
        TypeTraits<CameraData>::getTypeId()
        // TypeTraits<ModelData>::getTypeId()
    };
  }

  Vector<UUID>
  getProvidedOutputTypes() const override {
    return {TypeTraits<AlbedoData>::getTypeId(),   TypeTraits<NormalData>::getTypeId(),
            TypeTraits<MetallicData>::getTypeId(), TypeTraits<RoughnessData>::getTypeId(),
            TypeTraits<DepthData>::getTypeId(),    TypeTraits<MotionVectorData>::getTypeId()};
  }

  uint32
  getWidth() const {
    return m_width;
  }

  uint32
  getHeight() const {
    return m_height;
  }

  void
  initialize(uint32 width, uint32 height) override;

  bool
  execute(SPtr<ICommandBuffer> commandBuffer, const RenderStageIO& inputs, RenderStageIO& outputs, float deltaTime) override;

  void
  resize(uint32 width, uint32 height) override;

  void
  cleanup() override;

#if USING(CH_EDITOR)
  // Debug/Editor functionality
  void
  setDisplayMode(GBufferDisplayMode mode) {
    m_debugDisplayMode = mode;
  }
  GBufferDisplayMode
  getDisplayMode() const {
    return m_debugDisplayMode;
  }

  String
  getDisplayModeName() const {
    static const char* names[] = {"Combined",
                                  "Albedo Only",
                                  "Normal Only",
                                  "Metallic Only",
                                  "Roughness Only",
                                  "Depth Only",
                                  "Motion Only"};
    return String(names[static_cast<int>(m_debugDisplayMode)]);
  }

  bool
  isDebugMode() const {
    return m_debugDisplayMode != GBufferDisplayMode::Combined;
  }
#endif

 private:
  void
  createGBufferTargets();
  void
  createGBufferRenderPass();
  void
  createGBufferPipeline();
  void
  createCommandResources();

  SPtr<IPipeline>
  getPipelineForMaterial(const SPtr<IMaterial>& material) { return m_materialPipelines[material->getMaterialId()]; }

  bool
  renderGeometry(SPtr<Camera> camera, SPtr<Model> model, float deltaTime);

  SPtr<IPipeline>
  createPipelineForMaterial(const SPtr<IMaterial>& material);

  // G-Buffer render targets (6 targets)
  SPtr<ITexture> m_albedoTarget;    // RGB: albedo, A: unused
  SPtr<ITexture> m_normalTarget;    // RGB: world normal, A: unused
  SPtr<ITexture> m_metallicTarget;  // R: metallic, GBA: unused
  SPtr<ITexture> m_roughnessTarget; // R: roughness, GBA: unused
  SPtr<ITexture> m_depthTarget;     // Depth buffer
  SPtr<ITexture> m_motionTarget;    // RG: motion vectors, BA: unused

  // Texture views
  SPtr<ITextureView> m_albedoView;
  SPtr<ITextureView> m_normalView;
  SPtr<ITextureView> m_metallicView;
  SPtr<ITextureView> m_roughnessView;
  SPtr<ITextureView> m_depthView;
  SPtr<ITextureView> m_motionView;

  Map<UUID, SPtr<IPipeline>> m_materialPipelines;

  // Render resources
  SPtr<IRenderPass> m_renderPass;
  SPtr<IFrameBuffer> m_framebuffer;
  SPtr<ICommandPool> m_commandPool;
  SPtr<ICommandBuffer> m_commandBuffer;
  SPtr<IFence> m_fence;

#if USING(CH_EDITOR)
  // Debug display mode
  GBufferDisplayMode m_debugDisplayMode = GBufferDisplayMode::Combined;

  // Single target for debug display
  SPtr<ITexture> m_debugTarget;
  SPtr<ITextureView> m_debugView;
  SPtr<IRenderPass> m_debugRenderPass;
  SPtr<IFrameBuffer> m_debugFramebuffer;

  GBufferDisplayMode m_displayMode = GBufferDisplayMode::AlbedoOnly;
#endif

  // Clear colors for G-Buffer
  Vector<LinearColor> m_clearColors;
};
} // namespace chEngineSDK
