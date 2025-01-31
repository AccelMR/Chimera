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
  LookAtMatrix View = LookAtMatrix({0.0f, -20.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, Vector3::UP);
  PerspectiveMatrix Proj = PerspectiveMatrix(Radian(0.7853982f), 1920, 1080, 0.1f, 100.0f);
};

struct Vertex
{
  Vector4 position;
};

/*
 */
using namespace chGPUDesc;

SPtr<IndexBuffer> indexBuffer;
SPtr<VertexBuffer> vertexBuffer;
SPtr<GPUBuffer> MVPBuffer;
Box2D viewport;
GPUBarrier barrierStart;
GPUBarrier barrierEnd;

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

  Vector<uint8> shaderFile = FileSystem::fastReadFile({"resources/shaders/vertex.spv"});

  SPtr<Shader> vertexShader = GPUResourceMngr.createShader({
    .name = "vertex.vert",
    .byteCode = shaderFile,
    .entryFunc = "main"
  });

  MatrixViewProj MVP;
  MVPBuffer = GPUResourceMngr.createBuffer(sizeof(MatrixViewProj));
  MVPBuffer->update(sizeof(MatrixViewProj), &MVP);

  viewport = Box2D({0.0f, 0.0f}, {width, height});

  PipelineStateDesc pipelineDesc;
  pipelineDesc.VS = vertexShader;
  pipelineDesc.rasterizerStateDesc.cullMode = CULL_MODE::kNONE;
  pipelineDesc.vertexBufferBindingsDesc = {
    {VERTEX_SEMANTIC::kPOSITION, 0, sizeof(Vertex) , FORMAT::kR32G32B32A32_FLOAT}
  };
  pipelineDesc.bindingGroups = { BindingGroup(0, { 
    DescriptorBinding(DescriptorBinding::TYPE::kUNIFORM_BUFFER, SHADER_STAGE::kVERTEX, 0, MVPBuffer) 
    }) 
  };
  pipelineDesc.renderPassDesc.attachments = {
    { FORMAT::kB8G8R8A8_UNORM, SAMPLE_COUNT::kSAMPLE_COUNT_1,
      AttachmentDesc::LOAD_OP::kCLEAR, AttachmentDesc::STORE_OP::kSTORE }
  };
  pipelineDesc.renderPassDesc.subpasses = { { {}, {0}, {} } };
  pipelineDesc.sampleDesc = { SAMPLE_COUNT::kSAMPLE_COUNT_1 };
  pipelineDesc.viewports = { viewport };

  m_pipeline = GraphicAPI.createPipelineState(pipelineDesc);

  m_commandBuffer = GraphicAPI.beginCommandRecording(COMMAND_BUFFER_TYPES::kDIRECT, m_pipeline);

  const AABox box(-Vector3::UNIT, Vector3::UNIT);
  Array<Vector4, 8> boxVertices = box.generateVertices4();
  VertexBufferDesc vertexBuffDesc{
    .strideInBytes = sizeof(Vertex),
    .size = static_cast<uint32>(boxVertices.size() * sizeof(Vertex))
  };
  vertexBuffer = GPUResourceMngr.createVertexBuffer(vertexBuffDesc);
  vertexBuffer->update(vertexBuffDesc, &boxVertices[0]);

  Array<uint16, 36> boxIndices = box.getConstIndices();
  IndexBufferDesc indexBufferDesc = {
    .format = FORMAT::kR16_UINT,
    .size = static_cast<uint32>(boxIndices.size() * sizeof(uint16))
  };
  indexBuffer = GPUResourceMngr.createIndexBuffer(indexBufferDesc);
  indexBuffer->update(indexBufferDesc, &boxIndices[0]);

  barrierStart.transition.stateBefore = chGPUDesc::ResourceStates::kPRESENT;
  barrierStart.transition.stateAfter = chGPUDesc::ResourceStates::kRENDER_TARGET;

  barrierEnd.transition.stateBefore = chGPUDesc::ResourceStates::kRENDER_TARGET;
  barrierEnd.transition.stateAfter = chGPUDesc::ResourceStates::kPRESENT;

  m_swapChain = GraphicAPI.getSwapChain();
}


/*
*/
void
Renderer::render()
{
  GraphicsModule& GraphicAPI = GraphicsModule::instance();

  if (!m_swapChain->acquireNextFrame()) {
    CH_LOG_ERROR("Failed to acquire next frame.");
    //recreateSwapChain();
  }

  SPtr<Texture> currentFrame = m_swapChain->getCurrentFrame(m_pipeline);

  m_commandBuffer->reset(m_pipeline);
  m_commandBuffer->setPipeLineState(m_pipeline);
  m_commandBuffer->resourceBarrierSwapChain(barrierStart);
  m_commandBuffer->setRenderTarget(currentFrame);
  m_commandBuffer->setScissorRect(viewport);
  m_commandBuffer->setRect(viewport);


  m_commandBuffer->setGPUBuffer(MVPBuffer, 0);
  m_commandBuffer->clearRenderTarget(currentFrame, LinearColor::Magenta);
  m_commandBuffer->setTopology(PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  m_commandBuffer->setVertexBuffer(0, 1, vertexBuffer);
  m_commandBuffer->setIndexBuffer(indexBuffer);
  m_commandBuffer->drawIndexed(36, 1, 0, 0, 0);
  m_commandBuffer->resourceBarrierSwapChain(barrierEnd);
  m_commandBuffer->close(); 

  GraphicAPI.executeCommandBuffers({ m_commandBuffer });

  if (!m_commandBuffer->present(1, 0)){
    CH_LOG_ERROR("Failed to present.");
  }
}
} // namespace chEngineSDK+
