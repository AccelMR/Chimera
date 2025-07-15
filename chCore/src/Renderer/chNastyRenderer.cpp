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
#include "chFileSystem.h"
#include "chLogger.h"
#include "chMatrix4.h"
#include "chMatrixHelpers.h"
#include "chRadian.h"
#include "chVector3.h"

// Graphics-related includes
#include "chIBuffer.h"
#include "chICommandBuffer.h"
#include "chICommandPool.h"
#include "chICommandQueue.h"
#include "chIDescriptorPool.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chIShader.h"
#include "chISynchronization.h"
#include "chITexture.h"
#include "chITextureView.h"

#include "chModel.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace chEngineSDK {

#if USING(CH_DEBUG_MODE)
#define CH_NASTY_RENDERER_LOG_LEVEL All
#else
#define CH_NASTY_RENDERER_LOG_LEVEL Info
#endif // USING(CH_DEBUG_MODE)

CH_LOG_DECLARE_STATIC(NastyRendererSystem, CH_NASTY_RENDERER_LOG_LEVEL);

namespace RendererHelpers {
struct ProjectionViewMatrix {
  Matrix4 projectionMatrix;
  Matrix4 viewMatrix;
  Matrix4 modelMatrix;
};

Vector<uint8>
loadImage(const Path& path, int32* width, int32* height, int32* channels) {
  CH_ASSERT(FileSystem::isFile(path));

  Vector<uint8> imageData;
  uint8* data = stbi_load(path.toString().c_str(), width, height, channels, STBI_rgb_alpha);
  if (data) {
    imageData.resize((*width) * (*height) * (static_cast<SIZE_T>(STBI_rgb_alpha)));
    memcpy(imageData.data(), data, imageData.size());
    stbi_image_free(data);
  }
  return imageData;
}
} // namespace RendererHelpers

// Global variables from original renderer
RendererHelpers::ProjectionViewMatrix projectionViewMatrix;
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
static Array<Path, 5> ModelPaths = {
    Path("resources/models/rex_norm.obj"), Path("resources/models/cyberdemon.md5mesh"),
    Path("resources/models/Porch.fbx"), Path("resources/models/test.fbx"),
    Path("resources/models/Porce/scene.gltf")};
static uint32 ModelIndex = 0;

static constexpr uint64 MAX_WAIT_TIME = 100000000; // 1 second in nanoseconds

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
NastyRenderer::initialize(uint32 width, uint32 height) {
  CH_LOG_INFO(NastyRendererSystem, "Initializing NastyRenderer with dimensions: {0}x{1}",
              width, height);

  CH_ASSERT(IGraphicsAPI::instancePtr() != nullptr);

  m_renderWidth = width;
  m_renderHeight = height;

  createRenderTargets();
  createRenderPass();
  initializeRenderResources();

  CH_LOG_INFO(NastyRendererSystem, "NastyRenderer initialized successfully");
}

/*
 */
RendererOutput
NastyRenderer::onRender(float deltaTime) {
  auto& graphicsAPI = IGraphicsAPI::instance();

  // Wait for previous frame
  if (!m_renderFence->wait(MAX_WAIT_TIME)) {
    CH_LOG_WARNING(NastyRendererSystem, "Render fence timeout");
    return {}; // Return invalid output
  }
  m_renderFence->reset();

  // Begin command buffer recording
  m_commandBuffer->begin();

  // Begin render pass
  RenderPassBeginInfo renderPassInfo{.renderPass = m_renderPass,
                                     .framebuffer = m_framebuffer,
                                     .clearValues = {LinearColor::Black},
                                     .depthStencilClearValue = {{1.0f, 0}}};

  m_commandBuffer->beginRenderPass(renderPassInfo);
  m_commandBuffer->setViewport(0, 0, m_renderWidth, m_renderHeight);
  m_commandBuffer->setScissor(0, 0, m_renderWidth, m_renderHeight);
  m_commandBuffer->bindPipeline(m_pipeline);

  // Update camera matrices
  if (m_camera) {
    projectionViewMatrix.viewMatrix = m_camera->getViewMatrix();
    projectionViewMatrix.projectionMatrix = m_camera->getProjectionMatrix();

    // Render the model
    if (m_currentModel) {
      renderModel(m_commandBuffer, deltaTime);
    }
  }

  m_commandBuffer->endRenderPass();
  m_commandBuffer->end();

  // Submit command buffer
  SubmitInfo submitInfo{.commandBuffers = {m_commandBuffer},
                        .waitSemaphores = {},
                        .waitStages = {},
                        .signalSemaphores = {}};

  graphicsAPI.getQueue(QueueType::Graphics)->submit(submitInfo, m_renderFence);

  // Return output
  RendererOutput output;
  output.colorTarget = m_colorTargetView;
  output.depthTarget = m_depthTargetView;
  output.width = m_renderWidth;
  output.height = m_renderHeight;
  output.isValid = true;

  return output;
}

