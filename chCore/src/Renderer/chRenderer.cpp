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
void Renderer::initialize()
{
  // Instancias de los módulos
  GraphicsModule &GraphicAPI = GraphicsModule::instance();
  GPUResourceModule &GPUResourceMngr = GPUResourceModule::instance();

  CommandParser& commandParser = CommandParser::getInstance();
  const float width = std::stof(commandParser.getParam("Width", "1280"));
  const float height = std::stof(commandParser.getParam("Height", "720"));
  const float height = std::stof(commandParser.getParam("Height", "720"));

  // Leer el shader en binario (SPIR-V en Vulkan o HLSL en DX12)
  Vector<uint8> shaderFile = FileSystem::fastReadFile({"/home/accelmr/nvme2TB/Programming/Chimera/ChimeraCore/Resources/Shaders/Basic_VS_PS.hlsl"});

  // Crear el shader usando la abstracción
  SPtr<Shader> VS_PS = GPUResourceMngr.createShader({.name = "BasicShader",
                                                     .VS = {
                                                         .byteCode = shaderFile,
                                                         .entryFunc = "VSMain"},
                                                     .PS = {.byteCode = shaderFile, .entryFunc = "PSMain"}});

  // Crear el buffer para MVP
  MatrixViewProj MVP;
  MVPBuffer = GPUResourceMngr.createBuffer(sizeof(MatrixViewProj));
  MVPBuffer->update(sizeof(MatrixViewProj), &MVP);

  // Crear el pipeline usando la abstracción
  viewport = Box2D({0.0f, 0.0f}, { width, height });
  m_pipeline = GraphicAPI.createPipelineState({
    .VS = VS_PS,
    .PS = VS_PS,
    .rasterizerStateDesc = {
      .cullMode = CULL_MODE::kNONE
    },
    .vertexBufferBindingsDesc = {
      {
        VertexBufferBindingDesc(VERTEX_SEMANTIC::kPOSITION,
                                0,
                                offsetof(Vertex, position),
                                FORMAT::kR32G32B32A32_FLOAT)
        //VertexBufferBindingDesc(VERTEX_SEMANTIC::kTEXTCOORD, 1, offsetof(Vertex, texCoord), FORMAT::kR32G32_FLOAT)
      }
    },
    .bindingGroups = {
      .buffers = {
        {
          BufferBindingDesc(BufferBindingDesc::TYPE::kUNIFORM, 
                            SHADER_STAGE::ALL, 
                            0, 
                            sizeof(MatrixViewProj), 
                            MVPBuffer)
        }
      }
    },
    .renderPassDesc = {
      .colorAttachments = { FORMAT::kR8G8B8A8_UNORM },
    },
    .sampleDesc = {
      .count = 1,
      .quality = 0
    },
    .viewport = viewport
  });
  m_commandBuffer = GraphicAPI.beginCommandRecording(COMMAND_BUFFER_TYPES::kDIRECT, m_pipeline);

  // Crear y actualizar el buffer de vértices
  const AABox box(-Vector3::UNIT, Vector3::UNIT);
  Array<Vector4, 8> boxVertices = box.generateVertices4();
  VertexBufferDesc vertexBuffDesc{
      .strideInBytes = sizeof(Vertex),
      .size = static_cast<uint32>(boxVertices.size() * sizeof(Vertex))};
  vertexBuffer = GPUResourceMngr.createVertexBuffer(vertexBuffDesc);
  vertexBuffer->update(vertexBuffDesc, &boxVertices[0]);

  // Crear y actualizar el buffer de índices
  Array<uint16, 36> boxIndices = box.getConstIndices();
  IndexBufferDesc indexBufferDesc = {
      .format = FORMAT::kR16_UINT,
      .size = static_cast<uint32>(boxIndices.size() * sizeof(uint16))};
  indexBuffer = GPUResourceMngr.createIndexBuffer(indexBufferDesc);
  indexBuffer->update(indexBufferDesc, &boxIndices[0]);

  GPUResourceModule &GPUResourceMngr = GPUResourceModule::instance();
  // Configurar las barreras de recursos
 
  barrierStart.transition.stateBefore = chGPUDesc::ResourceStates::kPRESENT;
  barrierStart.transition.stateAfter = chGPUDesc::ResourceStates::kRENDER_TARGET;

  
  barrierEnd.transition.stateBefore = chGPUDesc::ResourceStates::kRENDER_TARGET;
  barrierEnd.transition.stateAfter = chGPUDesc::ResourceStates::kPRESENT;

  GraphicAPI.createFence();
}

/*
 */
void
Renderer::render()
{
  GraphicsModule &GraphicAPI = GraphicsModule::instance();

  m_commandBuffer->reset(m_pipeline);
  m_commandBuffer->setPipeLineState(m_pipeline);
  m_commandBuffer->setSwapChain();
  m_commandBuffer->setScissorRect(viewport);
  m_commandBuffer->setRect(viewport);
  m_commandBuffer->resourceBarrierSwapChain(barrierStart);
  m_commandBuffer->setGPUBuffer(MVPBuffer, 0);
  m_commandBuffer->clearSwapChainTexture(LinearColor::Pink);
  m_commandBuffer->setTopology(PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  m_commandBuffer->setVertexBuffer(0, 1, vertexBuffer);
  m_commandBuffer->setIndexBuffer(indexBuffer);
  m_commandBuffer->drawIndexed(36, 1, 0, 0, 0);
  m_commandBuffer->resourceBarrierSwapChain(barrierEnd);
  m_commandBuffer->close();

  GraphicAPI.executeCommandBuffers({ m_commandBuffer });
  GraphicAPI.present(0, 0);
  GraphicAPI.moveToNextFrame();
  GraphicAPI.waitGPU();
}
} // namespace chEngineSDK