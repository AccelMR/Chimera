/************************************************************************/
/**
 * @file chForwardRenderPath.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief Draws the scene items lit by the scene lights, in one pass, into the output.
 */
/************************************************************************/

#include "chForwardRenderPath.h"

#include "chCamera.h"
#include "chIBuffer.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chISampler.h"
#include "chIShader.h"
#include "chLightItem.h"
#include "chMaterial.h"
#include "chMath.h"
#include "chMatrix4.h"
#include "chMesh.h"
#include "chRenderItem.h"
#include "chRenderSettings.h"

namespace chEngineSDK {

namespace {
// Must match ViewData in cube.hlsl.
struct ViewData
{
  Matrix4 view;
  Matrix4 projection;
  float cameraPosition[3];
  uint32 lightCount;
  uint32 lightBufferIndex;
  float ambient;
  float padding[2];
};

// Must match PushConstants in cube.hlsl.
struct DrawPushConstants
{
  Matrix4 model;
  uint32 viewIndex;
  uint32 materialIndex;
  uint32 samplerIndex;
  uint32 padding;
};
static_assert(sizeof(DrawPushConstants) <= GraphicsLimits::PUSH_CONSTANTS_SIZE);

constexpr Format kDepthFormat = Format::D32_SFLOAT;

// Enough for a small scene, so most frames never make the light buffers again.
constexpr uint32 kInitialLightCapacity = 16;

NODISCARD SPtr<IBuffer>
createLightBuffer(uint32 capacity, SIZE_T lightSize)
{
  return IGraphicsAPI::instance().createBuffer(
      {.size = static_cast<uint32>(capacity * lightSize),
       .usage = BufferUsage::StorageBuffer,
       .memoryUsage = MemoryUsage::CpuToGpu});
}
} // namespace

/*
 */
ForwardRenderPath::ForwardRenderPath()
{
  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();

  for (SPtr<IBuffer>& viewBuffer : m_viewBuffers) {
    viewBuffer = graphicsAPI.createBuffer({.size = sizeof(ViewData),
                                           .usage = BufferUsage::UniformBuffer,
                                           .memoryUsage = MemoryUsage::CpuToGpu});
  }
  for (uint32 i = 0; i < GraphicsLimits::MAX_FRAMES_IN_FLIGHT; ++i) {
    m_lightBuffers[i] = createLightBuffer(kInitialLightCapacity, sizeof(GpuLight));
    m_lightBufferCapacities[i] = kInitialLightCapacity;
  }
  m_gpuLights.reserve(kInitialLightCapacity);

  m_sampler = graphicsAPI.createSampler({.magFilter = SamplerFilter::Linear,
                                         .minFilter = SamplerFilter::Linear,
                                         .mipmapMode = SamplerMipmapMode::Linear,
                                         .addressModeU = SamplerAddressMode::Repeat,
                                         .addressModeV = SamplerAddressMode::Repeat,
                                         .addressModeW = SamplerAddressMode::Repeat});

  m_vertexShader = graphicsAPI.loadShader(ShaderStage::Vertex, "cube");
  m_fragmentShader = graphicsAPI.loadShader(ShaderStage::Fragment, "cube");
}

/*
 */
ForwardRenderPath::~ForwardRenderPath() = default;

/*
 */
RGTextureHandle
ForwardRenderPath::addPasses(RenderGraph& graph,
                             const RenderView& view,
                             RGTextureHandle output)
{
  const RGTextureDesc outputDesc = graph.getTextureDesc(output);
  const RGTextureHandle depth = graph.createTexture(
      "Depth", {.format = kDepthFormat, .width = outputDesc.width, .height = outputDesc.height});

  // Looked up now, while building, so the pass code does not depend on the output format.
  updatePipeline(outputDesc.format);
  updateViewData(view);

  graph.addPass("Forward")
      .writeColor(output, LoadOp::Clear, view.clearColor)
      .writeDepth(depth)
      .setExecute([this, items = view.items](RenderPassContext& context) {
        recordDraws(context, items);
      });
  return depth;
}

/*
 */
void
ForwardRenderPath::updateViewData(const RenderView& view)
{
  const uint32 frameIndex = IGraphicsAPI::instance().getFrameIndex();
  const uint32 lightCount = static_cast<uint32>(view.lights.size());

  m_gpuLights.clear();
  for (const LightItem* light : view.lights) {
    // Spot falloff as glTF defines it: 0 at the outer cone, 1 at the inner one.
    float spotScale = 0.0f;
    float spotOffset = 1.0f;
    if (light->type == LightType::Spot) {
      spotScale = 1.0f / Math::max(light->innerConeCos - light->outerConeCos, 1e-4f);
      spotOffset = -light->outerConeCos * spotScale;
    }
    m_gpuLights.push_back(
        {.position = {light->position.x, light->position.y, light->position.z},
         .type = static_cast<uint32>(light->type),
         .direction = {light->direction.x, light->direction.y, light->direction.z},
         .range = light->range,
         .color = {light->color.r, light->color.g, light->color.b},
         .spotScale = spotScale,
         .spotOffset = spotOffset,
         .padding = {0.0f, 0.0f, 0.0f}});
  }

  uint32& capacity = m_lightBufferCapacities[frameIndex];
  if (lightCount > capacity) {
    capacity = Math::max(lightCount, capacity * 2);
    m_lightBuffers[frameIndex] = createLightBuffer(capacity, sizeof(GpuLight));
  }
  IBuffer& lightBuffer = *m_lightBuffers[frameIndex];
  if (lightCount > 0) {
    lightBuffer.update(m_gpuLights.data(), m_gpuLights.size() * sizeof(GpuLight));
  }

  const Camera& camera = view.camera;
  const Vector3& cameraPosition = camera.getPosition();
  const ViewData viewData{.view = camera.getViewMatrix(),
                          .projection = camera.getProjectionMatrix(),
                          .cameraPosition = {cameraPosition.x, cameraPosition.y,
                                             cameraPosition.z},
                          .lightCount = lightCount,
                          .lightBufferIndex = lightBuffer.getBindlessIndex(),
                          .ambient = g_cvarAmbient.get(),
                          .padding = {0.0f, 0.0f}};
  m_viewBuffers[frameIndex]->update(&viewData, sizeof(viewData));
}

/*
 */
void
ForwardRenderPath::recordDraws(RenderPassContext& context, Span<const RenderItem* const> items)
{
  ICommandList& commandList = context.getCommandList();
  const IBuffer& viewBuffer = *m_viewBuffers[IGraphicsAPI::instance().getFrameIndex()];

  commandList.bindPipeline(*m_pipeline);

  DrawPushConstants pushConstants{.model = Matrix4::IDENTITY,
                                  .viewIndex = viewBuffer.getBindlessIndex(),
                                  .materialIndex = 0,
                                  .samplerIndex = m_sampler->getBindlessIndex(),
                                  .padding = 0};
  for (const RenderItem* item : items) {
    const IBuffer* vertexBuffer = item->mesh->getVertexBuffer();
    const IBuffer* indexBuffer = item->mesh->getIndexBuffer();
    if (!vertexBuffer || !indexBuffer) {
      continue;
    }

    const Material& material = item->material ? *item->material : m_defaultMaterial;
    pushConstants.model = item->worldMatrix;
    pushConstants.materialIndex = material.getConstantBuffer().getBindlessIndex();
    commandList.pushConstants(&pushConstants, sizeof(pushConstants));

    commandList.bindVertexBuffer(*vertexBuffer);
    commandList.bindIndexBuffer(*indexBuffer, item->mesh->getIndexType());
    commandList.drawIndexed(item->mesh->getIndexCount());
  }
}

/*
 */
void
ForwardRenderPath::updatePipeline(Format colorFormat)
{
  if (m_pipeline != nullptr && m_pipelineFormat == colorFormat) {
    return;
  }

  GraphicsPipelineDesc pipelineDesc{.vertexShader = m_vertexShader,
                                    .fragmentShader = m_fragmentShader,
                                    .vertexLayout = VertexStatic::getLayout(),
                                    .colorAttachmentCount = 1,
                                    .depthFormat = kDepthFormat};
  pipelineDesc.colorFormats[0] = colorFormat;
  m_pipeline = m_pipelineCache.getOrCreate(pipelineDesc).get();
  m_pipelineFormat = colorFormat;
}

} // namespace chEngineSDK