/*
 */
void
NastyRenderer::resize(uint32 width, uint32 height) {
  CH_LOG_INFO(NastyRendererSystem, "Resizing NastyRenderer to {0}x{1}", width, height);

  auto& graphicsAPI = IGraphicsAPI::instance();
  graphicsAPI.waitIdle();

  if (m_renderFence) {
    m_renderFence->wait(MAX_WAIT_TIME);
  }

  m_renderWidth = width;
  m_renderHeight = height;

  // Recreate render targets with new dimensions
  createRenderTargets();
  createRenderPass();

  // Recreate framebuffer
  FrameBufferCreateInfo fbInfo{.renderPass = m_renderPass,
                               .attachments = {m_colorTargetView, m_depthTargetView},
                               .width = m_renderWidth,
                               .height = m_renderHeight,
                               .layers = 1};
  m_framebuffer = graphicsAPI.createFrameBuffer(fbInfo);

  // Update camera viewport
  if (m_camera) {
    m_camera->setViewportSize(width, height);
    m_camera->updateMatrices();
  }

  CH_LOG_INFO(NastyRendererSystem, "NastyRenderer resized successfully");
}

/*
 */
void
NastyRenderer::cleanup() {
  CH_LOG_INFO(NastyRendererSystem, "Cleaning up NastyRenderer");

  auto& graphicsAPI = IGraphicsAPI::instance();
  graphicsAPI.waitIdle();

  if (m_renderFence) {
    m_renderFence->wait();
  }

  cleanupModelResources();

  // Reset command resources
  m_commandBuffer.reset();
  m_commandPool.reset();
  m_renderFence.reset();

  // Reset pipeline resources
  m_pipeline.reset();
  m_vertexShader.reset();
  m_fragmentShader.reset();

  // Reset render targets
  m_framebuffer.reset();
  m_renderPass.reset();
  m_colorTargetView.reset();
  m_colorTarget.reset();
  m_depthTargetView.reset();
  m_depthTarget.reset();

  // Reset material resources
  m_textureView.reset();
  m_texture.reset();
  m_sampler.reset();
  m_descriptorSetLayout.reset();
  m_descriptorPool.reset();

  // Reset scene resources
  m_camera.reset();
  m_currentModel.reset();

  CH_LOG_INFO(NastyRendererSystem, "NastyRenderer cleanup completed");
}

/*
 */
void
NastyRenderer::createRenderTargets() {
  auto& graphicsAPI = IGraphicsAPI::instance();

  // Create color target (RGBA8 for now, can be upgraded to HDR later)
  TextureCreateInfo colorTextureInfo{.type = TextureType::Texture2D,
                                     .format = Format::R8G8B8A8_UNORM,
                                     .width = m_renderWidth,
                                     .height = m_renderHeight,
                                     .depth = 1,
                                     .mipLevels = 1,
                                     .arrayLayers = 1,
                                     .samples = SampleCount::Count1,
                                     .usage = TextureUsage::ColorAttachment |
                                              TextureUsage::Sampled};
  m_colorTarget = graphicsAPI.createTexture(colorTextureInfo);

  TextureViewCreateInfo colorViewInfo{.format = Format::R8G8B8A8_UNORM,
                                      .viewType = TextureViewType::View2D};
  m_colorTargetView = m_colorTarget->createView(colorViewInfo);

  // Create depth target
  TextureCreateInfo depthTextureInfo{.type = TextureType::Texture2D,
                                     .format = Format::D32_SFLOAT,
                                     .width = m_renderWidth,
                                     .height = m_renderHeight,
                                     .depth = 1,
                                     .mipLevels = 1,
                                     .arrayLayers = 1,
                                     .samples = SampleCount::Count1,
                                     .usage = TextureUsage::DepthStencil};
  m_depthTarget = graphicsAPI.createTexture(depthTextureInfo);

  TextureViewCreateInfo depthViewInfo{.format = Format::D32_SFLOAT,
                                      .viewType = TextureViewType::View2D,
                                      .bIsDepthStencil = true};
  m_depthTargetView = m_depthTarget->createView(depthViewInfo);

  CH_LOG_INFO(NastyRendererSystem, "Render targets created: {0}x{1}", m_renderWidth,
              m_renderHeight);
}

