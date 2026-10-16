/************************************************************************/
/**
 * @file chSkyPass.cpp
 * @author AccelMR
 * @date 2026/10/15
 * @brief Draws the environment image of the sky light behind the scene.
 */
/************************************************************************/

#include "chSkyPass.h"

#include "chCamera.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chISampler.h"
#include "chIShader.h"
#include "chITexture.h"
#include "chITextureView.h"
#include "chSkyLightItem.h"

namespace chEngineSDK {

namespace {
// Must match PushConstants in sky.hlsl.
struct SkyPushConstants
{
  Matrix4 clipToSky;
  uint32 skyIndex;
  uint32 samplerIndex;
  uint32 depthIndex;
  float intensity;
};
static_assert(sizeof(SkyPushConstants) <= GraphicsLimits::PUSH_CONSTANTS_SIZE);
} // namespace

/*
 */
SkyPass::SkyPass()
{
  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  m_vertexShader = graphicsAPI.loadShader(ShaderStage::Vertex, "sky");
  m_fragmentShader = graphicsAPI.loadShader(ShaderStage::Fragment, "sky");
  // The longitude wraps around; the latitude stops at the poles.
  m_sampler = graphicsAPI.createSampler({.magFilter = SamplerFilter::Linear,
                                         .minFilter = SamplerFilter::Linear,
                                         .mipmapMode = SamplerMipmapMode::Linear,
                                         .addressModeU = SamplerAddressMode::Repeat,
                                         .addressModeV = SamplerAddressMode::ClampToEdge,
                                         .addressModeW = SamplerAddressMode::ClampToEdge});
}

/*
 */
SkyPass::~SkyPass() = default;

/*
 */
void
SkyPass::addPass(RenderGraph& graph,
                 const Camera& camera,
                 const SkyLightItem* skyLight,
                 RGTextureHandle sceneColor,
                 RGTextureHandle depth)
{
  if (skyLight == nullptr || skyLight->texture == nullptr || !skyLight->bShowSky ||
      !depth.isValid()) {
    return;
  }
  updatePipeline(graph.getTextureDesc(sceneColor).format);

  // Without the camera position the clip space goes back to directions from the camera.
  Matrix4 viewRotation = camera.getViewMatrix();
  viewRotation[3][0] = 0.0f;
  viewRotation[3][1] = 0.0f;
  viewRotation[3][2] = 0.0f;
  const Matrix4 clipToWorld = (viewRotation * camera.getProjectionMatrix()).getInverse();
  const SkyPushConstants pushConstants{.clipToSky = clipToWorld * skyLight->worldToSky,
                                       .skyIndex = skyLight->texture->getBindlessIndex(),
                                       .samplerIndex = m_sampler->getBindlessIndex(),
                                       .depthIndex = GraphicsLimits::INVALID_BINDLESS_INDEX,
                                       .intensity = skyLight->intensity};

  graph.addPass("Sky")
      .read(depth)
      .writeColor(sceneColor, LoadOp::Load)
      .setExecute([this, pushConstants, depth](RenderPassContext& context) {
        ICommandList& commandList = context.getCommandList();
        SkyPushConstants constants = pushConstants;
        constants.depthIndex = context.getView(depth).getBindlessIndex();
        commandList.bindPipeline(*m_pipeline);
        commandList.pushConstants(&constants, sizeof(constants));
        commandList.draw(3);
      });
}

/*
 */
void
SkyPass::updatePipeline(Format outputFormat)
{
  if (m_pipeline != nullptr && m_pipelineFormat == outputFormat) {
    return;
  }

  GraphicsPipelineDesc pipelineDesc{.vertexShader = m_vertexShader,
                                    .fragmentShader = m_fragmentShader,
                                    .raster = {.cullMode = CullMode::None},
                                    .depth = {.testEnable = false, .writeEnable = false},
                                    .colorAttachmentCount = 1};
  pipelineDesc.colorFormats[0] = outputFormat;
  m_pipeline = m_pipelineCache.getOrCreate(pipelineDesc).get();
  m_pipelineFormat = outputFormat;
}

} // namespace chEngineSDK
