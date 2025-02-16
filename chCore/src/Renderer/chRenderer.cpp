/************************************************************************/
/**
 * @file chRenderer.cpp
 * @author AccelMR
 * @date 2020/12/11
 *   Renderer  clas that handles rendering of the engine.
 */
/************************************************************************/
#include "chRenderer.h"

#include "chCommandParser.h"
#include "chBox.h"
#include "chFence.h"
#include "chFileSystem.h"
#include "chGraphicsModule.h"
#include "chGPUResourceModule.h"
#include "chGPUResourceDescriptors.h"
#include "chLinearColor.h"
#include "chDebug.h"
#include "chMatrix4.h"
#include "chRadian.h"

namespace chEngineSDK
{
/*Delete this!!*/
struct MatrixViewProj
{
  LookAtMatrix View = 
    LookAtMatrix({0.0f, -20.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, Vector3::UP);
  PerspectiveMatrix Proj = 
    PerspectiveMatrix(Radian(0.7853982f), 1920, 1080, 0.1f, 100.0f);
};

struct Vertex
{
  Vector4 position;
};

/*
 */
using namespace chGPUDesc;

/*
 */
void 
Renderer::initialize() {
  GraphicsModule &GraphicAPI = GraphicsModule::instance();
  GPUResourceModule &GPUResourceMngr = GPUResourceModule::instance();

  m_swapChain = GraphicAPI.getSwapChain();

  CommandParser& commandParser = CommandParser::getInstance();
  const float width = std::stof(commandParser.getParam("Width", "1280"));
  const float height = std::stof(commandParser.getParam("Height", "720"));

  SPtr<Texture> gBufferAlbedo = GPUResourceMngr.createTexture({
    .dimensions = { static_cast<int32>(width), static_cast<int32>(height), 1},
    .usage = TEXTURE_USAGE::kUSAGE_RENDER_TARGET | TEXTURE_USAGE::kUSAGE_SAMPLED,
    .format = FORMAT::kB8G8R8A8_UNORM
  });
  CH_ASSERT(gBufferAlbedo != nullptr && "Fallo al crear gBufferAlbedo.");

  SPtr<Texture> gBufferNormals = GPUResourceMngr.createTexture({
    .dimensions = {static_cast<int32>(width), static_cast<int32>(height), 1},
    .usage = TEXTURE_USAGE::kUSAGE_RENDER_TARGET | TEXTURE_USAGE::kUSAGE_SAMPLED,
    .format = FORMAT::kR16G16B16A16_FLOAT
  });
  CH_ASSERT(gBufferNormals != nullptr && "Fallo al crear gBufferNormals.");

  SPtr<Texture> lightingOutput = GPUResourceMngr.createTexture({
    .dimensions = {static_cast<int32>(width), static_cast<int32>(height), 1},
    .usage = TEXTURE_USAGE::kUSAGE_RENDER_TARGET | TEXTURE_USAGE::kUSAGE_SAMPLED,
    .format = FORMAT::kB8G8R8A8_UNORM
  });
  CH_ASSERT(lightingOutput != nullptr && "Fallo al crear lightingOutput.");

  RenderPassDesc renderPassDesc;
  renderPassDesc.attachments = {
  {
    .format = FORMAT::kB8G8R8A8_UNORM,
    .sampleCount = SAMPLE_COUNT::kSAMPLE_COUNT_1,
    .loadOp = AttachmentDesc::LOAD_OP::kCLEAR,
    .storeOp = AttachmentDesc::STORE_OP::kSTORE,
    .initialLayout = LAYOUT::kUNDEFINED,
    .finalLayout = LAYOUT::kSHADER_READ_ONLY // For GBuffer Albedo
  },
  {
    .format = FORMAT::kR16G16B16A16_FLOAT,
    .sampleCount = SAMPLE_COUNT::kSAMPLE_COUNT_1,
    .loadOp = AttachmentDesc::LOAD_OP::kCLEAR,
    .storeOp = AttachmentDesc::STORE_OP::kSTORE,
    .initialLayout = LAYOUT::kUNDEFINED,
    .finalLayout = LAYOUT::kSHADER_READ_ONLY // For GBuffer Normals
  },
  {
    .format = FORMAT::kB8G8R8A8_UNORM,
    .sampleCount = SAMPLE_COUNT::kSAMPLE_COUNT_1,
    .loadOp = AttachmentDesc::LOAD_OP::kLOAD,
    .storeOp = AttachmentDesc::STORE_OP::kSTORE,
    .initialLayout = LAYOUT::kCOLOR_ATTACHMENT,
    .finalLayout = LAYOUT::kPRESENT // For Lighting Output
  }};

  renderPassDesc.subpasses = {
    { {}, {0, 1}, {} },  // Subpass 0: GBuffer (escribe en Albedo + Normales)
    { {0, 1}, {2}, {} }  // Subpass 1: Iluminación (lee Albedo + Normales, escribe en Lighting Output)
  };

  renderPassDesc.dependencies = {
    { 0, 1, ACCESS_FLAG::kCOLOR_ATTACHMENT_WRITE, ACCESS_FLAG::kSHADER_READ }
  };

  SPtr<RenderPass> renderPass = GraphicAPI.createRenderPass(renderPassDesc);
  CH_ASSERT(renderPass != nullptr && "Fallo al crear renderPass.");

  SPtr<Framebuffer> framebuffer = GraphicAPI.createFramebuffer({
    .renderPass = renderPass,
    .attachments = { gBufferAlbedo, gBufferNormals, lightingOutput }
  });
  CH_ASSERT(framebuffer != nullptr && "Fallo al crear framebuffer.");

  PipelineStateDesc gBufferPipelineDesc;
  gBufferPipelineDesc.VS = GPUResourceMngr.createShader({
    .byteCode = FileSystem::fastReadFile("gbuffer_vertex.spv"),
    .entryFunc = "main"
  });
  gBufferPipelineDesc.PS = GPUResourceMngr.createShader({
    .byteCode = FileSystem::fastReadFile("gbuffer_pixel.spv"),
    .entryFunc = "main"
  });
  gBufferPipelineDesc.renderPass = renderPass;
  gBufferPipelineDesc.subPassIndex =  0;

  PipelineStateDesc lightingPipeline;
  lightingPipeline.VS = GPUResourceMngr.createShader({
    .byteCode = FileSystem::fastReadFile("lighting_vertex.spv"),
    .entryFunc = "main"
  });
  lightingPipeline.PS = GPUResourceMngr.createShader({
    .byteCode = FileSystem::fastReadFile("lighting_pixel.spv"),
    .entryFunc = "main"
  });
  lightingPipeline.renderPass = renderPass;
  lightingPipeline.subPassIndex = 1;

  m_gBufferPipeline = GraphicAPI.createPipelineState(gBufferPipelineDesc);
  CH_ASSERT(m_gBufferPipeline != nullptr && "Fallo al crear gBufferPipeline.");

  m_lightingPipeline = GraphicAPI.createPipelineState(lightingPipeline);

  m_renderPass = renderPass;
  m_frameBuffer = framebuffer;
  m_albedo = gBufferAlbedo;
  m_normal = gBufferNormals;
  m_lightingOutput = lightingOutput;

  MatrixViewProj MVP;
  m_mvpBuffer = GPUResourceMngr.createBuffer(sizeof(MatrixViewProj));
  m_mvpBuffer->update(sizeof(MatrixViewProj), &MVP);
}

/*
*/
void
Renderer::render() {
  GraphicsModule& GraphicAPI = GraphicsModule::instance();

  if (!m_swapChain->acquireNextFrame()) {
    CH_LOG_ERROR("Failed to acquire next frame.");
    //recreateSwapChain();
    return;
  }

  SPtr<Texture> currentFrame = m_swapChain->getCurrentFrame(m_gBufferPipeline);

  m_commandBuffer->reset(m_gBufferPipeline);
  m_commandBuffer->setPipeLineState(m_gBufferPipeline);

  m_commandBuffer->beginRenderPass(m_renderPass, m_frameBuffer, {
    LinearColor::Black, LinearColor::Black, LinearColor::Black
  });

  m_commandBuffer->setVertexBuffers(0, { m_vertexBuffer } ); // Vector de buffers
  m_commandBuffer->setIndexBuffer(m_indexBuffer);
  m_commandBuffer->setGPUBuffer(m_mvpBuffer, 0);
  m_commandBuffer->drawIndexed(36, 1, 0, 0, 0); //Dar valores default

  m_commandBuffer->nextSubpass();
  
  m_commandBuffer->setPipeLineState(m_lightingPipeline);
  m_commandBuffer->setGPUBuffer(m_mvpBuffer, 0);
  m_commandBuffer->drawIndexed(6, 1, 0, 0, 0); //Quad

  m_commandBuffer->endRenderPass();

  static GPUBarrier presentBarrier;
  presentBarrier.flag = BARRIER_FLAG::kNONE;
  presentBarrier.transition.stateBefore = ResourceStates::kRENDER_TARGET;
  presentBarrier.transition.stateAfter = ResourceStates::kPRESENT;
  m_commandBuffer->resourceBarrierSwapChain(presentBarrier);

  m_commandBuffer->close();
  GraphicAPI.executeCommandBuffers({m_commandBuffer});

  if (!m_commandBuffer->present(1, 0)) {
    CH_LOG_ERROR("Failed to present.");
  }
}
} // namespace chEngineSDK+