/*
 */
void
NastyRenderer::createRenderPass() {
  auto& graphicsAPI = IGraphicsAPI::instance();

  AttachmentDescription colorAttachment{.format = Format::R8G8B8A8_UNORM,
                                        .loadOp = LoadOp::Clear,
                                        .storeOp = StoreOp::Store,
                                        .stencilLoadOp = LoadOp::DontCare,
                                        .stencilStoreOp = StoreOp::DontCare,
                                        .initialLayout = TextureLayout::Undefined,
                                        .finalLayout = TextureLayout::ShaderReadOnly};

  AttachmentDescription depthAttachment{.format = Format::D32_SFLOAT,
                                        .loadOp = LoadOp::Clear,
                                        .storeOp = StoreOp::DontCare,
                                        .stencilLoadOp = LoadOp::DontCare,
                                        .stencilStoreOp = StoreOp::DontCare,
                                        .initialLayout = TextureLayout::Undefined,
                                        .finalLayout = TextureLayout::DepthStencilAttachment};

  AttachmentReference colorRef{.attachment = 0, .layout = TextureLayout::ColorAttachment};

  AttachmentReference depthRef{.attachment = 1,
                               .layout = TextureLayout::DepthStencilAttachment};

  SubpassDescription subpass{.pipelineBindPoint = PipelineBindPoint::Graphics,
                             .colorAttachments = {colorRef},
                             .depthStencilAttachment = depthRef};

  SubpassDependency dependency{.srcSubpass = SUBPASS_EXTERNAL,
                               .dstSubpass = 0,
                               .srcStageMask = PipelineStage::ColorAttachmentOutput,
                               .dstStageMask = PipelineStage::ColorAttachmentOutput,
                               .srcAccessMask = Access::NoAccess,
                               .dstAccessMask = Access::ColorAttachmentWrite};

  RenderPassCreateInfo renderPassInfo{.attachments = {colorAttachment, depthAttachment},
                                      .subpasses = {subpass},
                                      .dependencies = {dependency}};

  m_renderPass = graphicsAPI.createRenderPass(renderPassInfo);

  // Create framebuffer
  FrameBufferCreateInfo fbInfo{.renderPass = m_renderPass,
                               .attachments = {m_colorTargetView, m_depthTargetView},
                               .width = m_renderWidth,
                               .height = m_renderHeight,
                               .layers = 1};
  m_framebuffer = graphicsAPI.createFrameBuffer(fbInfo);

  CH_LOG_INFO(NastyRendererSystem, "Render pass and framebuffer created");
}

/*
 */
