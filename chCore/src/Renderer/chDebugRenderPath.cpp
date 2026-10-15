/************************************************************************/
/**
 * @file chDebugRenderPath.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief Draws scene data (unlit texture, normals, depth, UVs, edges) instead of the lit scene.
 */
/************************************************************************/

#include "chDebugRenderPath.h"

#include "chCamera.h"
#include "chIBuffer.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chISampler.h"
#include "chIShader.h"
#include "chMaterial.h"
#include "chMatrix4.h"
#include "chMesh.h"
#include "chRenderItem.h"

namespace chEngineSDK {

namespace {
// Must match the kMode constants in debugview.hlsl.
enum class DebugMode : uint32
{
  Unlit,
  Normals,
  Depth,
  TexCoords,
  Color
};

// Must match CameraData in debugview.hlsl.
struct DebugCameraData
{
  Matrix4 view;
  Matrix4 projection;
  float nearPlane;
  float farPlane;
  float padding[2];
};

// Must match PushConstants in debugview.hlsl.
struct DebugPushConstants
{
  Matrix4 model;
  float color[4];
  uint32 cameraIndex;
  uint32 materialIndex;
  uint32 samplerIndex;
  uint32 mode;
};
static_assert(sizeof(DebugPushConstants) <= GraphicsLimits::PUSH_CONSTANTS_SIZE);

constexpr Format kDepthFormat = Format::D32_SFLOAT;

// Lines alone are drawn light on the background; over the lit scene they need a color that
// stands out from textures.
constexpr float kLineColor[4] = {0.85f, 0.85f, 0.85f, 1.0f};
constexpr float kOverlayLineColor[4] = {0.1f, 0.9f, 0.35f, 1.0f};

NODISCARD DebugMode
toDebugMode(ViewMode mode)
{
  switch (mode) {
    case ViewMode::Normals:
      return DebugMode::Normals;
    case ViewMode::Depth:
      return DebugMode::Depth;
    case ViewMode::TexCoords:
      return DebugMode::TexCoords;
    case ViewMode::Wireframe:
    case ViewMode::LitWireframe:
      return DebugMode::Color;
    case ViewMode::Lit:
    case ViewMode::Unlit:
    case ViewMode::BaseColor:
    case ViewMode::Metallic:
    case ViewMode::Roughness:
    case ViewMode::AmbientOcclusion:
    case ViewMode::Emissive:
    case ViewMode::ShadowCascades:
    case ViewMode::ShadowMaps:
    case ViewMode::COUNT:
      break;
  }
  return DebugMode::Unlit;
}
} // namespace

/*
 */
DebugRenderPath::DebugRenderPath()
{
  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();

  for (SPtr<IBuffer>& cameraBuffer : m_cameraBuffers) {
    cameraBuffer = graphicsAPI.createBuffer({.size = sizeof(DebugCameraData),
                                             .usage = BufferUsage::UniformBuffer,
                                             .memoryUsage = MemoryUsage::CpuToGpu});
  }

  m_sampler = graphicsAPI.createSampler({.magFilter = SamplerFilter::Linear,
                                         .minFilter = SamplerFilter::Linear,
                                         .mipmapMode = SamplerMipmapMode::Linear,
                                         .addressModeU = SamplerAddressMode::Repeat,
                                         .addressModeV = SamplerAddressMode::Repeat,
                                         .addressModeW = SamplerAddressMode::Repeat});

  m_vertexShader = graphicsAPI.loadShader(ShaderStage::Vertex, "debugview");
  m_fragmentShader = graphicsAPI.loadShader(ShaderStage::Fragment, "debugview");
  m_maskedFragmentShader = graphicsAPI.loadShader(ShaderStage::Fragment, "debugview_masked");
}

/*
 */
DebugRenderPath::~DebugRenderPath() = default;

/*
 */
void
DebugRenderPath::setViewMode(ViewMode mode)
{
  CH_ASSERT(mode != ViewMode::Lit && mode != ViewMode::LitWireframe && mode < ViewMode::COUNT);
  m_viewMode = mode;
}

/*
 */
RGTextureHandle
DebugRenderPath::addPasses(RenderGraph& graph, const RenderView& view, RGTextureHandle output)
{
  const RGTextureDesc outputDesc = graph.getTextureDesc(output);

  if (m_viewMode == ViewMode::Wireframe) {
    // Without depth, so the edges behind are seen too.
    const DrawSettings settings{
        .camera = &view.camera,
        .items = view.items,
        .pipelines = &getPipelines(PipelineKind::Lines, outputDesc.format, Format::Unknown),
        .shaderMode = static_cast<uint32>(DebugMode::Color),
        .color = kLineColor};
    graph.addPass("Wireframe")
        .writeColor(output, LoadOp::Clear, view.clearColor)
        .setExecute([this, settings](RenderPassContext& context) {
          recordDraws(context, settings);
        });
    return {};
  }

  const RGTextureHandle depth = graph.createTexture(
      "Depth", {.format = kDepthFormat, .width = outputDesc.width, .height = outputDesc.height});
  const DrawSettings settings{
      .camera = &view.camera,
      .items = view.items,
      .pipelines = &getPipelines(PipelineKind::Solid, outputDesc.format, kDepthFormat),
      .shaderMode = static_cast<uint32>(toDebugMode(m_viewMode)),
      .color = kLineColor};
  graph.addPass("DebugView")
      .writeColor(output, LoadOp::Clear, view.clearColor)
      .writeDepth(depth)
      .setExecute([this, settings](RenderPassContext& context) {
        recordDraws(context, settings);
      });
  return depth;
}

/*
 */
void
DebugRenderPath::addWireframeOverlay(RenderGraph& graph,
                                     const RenderView& view,
                                     RGTextureHandle output,
                                     RGTextureHandle depth)
{
  const DrawSettings settings{
      .camera = &view.camera,
      .items = view.items,
      .pipelines = &getPipelines(PipelineKind::LinesOverDepth,
                               graph.getTextureDesc(output).format,
                               graph.getTextureDesc(depth).format),
      .shaderMode = static_cast<uint32>(DebugMode::Color),
      .color = kOverlayLineColor};

  graph.addPass("WireframeOverlay")
      .writeColor(output, LoadOp::Load)
      .writeDepth(depth, LoadOp::Load)
      .setExecute([this, settings](RenderPassContext& context) {
        recordDraws(context, settings);
      });
}

/*
 */
void
DebugRenderPath::recordDraws(RenderPassContext& context, const DrawSettings& settings)
{
  ICommandList& commandList = context.getCommandList();
  const Camera& camera = *settings.camera;

  IBuffer& cameraBuffer = *m_cameraBuffers[IGraphicsAPI::instance().getFrameIndex()];
  const DebugCameraData cameraData{.view = camera.getViewMatrix(),
                                   .projection = camera.getProjectionMatrix(),
                                   .nearPlane = camera.getNearClipPlane(),
                                   .farPlane = camera.getFarClipPlane(),
                                   .padding = {0.0f, 0.0f}};
  cameraBuffer.update(&cameraData, sizeof(cameraData));

  const float* color = settings.color;
  DebugPushConstants pushConstants{.model = Matrix4::IDENTITY,
                                   .color = {color[0], color[1], color[2], color[3]},
                                   .cameraIndex = cameraBuffer.getBindlessIndex(),
                                   .materialIndex = 0,
                                   .samplerIndex = m_sampler->getBindlessIndex(),
                                   .mode = settings.shaderMode};
  settings.pipelines->recordDraws(
      commandList, settings.items, m_defaultMaterial,
      [&commandList, &pushConstants](const RenderItem& item, const Material& material) {
        pushConstants.model = item.worldMatrix;
        pushConstants.materialIndex = material.getConstantBuffer().getBindlessIndex();
        commandList.pushConstants(&pushConstants, sizeof(pushConstants));
      });
}

/*
 */
const MaterialPipelines&
DebugRenderPath::getPipelines(PipelineKind kind, Format colorFormat, Format depthFormat)
{
  PipelineEntry& entry = m_pipelines[static_cast<uint32>(kind)];
  if (entry.pipelines.isBuilt() && entry.colorFormat == colorFormat &&
      entry.depthFormat == depthFormat) {
    return entry.pipelines;
  }

  GraphicsPipelineDesc pipelineDesc{.vertexShader = m_vertexShader,
                                    .fragmentShader = m_fragmentShader,
                                    .vertexLayout = VertexStatic::getLayout(),
                                    .colorAttachmentCount = 1,
                                    .depthFormat = depthFormat};
  pipelineDesc.colorFormats[0] = colorFormat;
  // Lines show every triangle, also the parts a masked material cuts away.
  const SPtr<IShader>* maskedFragmentShader = &m_maskedFragmentShader;
  if (kind != PipelineKind::Solid) {
    pipelineDesc.raster.polygonMode = PolygonMode::Line;
    pipelineDesc.raster.cullMode = CullMode::None;
    maskedFragmentShader = &m_fragmentShader;
  }
  if (kind == PipelineKind::LinesOverDepth) {
    // Pulled towards the camera, so the edges of a surface pass the test against the depth
    // that surface wrote; they must not write it, or the next lines would fail.
    pipelineDesc.raster.depthBiasConstant = -4.0f;
    pipelineDesc.raster.depthBiasSlope = -1.5f;
    pipelineDesc.depth.writeEnable = false;
    pipelineDesc.depth.compareOp = CompareOp::LessOrEqual;
  }

  entry.pipelines.build(m_pipelineCache, pipelineDesc, *maskedFragmentShader);
  entry.colorFormat = colorFormat;
  entry.depthFormat = depthFormat;
  return entry.pipelines;
}

} // namespace chEngineSDK
