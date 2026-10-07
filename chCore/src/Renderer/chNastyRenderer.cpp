/************************************************************************/
/**
 * @file NastyRenderer.cpp
 * @author AccelMR
 * @date 2025/07/14
 * @brief Implementation of the NastyRenderer for offscreen rendering
 */
/************************************************************************/

#include "chNastyRenderer.h"

#include "chBox.h"
#include "chCamera.h"
#include "chDegree.h"
#include "chEventDispatcherManager.h"
#include "chEventSystem.h"
#include "chLogger.h"
#include "chMatrix4.h"
#include "chMatrixHelpers.h"
#include "chRadian.h"
#include "chVector3.h"

// Graphics-related includes
#include "chIBuffer.h"
#include "chICommandList.h"
#include "chISampler.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chIShader.h"
#include "chITexture.h"
#include "chITextureView.h"

#include "chModel.h"

namespace chEngineSDK {

#if USING(CH_DEBUG_MODE)
#define CH_NASTY_RENDERER_LOG_LEVEL All
#else
#define CH_NASTY_RENDERER_LOG_LEVEL Info
#endif // USING(CH_DEBUG_MODE)

CH_LOG_DECLARE_STATIC(NastyRendererSystem, CH_NASTY_RENDERER_LOG_LEVEL);

namespace {
// Must match CameraData in cube.hlsl.
struct CameraData {
  Matrix4 view;
  Matrix4 projection;
};

// Must match PushConstants in cube.hlsl.
struct DrawPushConstants {
  Matrix4 model;
  uint32 cameraIndex;
  uint32 textureIndex;
  uint32 samplerIndex;
  uint32 padding;
};
static_assert(sizeof(DrawPushConstants) <= GraphicsLimits::PUSH_CONSTANTS_SIZE);

constexpr Format kTextureFormat = Format::R8G8B8A8_UNORM;
constexpr Format kDepthFormat = Format::D32_SFLOAT;
} // namespace

float g_farPlane = 10000.0f;
float g_nearPlane = 0.1f;
Radian g_FOV(Degree(45.0f));
float g_cameraPanSpeed = 0.01f;
float g_cameraMoveSpeed = 0.1f;
float g_rotationSpeed = 0.1f;

Vector3 initialCameraPos(-5.0f, 0.0f, 0.0f);

static Vector<String> NodeNames;
static uint32 NodeIndex = 0;
static bool bIsModelRotating = false;

/*
 */
NastyRenderer::NastyRenderer() { CH_LOG_INFO(NastyRendererSystem, "NastyRenderer created"); }

/*
 */
NastyRenderer::~NastyRenderer() {
  CH_LOG_INFO(NastyRendererSystem, "NastyRenderer destroyed");
  cleanup();
}

/*
 */
void
NastyRenderer::initialize(uint32 width, uint32 height, Format colorFormat)
{
  CH_LOG_INFO(NastyRendererSystem, "Initializing NastyRenderer with dimensions: {0}x{1}",
              width, height);

  CH_ASSERT(IGraphicsAPI::instancePtr() != nullptr);
  m_renderWidth = width;
  m_renderHeight = height;
  m_colorFormat = colorFormat;

  createDepthTarget();
  initializeRenderResources();

  CH_LOG_INFO(NastyRendererSystem, "NastyRenderer initialized successfully");
}

/*
 */
void
NastyRenderer::onRender(ICommandList& commandList,
                        const ITextureView& colorTarget,
                        float deltaTime)
{
  IBuffer& cameraBuffer = *m_cameraBuffers[IGraphicsAPI::instance().getFrameIndex()];
  if (m_camera) {
    const CameraData cameraData{.view = m_camera->getViewMatrix(),
                                .projection = m_camera->getProjectionMatrix()};
    cameraBuffer.update(&cameraData, sizeof(cameraData));
  }

  // The depth target is cleared, so its old contents (and layout) are not needed.
  const Array<TextureBarrier, 1> toRendering = {
      TextureBarrier{.texture = m_depthTarget.get(),
                     .before = ResourceState::Undefined,
                     .after = ResourceState::DepthWrite}};
  commandList.barrier(toRendering);

  RenderingDesc renderingDesc{.colorAttachmentCount = 1,
                              .depthAttachment = {.view = m_depthTargetView.get()},
                              .width = m_renderWidth,
                              .height = m_renderHeight};
  renderingDesc.colorAttachments[0] = {
      .view = &colorTarget,
      .clearColor = m_clearColors.empty() ? LinearColor::Black : m_clearColors[0]};

  commandList.beginRendering(renderingDesc);
  commandList.setViewport(0, 0, static_cast<float>(m_renderWidth),
                          static_cast<float>(m_renderHeight));
  commandList.setScissor(0, 0, m_renderWidth, m_renderHeight);

  if (m_camera && m_currentModel) {
    renderModel(commandList, cameraBuffer.getBindlessIndex(), deltaTime);
  }

  commandList.endRendering();
}

/*
 */
void
NastyRenderer::resize(uint32 width, uint32 height)
{
  CH_LOG_INFO(NastyRendererSystem, "Resizing NastyRenderer to {0}x{1}", width, height);

  m_renderWidth = width;
  m_renderHeight = height;

  // The old depth target goes through the deferred deletion, so the GPU can keep using it
  // until the frames in flight are done.
  createDepthTarget();

  // Update camera viewport
  if (m_camera) {
    m_camera->setViewportSize(static_cast<float>(width), static_cast<float>(height));
    m_camera->updateMatrices();
  }

  CH_LOG_INFO(NastyRendererSystem, "NastyRenderer resized successfully");
}

/*
 */
void
NastyRenderer::cleanup()
{
  CH_LOG_INFO(NastyRendererSystem, "Cleaning up NastyRenderer");

  IGraphicsAPI::instance().waitIdle();

  cleanupModelResources();

  // Reset pipeline resources
  m_pipeline = nullptr;
  m_pipelineCache.clear();
  m_vertexShader.reset();
  m_fragmentShader.reset();
  for (SPtr<IBuffer>& cameraBuffer : m_cameraBuffers) {
    cameraBuffer.reset();
  }

  // Reset render targets
  m_depthTargetView.reset();
  m_depthTarget.reset();

  // Reset material resources
  m_texture.reset();
  m_defaultTexture.reset();
  m_sampler.reset();

  // Reset scene resources
  m_camera.reset();
  m_currentModel.reset();

  CH_LOG_INFO(NastyRendererSystem, "NastyRenderer cleanup completed");
}

/*
 */
void
NastyRenderer::createDepthTarget()
{
  auto& graphicsAPI = IGraphicsAPI::instance();

  TextureCreateInfo depthTextureInfo{.type = TextureType::Texture2D,
                                     .format = kDepthFormat,
                                     .width = m_renderWidth,
                                     .height = m_renderHeight,
                                     .depth = 1,
                                     .mipLevels = 1,
                                     .arrayLayers = 1,
                                     .samples = SampleCount::Count1,
                                     .usage = TextureUsage::DepthStencil};

  m_depthTarget = graphicsAPI.createTexture(depthTextureInfo);

  TextureViewCreateInfo depthViewInfo{.format = kDepthFormat,
                                      .viewType = TextureViewType::View2D};

  m_depthTargetView = m_depthTarget->createView(depthViewInfo);

  CH_LOG_INFO(NastyRendererSystem, "Depth target created: {0}x{1}", m_renderWidth,
              m_renderHeight);
}

/*
 */
void
NastyRenderer::initializeRenderResources() {
  auto& graphicsAPI = IGraphicsAPI::instance();

  // Create camera
  m_camera = chMakeUnique<Camera>(initialCameraPos,
                                  Vector3::ZERO,
                                  static_cast<float>(m_renderWidth),
                                  static_cast<float>(m_renderHeight));
  m_camera->setProjectionType(CameraProjectionType::Perspective);
  m_camera->setFieldOfView(g_FOV);
  m_camera->setClipPlanes(g_nearPlane, g_farPlane);
  m_camera->updateMatrices();

  for (SPtr<IBuffer>& cameraBuffer : m_cameraBuffers) {
    cameraBuffer = graphicsAPI.createBuffer({.size = sizeof(CameraData),
                                             .usage = BufferUsage::UniformBuffer,
                                             .memoryUsage = MemoryUsage::CpuToGpu});
  }

  // Create sampler
  SamplerCreateInfo samplerCreateInfo{.magFilter = SamplerFilter::Linear,
                                      .minFilter = SamplerFilter::Linear,
                                      .mipmapMode = SamplerMipmapMode::Linear,
                                      .addressModeU = SamplerAddressMode::Repeat,
                                      .addressModeV = SamplerAddressMode::Repeat,
                                      .addressModeW = SamplerAddressMode::Repeat,
                                      .anisotropyEnable = false,
                                      .maxAnisotropy = 16.0f};
  m_sampler = graphicsAPI.createSampler(samplerCreateInfo);

  constexpr uint32 kWhitePixel = 0xFFFFFFFF;
  m_defaultTexture = graphicsAPI.createTexture({.format = kTextureFormat,
                                                .initialData = &kWhitePixel,
                                                .initialDataSize = sizeof(kWhitePixel)});

  m_vertexShader = graphicsAPI.loadShader(ShaderStage::Vertex, "cube");
  m_fragmentShader = graphicsAPI.loadShader(ShaderStage::Fragment, "cube");

  GraphicsPipelineDesc pipelineDesc{.vertexShader = m_vertexShader,
                                    .fragmentShader = m_fragmentShader,
                                    .vertexLayout = VertexNormalTexCoord::getLayout(),
                                    .colorAttachmentCount = 1,
                                    .depthFormat = kDepthFormat};
  pipelineDesc.colorFormats[0] = m_colorFormat;
  m_pipeline = m_pipelineCache.getOrCreate(pipelineDesc).get();

  CH_LOG_INFO(NastyRendererSystem, "Render resources initialized");
}

/*
 */
void
NastyRenderer::setTexture(const SPtr<ITexture>& texture) {
  m_texture = texture;
}

/*
 */
void
NastyRenderer::loadModel(const SPtr<Model>& model) {

  if (!model) {
    CH_LOG_ERROR(NastyRendererSystem, "Cannot load null model");
    cleanupModelResources();
    m_currentModel.reset();
    NodeNames.clear();
    NodeIndex = 0;
    CH_LOG_INFO(NastyRendererSystem, "Model unloaded successfully");
    return;
  }

  cleanupModelResources();
  m_currentModel = model;

  createMeshBuffers();

  CH_LOG_INFO(NastyRendererSystem, "Model loaded successfully");
}

/*
 */
void
NastyRenderer::createMeshBuffers() {
  auto& graphicsAPI = IGraphicsAPI::instance();

  if (!m_currentModel) {
    return;
  }

  Vector<SPtr<Mesh>> uniqueMeshes;
  UnorderedMap<SPtr<Mesh>, uint32> meshToIndexMap;

  // Collect all unique meshes from the model
  for (ModelNode* node : m_currentModel->getAllNodes()) {
    NodeNames.push_back(node->getName());
    for (const auto& mesh : node->getMeshes()) {
      // If this mesh is not already in our list
      if (meshToIndexMap.find(mesh) == meshToIndexMap.end()) {
        meshToIndexMap[mesh] = static_cast<uint32>(uniqueMeshes.size());
        uniqueMeshes.push_back(mesh);
      }
    }
  }

  // Resize our buffer arrays
  m_meshVertexBuffers.resize(uniqueMeshes.size());
  m_meshIndexBuffers.resize(uniqueMeshes.size());
  m_meshIndexCounts.resize(uniqueMeshes.size());
  m_meshIndexTypes.resize(uniqueMeshes.size());

  // Create vertex and index buffers for each unique mesh
  for (SIZE_T i = 0; i < uniqueMeshes.size(); ++i) {
    const auto& mesh = uniqueMeshes[i];

    // Create vertex buffer
    const Vector<uint8>& vertexData = mesh->getVertexData();
    const uint32 vertexDataSize = static_cast<uint32>(mesh->getVertexDataSize());

    BufferCreateInfo vertexBufferCreateInfo{
        .size = vertexDataSize,
        .usage = BufferUsage::VertexBuffer,
        .memoryUsage = MemoryUsage::GpuOnly,
        .initialData = const_cast<void*>(static_cast<const void*>(vertexData.data())),
        .initialDataSize = vertexDataSize,
    };
    m_meshVertexBuffers[i] = graphicsAPI.createBuffer(vertexBufferCreateInfo);

    // Store index information
    m_meshIndexTypes[i] = mesh->getIndexType();
    m_meshIndexCounts[i] = mesh->getIndexCount();

    // Create index buffer based on index type
    if (m_meshIndexTypes[i] == IndexType::UInt16) {
      Vector<uint16> indexData = mesh->getIndicesAsUInt16();
      const uint32 indexDataSize = static_cast<uint32>( mesh->getIndexDataSize());

      BufferCreateInfo indexBufferCreateInfo{
          .size = indexDataSize,
          .usage = BufferUsage::IndexBuffer,
          .memoryUsage = MemoryUsage::GpuOnly,
          .initialData = const_cast<void*>(static_cast<const void*>(indexData.data())),
          .initialDataSize = indexDataSize,
      };
      m_meshIndexBuffers[i] = graphicsAPI.createBuffer(indexBufferCreateInfo);
    }
    else {
      Vector<uint32> indexData = mesh->getIndicesAsUInt32();
      const uint32 indexDataSize = static_cast<uint32>(mesh->getIndexDataSize());

      BufferCreateInfo indexBufferCreateInfo{
          .size = indexDataSize,
          .usage = BufferUsage::IndexBuffer,
          .memoryUsage = MemoryUsage::GpuOnly,
          .initialData = const_cast<void*>(static_cast<const void*>(indexData.data())),
          .initialDataSize = indexDataSize,
      };
      m_meshIndexBuffers[i] = graphicsAPI.createBuffer(indexBufferCreateInfo);
    }
  }

  // Store the mesh to index mapping
  m_meshToIndexMap = std::move(meshToIndexMap);

  CH_LOG_INFO(NastyRendererSystem, "Created mesh buffers for {0} unique meshes",
              uniqueMeshes.size());
}

/*
 */
void
NastyRenderer::bindInputEvents() {
  EventDispatcherManager& eventDispatcher = EventDispatcherManager::instance();

  listenKeyDown = eventDispatcher.OnKeyDown.connect([&](const KeyBoardData& keydata) {
    if (!m_bIsfocused) {
      return;
    }
    if (keydata.key == Key::P && m_camera) {
      Vector3 cameraPosition = m_camera->getPosition();
      CH_LOG_INFO(NastyRendererSystem, "Camera Position: ({0}, {1}, {2})", cameraPosition.x,
                  cameraPosition.y, cameraPosition.z);
      return;
    }

    if (keydata.key == Key::Num1) {
      NodeIndex = (NodeIndex + 1) % NodeNames.size();
      CH_LOG_INFO(NastyRendererSystem, "Node Rotating: {0}",
                  NodeNames.empty() ? "None" : NodeNames[NodeIndex]);
    }

    if (keydata.key == Key::Num2) {
      if (!NodeNames.empty()) {
        NodeIndex = (NodeIndex - 1) % NodeNames.size();
        CH_LOG_INFO(NastyRendererSystem, "Node Rotating: {0}", NodeNames[NodeIndex]);
      }
    }

    if (keydata.key == Key::Num3) {
      bIsModelRotating = !bIsModelRotating;
      CH_LOG_INFO(NastyRendererSystem, "Model rotation {0}",
                  bIsModelRotating ? "enabled" : "disabled");
    }
  });

  listenKeys = eventDispatcher.OnKeyDown.connect([&](const KeyBoardData& keydata) {
    if (!m_camera || !m_bIsfocused) {
      return;
    }

    float moveSpeed = g_cameraMoveSpeed * 0.1f;
    switch (keydata.key) {
    case Key::W:
      m_camera->moveForward(std::move(moveSpeed));
      break;
    case Key::S:
      m_camera->moveForward(std::move(-moveSpeed));
      break;
    case Key::A:
      m_camera->moveRight(std::move(-moveSpeed));
      break;
    case Key::D:
      m_camera->moveRight(std::move(moveSpeed));
      break;
    case Key::Q:
      m_camera->moveUp(std::move(moveSpeed));
      break;
    case Key::E:
      m_camera->moveUp(std::move(-moveSpeed));
      break;
    case Key::R:
      CH_LOG_INFO(NastyRendererSystem, "Resetting camera position to pos {0}, {1}, {2}",
                  initialCameraPos.x, initialCameraPos.y, initialCameraPos.z);
      m_camera->setPosition(std::move(initialCameraPos));
      m_camera->lookAt(Vector3::ZERO);
      break;
    case Key::P:
      CH_LOG_INFO(NastyRendererSystem, "Camera Position: ({0}, {1}, {2})",
                  m_camera->getPosition().x, m_camera->getPosition().y,
                  m_camera->getPosition().z);
      break;
    default:
      return;
    }
  });

  listenWheel = eventDispatcher.OnMouseWheel.connect([&](const MouseWheelData& wheelData) {
    if (m_camera && wheelData.deltaY != 0 && m_bIsfocused) {
      m_camera->moveForward(std::move(wheelData.deltaY * g_cameraMoveSpeed));
    }
  });

  listenMouse = eventDispatcher.OnMouseMove.connect([&](const MouseMoveData& mouseData) {
    if (!m_camera || !m_bIsfocused) {
      return;
    }

    const bool isMouseButtonDown = eventDispatcher.isMouseButtonDown(MouseButton::Right);
    const bool isMouseButtonDownMiddle =
        eventDispatcher.isMouseButtonDown(MouseButton::Middle);
    if (!isMouseButtonDown && !isMouseButtonDownMiddle) {
      return;
    }

    if (mouseData.deltaX != 0 || mouseData.deltaY != 0) {
      if (isMouseButtonDownMiddle) {
        m_camera->pan(std::move(-mouseData.deltaX * g_cameraPanSpeed),
                      std::move(-mouseData.deltaY * g_cameraPanSpeed));
      }
      if (isMouseButtonDown) {
        m_camera->rotate(std::move(mouseData.deltaY * g_rotationSpeed),
                         std::move(mouseData.deltaX * g_rotationSpeed), 0.0f);
      }
    }
  });

  CH_LOG_INFO(NastyRendererSystem, "Input events bound");
}

/*
 */
void
NastyRenderer::renderModel(ICommandList& commandList, uint32 cameraIndex, float deltaTime)
{
  if (!m_currentModel) {
    return;
  }

  m_currentModel->updateTransforms();

  if (bIsModelRotating && !NodeNames.empty()) {
    if (ModelNode* targetNode = m_currentModel->findNode(NodeNames[NodeIndex])) {
      const Matrix4 originalTransform = targetNode->getLocalTransform();
      // Rotating before the node's own transform spins it in place, around its own up.
      RotationMatrix rotationMatrix(Rotator(0.0f, deltaTime * 20, 0.0f));
      const Matrix4 newTransform = rotationMatrix * originalTransform;
      m_currentModel->updateNodeTransform(targetNode, newTransform);
    }
  }

  commandList.bindPipeline(*m_pipeline);

  const ITexture& texture = m_texture ? *m_texture : *m_defaultTexture;
  DrawPushConstants pushConstants{.model = Matrix4::IDENTITY,
                                  .cameraIndex = cameraIndex,
                                  .textureIndex = texture.getBindlessIndex(),
                                  .samplerIndex = m_sampler->getBindlessIndex(),
                                  .padding = 0};

  for (ModelNode* node : m_currentModel->getAllNodes()) {
    if (node->getMeshes().empty()) {
      continue;
    }

    pushConstants.model = node->getGlobalTransform();
    commandList.pushConstants(&pushConstants, sizeof(pushConstants));

    for (const auto& mesh : node->getMeshes()) {
      const auto it = m_meshToIndexMap.find(mesh);
      if (it == m_meshToIndexMap.end()) {
        continue;
      }
      const uint32 meshIndex = it->second;

      commandList.bindVertexBuffer(*m_meshVertexBuffers[meshIndex]);
      commandList.bindIndexBuffer(*m_meshIndexBuffers[meshIndex],
                                  m_meshIndexTypes[meshIndex]);
      commandList.drawIndexed(m_meshIndexCounts[meshIndex]);
    }
  }
}

/*
 */
void
NastyRenderer::cleanupModelResources() {
  // Clear mesh buffers
  m_meshVertexBuffers.clear();
  m_meshIndexBuffers.clear();
  m_meshIndexCounts.clear();
  m_meshIndexTypes.clear();
  m_meshToIndexMap.clear();

  // Clear node names
  NodeNames.clear();
  NodeIndex = 0;

  // Reset current model
  m_currentModel.reset();

  CH_LOG_INFO(NastyRendererSystem, "Model resources cleaned up");
}

} // namespace chEngineSDK