void
NastyRenderer::initializeRenderResources() {
  auto& graphicsAPI = IGraphicsAPI::instance();

  // Create command pool and buffer
  m_commandPool = graphicsAPI.createCommandPool(QueueType::Graphics);
  m_commandBuffer = m_commandPool->allocateCommandBuffer();
  m_renderFence = graphicsAPI.createFence(true);

  // Load texture
  int32 imageWidth = 0;
  int32 imageHeight = 0;
  int32 channel = 0;
  Vector<uint8> imageData = RendererHelpers::loadImage(Path("resources/images/Rex_C.bmp"),
                                                       &imageWidth, &imageHeight, &channel);

  TextureCreateInfo textureCreateInfo{.type = TextureType::Texture2D,
                                      .format = Format::R8G8B8A8_UNORM,
                                      .width = static_cast<uint32>(imageWidth),
                                      .height = static_cast<uint32>(imageHeight),
                                      .depth = 1,
                                      .mipLevels = 1,
                                      .arrayLayers = 1,
                                      .samples = SampleCount::Count1,
                                      .usage =
                                          TextureUsage::Sampled | TextureUsage::TransferDst,
                                      .initialData = imageData.data(),
                                      .initialDataSize = imageData.size()};
  m_texture = graphicsAPI.createTexture(textureCreateInfo);

  TextureViewCreateInfo textureViewCreateInfo{.format = Format::R8G8B8A8_UNORM,
                                              .viewType = TextureViewType::View2D,
                                              .baseMipLevel = 0,
                                              .mipLevelCount = 1,
                                              .baseArrayLayer = 0,
                                              .arrayLayerCount = 1};
  m_textureView = m_texture->createView(textureViewCreateInfo);

  // Create camera
  m_camera =
      chMakeUnique<Camera>(initialCameraPos, Vector3::ZERO, m_renderWidth, m_renderHeight);
  m_camera->setProjectionType(CameraProjectionType::Perspective);
  m_camera->setFieldOfView(g_FOV);
  m_camera->setClipPlanes(g_nearPlane, g_farPlane);
  m_camera->updateMatrices();

  // Create descriptor set layout
  Vector<DescriptorSetLayoutBinding> bindings{{.binding = 0,
                                               .type = DescriptorType::UniformBuffer,
                                               .count = 1,
                                               .stageFlags = ShaderStage::Vertex},
                                              {.binding = 1,
                                               .type = DescriptorType::CombinedImageSampler,
                                               .count = 1,
                                               .stageFlags = ShaderStage::Fragment}};

  DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{.bindings = bindings};
  m_descriptorSetLayout = graphicsAPI.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);

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

  // Load shaders
  ShaderCreateInfo shaderCreateInfo{
      .stage = ShaderStage::Vertex,
      .entryPoint = "main",
      .sourceCode = FileSystem::fastRead(Path("resources/shaders/cubeVertex.spv")),
      .filePath = "resources/shaders/cubeVertex.spv",
      .defines = {}};

  ShaderCreateInfo fragmentShaderCreateInfo{
      .stage = ShaderStage::Fragment,
      .entryPoint = "main",
      .sourceCode = FileSystem::fastRead(Path("resources/shaders/cubeFragment.spv")),
      .filePath = "resources/shaders/cubeFragment.spv",
      .defines = {}};

  m_vertexShader = graphicsAPI.createShader(shaderCreateInfo);
  m_fragmentShader = graphicsAPI.createShader(fragmentShaderCreateInfo);

  // Create pipeline
  PipelineCreateInfo pipelineCreateInfo{
      .shaders = {{ShaderStage::Vertex, m_vertexShader},
                  {ShaderStage::Fragment, m_fragmentShader}},
      .vertexLayout = VertexNormalTexCoord::getLayout(),
      .topology = PrimitiveTopology::TriangleList,
      .depthStencil = {.enable = true, .writeEnable = true, .compareOp = CompareOp::Less},
      .renderPass = m_renderPass,
      .setLayouts = {m_descriptorSetLayout}};
  m_pipeline = graphicsAPI.createPipeline(pipelineCreateInfo);

  CH_LOG_INFO(NastyRendererSystem, "Render resources initialized");
}

/*
 */
