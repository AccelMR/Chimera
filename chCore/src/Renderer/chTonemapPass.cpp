/************************************************************************/
/**
 * @file chTonemapPass.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief Turns the HDR scene color into the colors the screen shows.
 */
/************************************************************************/

#include "chTonemapPass.h"

#include "chConsoleVariable.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chIShader.h"
#include "chITextureView.h"

namespace chEngineSDK {

namespace {
ConsoleVariable<float> g_cvarExposure("Renderer.Exposure",
                                      1.0f,
                                      "Multiplies the scene light before the tonemap curve.",
                                      "Exposure");

// Must match PushConstants in tonemap.hlsl.
struct TonemapPushConstants
{
  uint32 sceneColorIndex;
  float exposure;
};
} // namespace

/*
 */
TonemapPass::TonemapPass()
{
  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  m_vertexShader = graphicsAPI.loadShader(ShaderStage::Vertex, "tonemap");
  m_fragmentShader = graphicsAPI.loadShader(ShaderStage::Fragment, "tonemap");
}

/*
 */
TonemapPass::~TonemapPass() = default;

/*
 */
void
TonemapPass::addPass(RenderGraph& graph, RGTextureHandle sceneColor, RGTextureHandle output)
{
  updatePipeline(graph.getTextureDesc(output).format);

  graph.addPass("Tonemap")
      .read(sceneColor)
      .writeColor(output, LoadOp::DontCare)
      .setExecute([this, sceneColor](RenderPassContext& context) {
        ICommandList& commandList = context.getCommandList();
        const TonemapPushConstants pushConstants{
            .sceneColorIndex = context.getView(sceneColor).getBindlessIndex(),
            .exposure = g_cvarExposure.get()};
        commandList.bindPipeline(*m_pipeline);
        commandList.pushConstants(&pushConstants, sizeof(pushConstants));
        commandList.draw(3);
      });
}

/*
 */
void
TonemapPass::updatePipeline(Format outputFormat)
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
