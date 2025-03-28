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
#include "chVector3.h"

namespace chEngineSDK
{
const Path m_shaderPath = "resources/shaders/";


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

  MatrixViewProj MVP;
  m_mvpBuffer = GPUResourceMngr.createBuffer(sizeof(MatrixViewProj));
  m_mvpBuffer->update(sizeof(MatrixViewProj), &MVP);

  m_albedo = GPUResourceMngr.createTexture({
    .dimensions = { static_cast<int32>(width), static_cast<int32>(height), 1},
    .usage = TEXTURE_USAGE::kRENDER_TARGET | TEXTURE_USAGE::kSAMPLED | TEXTURE_USAGE::kINPUT_ATTACHMENT,
    .format = FORMAT::kB8G8R8A8_UNORM
  });
  CH_ASSERT(m_albedo != nullptr && "Fallo al crear gBufferAlbedo.");

  m_normal = GPUResourceMngr.createTexture({
    .dimensions = {static_cast<int32>(width), static_cast<int32>(height), 1},
    .usage = TEXTURE_USAGE::kRENDER_TARGET | TEXTURE_USAGE::kSAMPLED | TEXTURE_USAGE::kINPUT_ATTACHMENT,
    .format = FORMAT::kR16G16B16A16_FLOAT
  });
  CH_ASSERT(m_normal != nullptr && "Fallo al crear gBufferNormals.");