void
NastyRenderer::loadModel(const SPtr<Model>& model) {

  cleanupModelResources();

  // Load model using your model manager (uncomment when available)
  //SPtr<Model> model = AssetManager::getInstance().getAsset<Model>();
  // if (!model) {
  //   CH_LOG_ERROR(NastyRendererSystem, "Failed to load model");
  //   return;
  // }
  m_currentModel = model;

  // For now, create a temporary model placeholder
  // You can replace this with actual model loading later
  if (!m_currentModel) {
    CH_LOG_WARNING(NastyRendererSystem, "No model manager available, skipping model load");
    return;
  }

  // Create mesh buffers and descriptor resources
  createMeshBuffers();
  createNodeDescriptorResources();

  // Apply initial transformation
  RotationMatrix rotationMatrix(Rotator(180.0f, 0.0f, 90.0f));
  if (!NodeNames.empty()) {
    m_currentModel->updateNodeTransform(m_currentModel->findNode(NodeNames[NodeIndex]),
                                        rotationMatrix);
  }

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
  for (size_t i = 0; i < uniqueMeshes.size(); ++i) {
    const auto& mesh = uniqueMeshes[i];

    // Create vertex buffer
    const Vector<uint8>& vertexData = mesh->getVertexData();
    const uint32 vertexDataSize = mesh->getVertexDataSize();

    BufferCreateInfo vertexBufferCreateInfo{
        .size = vertexDataSize,
        .usage = BufferUsage::VertexBuffer,
        .memoryUsage = MemoryUsage::CpuToGpu,
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
      const uint32 indexDataSize = mesh->getIndexDataSize();

      BufferCreateInfo indexBufferCreateInfo{
          .size = indexDataSize,
          .usage = BufferUsage::IndexBuffer,
          .memoryUsage = MemoryUsage::CpuToGpu,
          .initialData = const_cast<void*>(static_cast<const void*>(indexData.data())),
          .initialDataSize = indexDataSize,
      };
      m_meshIndexBuffers[i] = graphicsAPI.createBuffer(indexBufferCreateInfo);
    }
    else {
      Vector<uint32> indexData = mesh->getIndicesAsUInt32();
      const uint32 indexDataSize = mesh->getIndexDataSize();

      BufferCreateInfo indexBufferCreateInfo{
          .size = indexDataSize,
          .usage = BufferUsage::IndexBuffer,
          .memoryUsage = MemoryUsage::CpuToGpu,
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
NastyRenderer::createNodeDescriptorResources() {
  auto& graphicsAPI = IGraphicsAPI::instance();

  if (!m_currentModel) {
    return;
  }

  uint32 nodeCount = static_cast<uint32>(m_currentModel->getAllNodes().size());

  // Create descriptor pool for all nodes
  DescriptorPoolCreateInfo descriptorPoolCreateInfo{
      .maxSets = nodeCount,
      .poolSizes = {{DescriptorType::UniformBuffer, nodeCount},
                    {DescriptorType::CombinedImageSampler, nodeCount}}};
  m_descriptorPool = graphicsAPI.createDescriptorPool(descriptorPoolCreateInfo);

  // Create descriptor resources for each node
  for (ModelNode* node : m_currentModel->getAllNodes()) {
    if (node->getMeshes().empty()) {
      continue;
    }

    // Create uniform buffer for this node
    BufferCreateInfo bufferCreateInfo{.size = sizeof(RendererHelpers::ProjectionViewMatrix),
                                      .usage = BufferUsage::UniformBuffer,
                                      .memoryUsage = MemoryUsage::CpuToGpu};
    SPtr<IBuffer> nodeBuffer = graphicsAPI.createBuffer(bufferCreateInfo);

    // Allocate descriptor set for this node
    DescriptorSetAllocateInfo allocInfo{.pool = m_descriptorPool,
                                        .layout = m_descriptorSetLayout};
    SPtr<IDescriptorSet> nodeDescriptorSet =
        m_descriptorPool->allocateDescriptorSet(allocInfo);

    // Setup descriptor writes
    DescriptorBufferInfo bufferInfo{.buffer = nodeBuffer,
                                    .offset = 0,
                                    .range = sizeof(RendererHelpers::ProjectionViewMatrix)};

    DescriptorImageInfo imageInfo{.sampler = m_sampler,
                                  .imageView = m_textureView,
                                  .imageLayout = TextureLayout::ShaderReadOnly};

    Vector<WriteDescriptorSet> writeDescriptorSets{
        {.dstSet = nodeDescriptorSet,
         .dstBinding = 0,
         .dstArrayElement = 0,
         .descriptorType = DescriptorType::UniformBuffer,
         .bufferInfos = {bufferInfo}},
        {.dstSet = nodeDescriptorSet,
         .dstBinding = 1,
         .dstArrayElement = 0,
         .descriptorType = DescriptorType::CombinedImageSampler,
         .imageInfos = {imageInfo}}};

    // Update descriptor sets
    graphicsAPI.updateDescriptorSets(writeDescriptorSets);

    // Store node resources
    m_nodeResources[node] = {.uniformBuffer = nodeBuffer, .descriptorSet = nodeDescriptorSet};
  }

  CH_LOG_INFO(NastyRendererSystem, "Created descriptor resources for {0} nodes", nodeCount);
}

/*
 */
void
NastyRenderer::bindInputEvents() {
  EventDispatcherManager& eventDispatcher = EventDispatcherManager::instance();

  HEvent listenKeyDown = eventDispatcher.OnKeyDown.connect([&](const KeyBoardData& keydata) {
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

    if (keydata.key == Key::Num9) {
      ModelIndex = (ModelIndex + 1) % ModelPaths.size();
      CH_LOG_INFO(NastyRendererSystem, "Loading model: {0}", ModelPaths[ModelIndex]);
      //loadModel();
    }
  });

  HEvent listenKeys = eventDispatcher.OnKeyPressed.connect([&](const KeyBoardData& keydata) {
    if (!m_camera) {
      return;
    }

    float moveSpeed = g_cameraMoveSpeed * 0.1f;
    switch (keydata.key) {
    case Key::W:
      m_camera->moveForward(moveSpeed);
      break;
    case Key::S:
      m_camera->moveForward(-moveSpeed);
      break;
    case Key::A:
      m_camera->moveRight(-moveSpeed);
      break;
    case Key::D:
      m_camera->moveRight(moveSpeed);
      break;
    case Key::Q:
      m_camera->moveUp(moveSpeed);
      break;
    case Key::E:
      m_camera->moveUp(-moveSpeed);
      break;
    case Key::R:
      m_camera->setPosition(initialCameraPos);
      m_camera->lookAt(Vector3::ZERO);
      break;
    default:
      return;
    }

    projectionViewMatrix.viewMatrix = m_camera->getViewMatrix();
  });

  HEvent listenWheel =
      eventDispatcher.OnMouseWheel.connect([&](const MouseWheelData& wheelData) {
        if (m_camera && wheelData.deltaY != 0) {
          m_camera->moveForward(wheelData.deltaY * g_cameraMoveSpeed);
        }
      });

  HEvent listenMouse =
      eventDispatcher.OnMouseMove.connect([&](const MouseMoveData& mouseData) {
        if (!m_camera) {
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
            m_camera->pan(-mouseData.deltaX * g_cameraPanSpeed,
                          -mouseData.deltaY * g_cameraPanSpeed);
          }
          if (isMouseButtonDown) {
            m_camera->rotate(mouseData.deltaY * g_rotationSpeed,
                             mouseData.deltaX * g_rotationSpeed, 0.0f);
          }
          projectionViewMatrix.viewMatrix = m_camera->getViewMatrix();
        }
      });

  CH_LOG_INFO(NastyRendererSystem, "Input events bound");
}

/*
 */
void
NastyRenderer::renderModel(const SPtr<ICommandBuffer>& commandBuffer, float deltaTime) {
  if (!m_currentModel) {
    return;
  }

  m_currentModel->updateTransforms();

  if (bIsModelRotating && !NodeNames.empty()) {
    if (ModelNode* targetNode = m_currentModel->findNode(NodeNames[NodeIndex])) {
      const Matrix4 originalTransform = targetNode->getLocalTransform();
      RotationMatrix rotationMatrix(Rotator(0.0f, deltaTime * 20, 0.0f));
      const Matrix4 newTransform = originalTransform * rotationMatrix;
      m_currentModel->updateNodeTransform(targetNode, newTransform);
    }
  }

  const Matrix4& projectionMatrix = m_camera->getProjectionMatrix();
  const Matrix4& viewMatrix = m_camera->getViewMatrix();

  for (ModelNode* node : m_currentModel->getAllNodes()) {
    if (node->getMeshes().empty()) {
      continue;
    }

    auto it = m_nodeResources.find(node);
    if (it == m_nodeResources.end()) {
      CH_LOG_ERROR(NastyRendererSystem, "No render resources found for node: {0}",
                   node->getName());
      continue;
    }

    NodeRenderResources& resources = it->second;

    RendererHelpers::ProjectionViewMatrix matrices = {.projectionMatrix = projectionMatrix,
                                                      .viewMatrix = viewMatrix,
                                                      .modelMatrix =
                                                          node->getGlobalTransform()};

    resources.uniformBuffer->update(&matrices, sizeof(matrices));

    commandBuffer->bindDescriptorSets(PipelineBindPoint::Graphics, m_pipeline->getLayout(), 0,
                                      {resources.descriptorSet});

    for (const auto& mesh : node->getMeshes()) {
      uint32 meshIndex = m_meshToIndexMap[mesh];

      commandBuffer->bindVertexBuffer(m_meshVertexBuffers[meshIndex]);
      commandBuffer->bindIndexBuffer(m_meshIndexBuffers[meshIndex],
                                     m_meshIndexTypes[meshIndex]);
      commandBuffer->drawIndexed(m_meshIndexCounts[meshIndex]);
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

  // Clear node resources
  for (auto& pair : m_nodeResources) {
    pair.second.uniformBuffer.reset();
    pair.second.descriptorSet.reset();
  }
  m_nodeResources.clear();

  // Clear node names
  NodeNames.clear();
  NodeIndex = 0;

  // Reset descriptor pool
  if (m_descriptorPool) {
    m_descriptorPool.reset();
  }

  // Reset current model
  m_currentModel.reset();

  CH_LOG_INFO(NastyRendererSystem, "Model resources cleaned up");
}

} // namespace chEngineSDK