  m_lightingOutput = GPUResourceMngr.createTexture({
    .dimensions = {static_cast<int32>(width), static_cast<int32>(height), 1},
    .usage = TEXTURE_USAGE::kRENDER_TARGET | TEXTURE_USAGE::kSAMPLED,
    .format = FORMAT::kB8G8R8A8_UNORM
  });
  CH_ASSERT(m_lightingOutput != nullptr && "Fallo al crear lightingOutput.");

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
    .loadOp = AttachmentDesc::LOAD_OP::kCLEAR,
    .storeOp = AttachmentDesc::STORE_OP::kSTORE,
    .initialLayout = LAYOUT::kUNDEFINED,
    .finalLayout = LAYOUT::kPRESENT // For Lighting Output
  }};

  renderPassDesc.subpasses = {
    // First subpass: GBuffer (writes to Albedo + Normals)
    {
      .inputAttachments = {},
      .colorAttachments = {
        {0, LAYOUT::kCOLOR_ATTACHMENT},  // Albedo
        {1, LAYOUT::kCOLOR_ATTACHMENT}   // Normals
      }
    },
    // Second subpass: Lighting (reads Albedo + Normals, writes to Lighting Output)
    {
      .inputAttachments = {
        {0, LAYOUT::kSHADER_READ_ONLY},  // Albedo como input
        {1, LAYOUT::kSHADER_READ_ONLY}   // Normals como input
      },
      .colorAttachments = {
        {2, LAYOUT::kCOLOR_ATTACHMENT}   // Lighting output
      }
    }
  };

  renderPassDesc.dependencies = {
    { 0, 1, ACCESS_FLAG::kCOLOR_ATTACHMENT_WRITE, ACCESS_FLAG::kSHADER_READ }
  };

  SPtr<RenderPass> renderPass = GraphicAPI.createRenderPass(renderPassDesc);
  CH_ASSERT(renderPass != nullptr && "Fallo al crear renderPass.");

  SPtr<Framebuffer> framebuffer = GraphicAPI.createFramebuffer({
    .renderPass = renderPass,
    .attachments = { m_albedo, m_normal, m_lightingOutput }
  });
  CH_ASSERT(framebuffer != nullptr && "Fallo al crear framebuffer.");

  SamplerDesc samplerDesc;
  samplerDesc.minFilter = FILTER::kLINEAR;
  samplerDesc.magFilter = FILTER::kLINEAR;
  samplerDesc.addressU = TEXTURE_ADDRESS_MODE::kCLAMP;
  samplerDesc.addressV = TEXTURE_ADDRESS_MODE::kCLAMP;
  samplerDesc.addressW = TEXTURE_ADDRESS_MODE::kCLAMP;
  m_sampler = GPUResourceMngr.createSampler(samplerDesc);

  Vector<BindingGroup> gBufferBindings = {
    BindingGroup(0, {
        DescriptorBinding(
            DescriptorBinding::TYPE::kUNIFORM_BUFFER,
            SHADER_STAGE::kVERTEX,
            0,  // slot
            m_mvpBuffer
        )
    })
  };

  // Bindings para Lighting pipeline (MVP + texturas)
  Vector<BindingGroup> lightingBindings = {
      BindingGroup(0, {DescriptorBinding( // MVP Buffer
                        DescriptorBinding::TYPE::kUNIFORM_BUFFER,
                        SHADER_STAGE::kVERTEX | SHADER_STAGE::kPIXEL,
                        0,
                        m_mvpBuffer),
                       DescriptorBinding( // Albedo texture
                        DescriptorBinding::TYPE::kINPUT_ATTACHMENT,
                        SHADER_STAGE::kPIXEL,
                        1,
                        m_albedo),
                       DescriptorBinding(// Normal texture
                        DescriptorBinding::TYPE::kINPUT_ATTACHMENT,
                        SHADER_STAGE::kPIXEL,
                        2,
                        m_normal),
                       DescriptorBinding( // Sampler
                        DescriptorBinding::TYPE::kSAMPLER,
                        SHADER_STAGE::kPIXEL,
                        3,
                        m_sampler)})};

  PipelineStateDesc gBufferPipelineDesc;
  gBufferPipelineDesc.VS = GPUResourceMngr.createShader({
    .byteCode = FileSystem::fastReadFile(m_shaderPath + "gbuffer_vertex.spv"),
    .entryFunc = "main"
  });
  gBufferPipelineDesc.PS = GPUResourceMngr.createShader({
    .byteCode = FileSystem::fastReadFile(m_shaderPath + "gbuffer_pixel.spv"),
    .entryFunc = "main"
  });
  gBufferPipelineDesc.renderPass = renderPass;
  gBufferPipelineDesc.subPassIndex =  0;
  gBufferPipelineDesc.bindingGroups = gBufferBindings;
  gBufferPipelineDesc.viewports = { { {0, 0}, {width, height} } };
  gBufferPipelineDesc.scissorRects = { { {0, 0}, {width, height} } };
  gBufferPipelineDesc.vertexBufferBindingsDesc = {
    VertexBufferBindingDesc(VERTEX_SEMANTIC::kPOSITION, 
    0, 
    sizeof(Vertex), 
    FORMAT::kR32G32B32A32_FLOAT)
  };

  BlendStateDesc gBufferBlendState;
  // Set up for albedo
  gBufferBlendState.renderTargetBlendDesc[0] = {
      .srcBlend = BLEND::kBLEND_ONE,
      .destBlend = BLEND::kBLEND_ZERO,
      .blendOP = BLEND_OP::kBLEND_OP_ADD,
      .srcsBlendAlpha = BLEND::kBLEND_ONE,
      .destBlendAlpha = BLEND::kBLEND_ZERO,
      .blenOpAlpha = BLEND_OP::kBLEND_OP_ADD,
      .renderTargetWritemask = COLOR_WRITE_ENABLE::kALL};
  // Set up for normals
  gBufferBlendState.renderTargetBlendDesc[1] = gBufferBlendState.renderTargetBlendDesc[0];
  gBufferPipelineDesc.blendState = gBufferBlendState;

  PipelineStateDesc lightingPipelineDesc;
  lightingPipelineDesc.VS = GPUResourceMngr.createShader({
    .byteCode = FileSystem::fastReadFile(m_shaderPath + "lighting_vertex.spv"),
    .entryFunc = "main"
  });
  lightingPipelineDesc.PS = GPUResourceMngr.createShader({
    .byteCode = FileSystem::fastReadFile(m_shaderPath + "lighting_pixel.spv"),
    .entryFunc = "main"
  });
  lightingPipelineDesc.renderPass = renderPass;
  lightingPipelineDesc.subPassIndex = 1;
  lightingPipelineDesc.bindingGroups = lightingBindings;
  lightingPipelineDesc.viewports = { { {0, 0}, {width, height} } };
  lightingPipelineDesc.scissorRects = { { {0, 0}, {width, height} } };
  lightingPipelineDesc.vertexBufferBindingsDesc = {
      VertexBufferBindingDesc(
          VERTEX_SEMANTIC::kPOSITION,
          0,
          sizeof(Vector4),
          FORMAT::kR32G32B32A32_FLOAT)};
  BlendStateDesc blendStateDesc;
  blendStateDesc.renderTargetBlendDesc[0] = {
      .srcBlend = BLEND::kBLEND_ONE,
      .destBlend = BLEND::kBLEND_ZERO,
      .blendOP = BLEND_OP::kBLEND_OP_ADD,
      .srcsBlendAlpha = BLEND::kBLEND_ONE,
      .destBlendAlpha = BLEND::kBLEND_ZERO,
      .blenOpAlpha = BLEND_OP::kBLEND_OP_ADD,
      .renderTargetWritemask = COLOR_WRITE_ENABLE::kALL};
  lightingPipelineDesc.blendState = blendStateDesc;
  lightingPipelineDesc.subPassIndex = 1;

  m_gBufferPipeline = GraphicAPI.createPipelineState(gBufferPipelineDesc);
  CH_ASSERT(m_gBufferPipeline != nullptr && "Fallo al crear gBufferPipeline.");

  m_lightingPipeline = GraphicAPI.createPipelineState(lightingPipelineDesc);
  CH_ASSERT(m_lightingPipeline != nullptr && "Fallo al crear lightingPipeline.");

  m_renderPass = renderPass;
  m_frameBuffer = framebuffer;

  AABox box(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));
  Array<Vector4, 8> vertices = box.generateVertices4();

  chGPUDesc::VertexBufferDesc vertexBufferDesc;
  vertexBufferDesc.strideInBytes = sizeof(Vector4);
  vertexBufferDesc.size = sizeof(Vector4) * 8;

  m_vertexBuffer = GPUResourceMngr.createVertexBuffer(vertexBufferDesc);
  m_vertexBuffer->update(vertexBufferDesc, vertices.data());

  Array<uint16, 36> indices = box.getConstIndices();
  chGPUDesc::IndexBufferDesc indexBufferDesc;
  indexBufferDesc.format = FORMAT::kR16_UINT;
  indexBufferDesc.size = sizeof(uint16) * 36;

  m_indexBuffer = GPUResourceMngr.createIndexBuffer(indexBufferDesc);
  m_indexBuffer->update(indexBufferDesc, indices.data());

  m_commandBuffer = GraphicAPI.createCommandBuffer(COMMAND_BUFFER_TYPES::kDIRECT);

  Vector<GPUBarrier> barriers;

  GPUBarrier albedoBarrier;
  albedoBarrier.flag = BARRIER_FLAG::kNONE;
  albedoBarrier.transition.resource = m_albedo;
  albedoBarrier.transition.subresource = 0;
  albedoBarrier.transition.stateBefore = ResourceStates::kPRESENT;
  albedoBarrier.transition.stateAfter = ResourceStates::kRENDER_TARGET;
  barriers.push_back(albedoBarrier);

  GPUBarrier normalBarrier;
  normalBarrier.flag = BARRIER_FLAG::kNONE;
  normalBarrier.transition.resource = m_normal;
  normalBarrier.transition.subresource = 0;
  normalBarrier.transition.stateBefore = ResourceStates::kPRESENT;
  normalBarrier.transition.stateAfter = ResourceStates::kRENDER_TARGET;
  barriers.push_back(normalBarrier);

  GPUBarrier outputBarrier;
  outputBarrier.flag = BARRIER_FLAG::kNONE;
  outputBarrier.transition.resource = m_lightingOutput;
  outputBarrier.transition.subresource = 0;
  outputBarrier.transition.stateBefore = ResourceStates::kPRESENT;
  outputBarrier.transition.stateAfter = ResourceStates::kRENDER_TARGET;
  barriers.push_back(outputBarrier);

  auto tempCmdBuffer = GraphicAPI.createCommandBuffer(COMMAND_BUFFER_TYPES::kDIRECT);
  auto fence = GraphicAPI.createFence();
  tempCmdBuffer->reset(nullptr);
  tempCmdBuffer->resourceBarrier(barriers);
  tempCmdBuffer->close();
  GraphicAPI.executeCommandBuffers({tempCmdBuffer});

  fence->wait(10);
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
