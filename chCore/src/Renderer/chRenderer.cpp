/************************************************************************/
/**
 * @file chRenderer.h
 * @author AccelMR
 * @date 2025/04/09
 */
/************************************************************************/

#include "chRenderer.h"

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
#include "chISwapChain.h"
#include "chISynchronization.h"
#include "chITexture.h"
#include "chITextureView.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace chEngineSDK {

#if USING(CH_DEBUG_MODE)
#define CH_RENDERER_LOG_LEVEL All
#else
#define CH_RENDERER_LOG_LEVEL Info
#endif //USING(CH_DEBUG_MODE)

CH_LOG_DECLARE_STATIC(RendererSystem, CH_RENDERER_LOG_LEVEL);

namespace RendererHelpers{
struct ProjectionViewMatrix{
  Matrix4 projectionMatrix;
  Matrix4 viewMatrix;
  Matrix4 modelMatrix;
};

Vector<uint8> loadImage(const Path& path,
                        int32* width,
                        int32* height,
                        int32* channels) {
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

RendererHelpers::ProjectionViewMatrix projectionViewMatrix;
float g_farPlane = 10000.0f;
float g_nearPlane = 0.1f;
Radian g_FOV(Degree(45.0f));
float g_cameraPanSpeed = 0.01f;
float g_cameraMoveSpeed = 0.1f;
float g_rotationSpeed = 0.1f;
SPtr<Model> g_model;

Vector3 initialCameraPos(-5.0f, 0.0f, 0.0f);

static Vector<String> NodeNames;
static uint32 NodeIndex = 0;
static bool bIsModelRotating = false;
static Array<Path, 5> ModelPaths = {
  Path("resources/models/rex_norm.obj"),
  Path("resources/models/cyberdemon.md5mesh"),
  Path("resources/models/Porch.fbx"),
  Path("resources/models/test.fbx"),
  Path("resources/models/Porce/scene.gltf")

};
static uint32 ModelIndex = 0;

static constexpr uint64 MAX_WAIT_TIME = 1000000000; // 1 second

/*
*/
Renderer::~Renderer() {
  auto& graphicsAPI = IGraphicsAPI::instance();

  graphicsAPI.getQueue(QueueType::Graphics)->waitIdle();

  graphicsAPI.waitIdle();

  for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    if (m_inFlightFences[i]){
      m_inFlightFences[i]->wait();
    }
  }

  m_commandBuffers.clear();
  m_commandPool->reset();

  m_imageAvailableSemaphores.clear();
  m_renderFinishedSemaphores.clear();
  m_inFlightFences.clear();

  m_vertexShader.reset();
  m_fragmentShader.reset();
  m_pipeline.reset();

  m_swapChain.reset();
}

/*
*/
void
Renderer::initialize(uint32 width,
                     uint32 height,
                     bool vsync) {
  CH_ASSERT(IGraphicsAPI::instancePtr() != nullptr);

  m_swapChain = IGraphicsAPI::instance().createSwapChain(width, height, vsync);

  createSyncObjects();
  initializeRenderResources();
  bindInputEvents();
}

/*
*/
void
Renderer::createSyncObjects() {
  const uint32 imageCount = m_swapChain->getTextureCount();

  m_imageAvailableSemaphores.resize(imageCount);
  m_renderFinishedSemaphores.resize(imageCount);
  m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  auto& graphicsAPI = IGraphicsAPI::instance();
  for (uint32 i = 0; i < imageCount; ++i) {
    m_imageAvailableSemaphores[i] = graphicsAPI.createSemaphore();
    m_renderFinishedSemaphores[i] = graphicsAPI.createSemaphore();
  }

  for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    m_inFlightFences[i] = graphicsAPI.createFence(true);
  }
}

/*
*/
void
Renderer::loadModel() {
  auto& graphicsAPI = IGraphicsAPI::instance();

  cleanupModelResources();
  const Path& modelPath = ModelPaths[ModelIndex];
  CH_LOG_INFO(RendererSystem, "Loading model: {0}", modelPath.toString());

  SPtr<Model> model = MeshManager::instance().loadModel(modelPath);
  if (!model) {
    CH_LOG_ERROR(RendererSystem, "Failed to load model");
    return;
  }
  m_currentModel = model;

  Vector<SPtr<Mesh>> uniqueMeshes;
  UnorderedMap<SPtr<Mesh>, uint32> meshToIndexMap;

  // Primero recopilamos todas las mallas únicas
  for (ModelNode* node : model->getAllNodes()) {
    NodeNames.push_back(node->getName());
    for (const auto& mesh : node->getMeshes()) {
      // Si esta malla no está ya en nuestra lista
      if (meshToIndexMap.find(mesh) == meshToIndexMap.end()) {
        meshToIndexMap[mesh] = static_cast<uint32>(uniqueMeshes.size());
        uniqueMeshes.push_back(mesh);
      }
    }
  }

  m_meshVertexBuffers.resize(uniqueMeshes.size());
  m_meshIndexBuffers.resize(uniqueMeshes.size());
  m_meshIndexCounts.resize(uniqueMeshes.size());
  m_meshIndexTypes.resize(uniqueMeshes.size());

  for (size_t i = 0; i < uniqueMeshes.size(); ++i) {
    const auto& mesh = uniqueMeshes[i];

    const Vector<uint8>& vertexData = mesh->getVertexData();
    const uint32 vertexDataSize = mesh->getVertexDataSize();

    BufferCreateInfo bufferCreateInfo{
        .size = vertexDataSize,
        .usage = BufferUsage::VertexBuffer,
        .memoryUsage = MemoryUsage::CpuToGpu,
        .initialData = const_cast<void*>(static_cast<const void*>(vertexData.data())),
        .initialDataSize = vertexDataSize,
    };
    m_meshVertexBuffers[i] = graphicsAPI.createBuffer(bufferCreateInfo);

    m_meshIndexTypes[i] = mesh->getIndexType();
    m_meshIndexCounts[i] = mesh->getIndexCount();

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

  m_meshToIndexMap = std::move(meshToIndexMap);

  uint32 nodeCount = static_cast<uint32>(model->getAllNodes().size());
  DescriptorPoolCreateInfo descriptorPoolCreateInfo{
    .maxSets = nodeCount,
    .poolSizes = {
      { DescriptorType::UniformBuffer, nodeCount },
      { DescriptorType::CombinedImageSampler, nodeCount }
    }
  };
  m_descriptorPool = graphicsAPI.createDescriptorPool(descriptorPoolCreateInfo);

  for (ModelNode* node : m_currentModel->getAllNodes()) {
    if (node->getMeshes().empty()) {
      continue;
    }

    BufferCreateInfo bufferCreateInfo{
      .size = sizeof(RendererHelpers::ProjectionViewMatrix),
      .usage = BufferUsage::UniformBuffer,
      .memoryUsage = MemoryUsage::CpuToGpu
    };
    SPtr<IBuffer> nodeBuffer = graphicsAPI.createBuffer(bufferCreateInfo);

    DescriptorSetAllocateInfo allocInfo{
      .pool = m_descriptorPool,
      .layout = m_descriptorSetLayout
    };
    SPtr<IDescriptorSet> nodeDescriptorSet = m_descriptorPool->allocateDescriptorSet(allocInfo);

    DescriptorBufferInfo bufferInfo{
      .buffer = nodeBuffer,
      .offset = 0,
      .range = sizeof(RendererHelpers::ProjectionViewMatrix)
    };

    DescriptorImageInfo imageInfo{
      .sampler = m_sampler,
      .imageView = m_textureView,
      .imageLayout = TextureLayout::ShaderReadOnly
    };

    Vector<WriteDescriptorSet> writeDescriptorSets{
      {
        .dstSet = nodeDescriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorType = DescriptorType::UniformBuffer,
        .bufferInfos = { bufferInfo }
      },
      {
        .dstSet = nodeDescriptorSet,
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorType = DescriptorType::CombinedImageSampler,
        .imageInfos = { imageInfo }
      }
    };

    graphicsAPI.updateDescriptorSets(writeDescriptorSets);

    m_nodeResources[node] = {
      .uniformBuffer = nodeBuffer,
      .descriptorSet = nodeDescriptorSet
    };
  }

  RotationMatrix rotationMatrix(Rotator(180.0f, 0.0f, 90.0f));
  m_currentModel->updateNodeTransform(m_currentModel->findNode(NodeNames[NodeIndex]), rotationMatrix);
}

/*
*/
void
Renderer::initializeRenderResources() {
  auto& graphicsAPI = IGraphicsAPI::instance();

  m_width = m_swapChain->getWidth();
  m_height = m_swapChain->getHeight();

  m_commandPool = graphicsAPI.createCommandPool(QueueType::Graphics);
  m_commandBuffers.resize(m_swapChain->getTextureCount());
  for (auto& commandBuffer : m_commandBuffers) {
    commandBuffer = m_commandPool->allocateCommandBuffer();
  }

  int32 imageWidth = 0;
  int32 imageHeight = 0;
  int32 channel = 0;
  Vector<uint8> imageData = RendererHelpers::loadImage(Path("resources/images/Rex_C.bmp"),
                                                       &imageWidth,
                                                       &imageHeight,
                                                       &channel);

  TextureCreateInfo textureCreateInfo{
    .type = TextureType::Texture2D,
    .format = Format::R8G8B8A8_UNORM,
    .width = static_cast<uint32>(imageWidth),
    .height = static_cast<uint32>(imageHeight),
    .depth = 1,
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = SampleCount::Count1,
    .usage = TextureUsage::Sampled | TextureUsage::TransferDst,
    .initialData = imageData.data(),
    .initialDataSize = imageData.size()
  };
  m_texture = graphicsAPI.createTexture(textureCreateInfo);

  TextureViewCreateInfo textureViewCreateInfo{
    .format = Format::R8G8B8A8_UNORM,
    .viewType = TextureViewType::View2D,
    .baseMipLevel = 0,
    .mipLevelCount = 1,
    .baseArrayLayer = 0,
    .arrayLayerCount = 1
  };
  m_textureView = m_texture->createView(textureViewCreateInfo);

  // Camera setup
  m_camera = chMakeUnique<Camera>(initialCameraPos, Vector3::ZERO, m_width, m_height);
  m_camera->setProjectionType(CameraProjectionType::Perspective);
  m_camera->setFieldOfView(g_FOV);
  m_camera->setClipPlanes(g_nearPlane, g_farPlane);
  m_camera->updateMatrices();

  // Configuración de layouts y descriptores
  Vector<DescriptorSetLayoutBinding> bindings{
    {
      .binding = 0,
      .type = DescriptorType::UniformBuffer,
      .count = 1,
      .stageFlags = ShaderStage::Vertex
    },
    {
      .binding = 1,
      .type = DescriptorType::CombinedImageSampler,
      .count = 1,
      .stageFlags = ShaderStage::Fragment
    }
  };

  DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{
    .bindings = bindings
  };
  m_descriptorSetLayout = graphicsAPI.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);

  SamplerCreateInfo samplerCreateInfo {
    .magFilter = SamplerFilter::Linear,
    .minFilter = SamplerFilter::Linear,
    .mipmapMode = SamplerMipmapMode::Linear,
    .addressModeU = SamplerAddressMode::Repeat,
    .addressModeV = SamplerAddressMode::Repeat,
    .addressModeW = SamplerAddressMode::Repeat,
    .anisotropyEnable = false,
    .maxAnisotropy = 16.0f
  };
  m_sampler = graphicsAPI.createSampler(samplerCreateInfo);

  loadModel();
  ShaderCreateInfo shaderCreateInfo{
    .stage = ShaderStage::Vertex,
    .entryPoint = "main",
    .sourceCode = FileSystem::fastRead(Path("resources/shaders/cubeVertex.spv")),
    .filePath = "resources/shaders/cubeVertex.spv",
    .defines = { /* Preprocessor defines */ }
  };

  ShaderCreateInfo fragmentShaderCreateInfo{
    .stage = ShaderStage::Fragment,
    .entryPoint = "main",
    .sourceCode = FileSystem::fastRead(Path("resources/shaders/cubeFragment.spv")),
    .filePath = "resources/shaders/cubeFragment.spv",
    .defines = { /* Preprocessor defines */ }
  };

  m_vertexShader = graphicsAPI.createShader(shaderCreateInfo);
  m_fragmentShader = graphicsAPI.createShader(fragmentShaderCreateInfo);

  TextureCreateInfo depthStencilCreateInfo{
    .type = TextureType::Texture2D,
    .format = Format::D32_SFLOAT,
    .width = m_swapChain->getWidth(),
    .height = m_swapChain->getHeight(),
    .depth = 1,
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = SampleCount::Count1,
    .usage = TextureUsage::DepthStencil
  };
  m_depthTexture = graphicsAPI.createTexture(depthStencilCreateInfo);

  TextureViewCreateInfo depthStencilViewCreateInfo{
    .format = Format::D32_SFLOAT,
    .viewType = TextureViewType::View2D,
    .bIsDepthStencil = true
  };
  m_depthTextureView =

  m_depthTexture->createView(depthStencilViewCreateInfo);

  createRenderPass();

  m_framebuffers.resize(m_swapChain->getTextureCount());
  for (uint32 i = 0; i < m_swapChain->getTextureCount(); i++) {
    FrameBufferCreateInfo fbInfo{
      .renderPass = m_renderPass,
      .attachments = {m_swapChain->getTextureView(i), m_depthTextureView},
      .width = m_width,
      .height = m_height,
      .layers = 1
    };
    m_framebuffers[i] = graphicsAPI.createFrameBuffer(fbInfo);
  }

  PipelineCreateInfo pipelineCreateInfo{
    .shaders = {
      { ShaderStage::Vertex, m_vertexShader },
      { ShaderStage::Fragment, m_fragmentShader }
    },
    .vertexLayout = VertexNormalTexCoord::getLayout(),
    .topology = PrimitiveTopology::TriangleList,
    .depthStencil = {
      .enable = true,
      .writeEnable = true,
      .compareOp = CompareOp::Less
    },
    .renderPass = m_renderPass,
    .setLayouts = { m_descriptorSetLayout },
  };
  m_pipeline = graphicsAPI.createPipeline(pipelineCreateInfo);
}

/*
*/
void
Renderer::render(const float deltaTime) {
  auto& graphicsAPI = IGraphicsAPI::instance();

  m_inFlightFences[m_currentFrame]->wait(MAX_WAIT_TIME);
  m_inFlightFences[m_currentFrame]->reset();

  // if (m_imageAvailableSemaphores.empty()) {
  //   CH_LOG_ERROR(RendererSystem, "No image available semaphores");
  //   return;
  // }

  //m_currentFrame = m_currentFrame % m_imageAvailableSemaphores.size();

  if (!m_swapChain->acquireNextImage(m_imageAvailableSemaphores[m_currentFrame])){
    resize();
    return;
  }
  uint32 imageIndex = m_swapChain->getCurrentImageIndex();

  //TODO: might need to fix this
  if (imageIndex >= m_commandBuffers.size()) {
    CH_LOG_ERROR(RendererSystem, "Image index out of range, swapchain may be resized. Deferring to next frame.");
    return;
  }

  auto& cmdBuffer = m_commandBuffers[imageIndex];
  cmdBuffer->begin();

  RenderPassBeginInfo renderPassInfo{
    .renderPass = m_renderPass,
    .framebuffer = m_framebuffers[imageIndex],
    .clearValues = {
      LinearColor::Black
    },
    .depthStencilClearValue = {
      {1.0f, 0}
    }
  };

  cmdBuffer->beginRenderPass(renderPassInfo);
  cmdBuffer->setViewport(0, 0, m_swapChain->getWidth(), m_swapChain->getHeight());
  cmdBuffer->setScissor(0, 0, m_swapChain->getWidth(), m_swapChain->getHeight());
  cmdBuffer->bindPipeline(m_pipeline);

  projectionViewMatrix.viewMatrix = m_camera->getViewMatrix();
  projectionViewMatrix.projectionMatrix = m_camera->getProjectionMatrix();

  renderModel(cmdBuffer, deltaTime);

  cmdBuffer->endRenderPass();
  cmdBuffer->end();

  Vector<SPtr<ICommandBuffer>> submitBuffers = { cmdBuffer };

  SubmitInfo submitInfo{
    .commandBuffers = { m_commandBuffers[imageIndex] },
    .waitSemaphores = { m_imageAvailableSemaphores[m_currentFrame] },
    .waitStages = { PipelineStage::ColorAttachmentOutput },
    .signalSemaphores = { m_renderFinishedSemaphores[imageIndex] },
  };

  graphicsAPI.getQueue(QueueType::Graphics)->submit(submitInfo, m_inFlightFences[m_currentFrame]);

  m_swapChain->present({ m_renderFinishedSemaphores[imageIndex] });


  m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/*
*/
void
Renderer::resize() {
  auto& graphicsAPI = IGraphicsAPI::instance();
  graphicsAPI.waitIdle();

  for (auto& fence : m_inFlightFences) {
    if (fence) {
      fence->wait(MAX_WAIT_TIME * 10);
    }
  }

  m_imageAvailableSemaphores.clear();
  m_renderFinishedSemaphores.clear();

  m_swapChain->resize(m_width, m_height);

  uint32 swapchainWidth = m_swapChain->getWidth();
  uint32 swapchainHeight = m_swapChain->getHeight();

  m_width = swapchainWidth;
  m_height = swapchainHeight;

  TextureCreateInfo depthTextureInfo{
    .type = TextureType::Texture2D,
    .format = Format::D32_SFLOAT,
    .width = swapchainWidth,
    .height = swapchainHeight,
    .depth = 1,
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = SampleCount::Count1,
    .usage = TextureUsage::DepthStencil
  };

  m_depthTexture = graphicsAPI.createTexture(depthTextureInfo);
  TextureViewCreateInfo depthTextureViewInfo{
    .format = Format::D32_SFLOAT,
    .viewType = TextureViewType::View2D,
    .bIsDepthStencil = true
  };
  m_depthTextureView = m_depthTexture->createView(depthTextureViewInfo);

  createRenderPass();

  m_framebuffers.clear();
  m_framebuffers.resize(m_swapChain->getTextureCount());
  for (uint32 i = 0; i < m_swapChain->getTextureCount(); i++) {
    FrameBufferCreateInfo fbInfo{
      .renderPass = m_renderPass,
      .attachments = {m_swapChain->getTextureView(i), m_depthTextureView},
      .width = swapchainWidth,
      .height = swapchainHeight,
      .layers = 1
    };

    m_framebuffers[i] = graphicsAPI.createFrameBuffer(fbInfo);
  }

  createSyncObjects();

  m_commandBuffers.clear();
  m_commandBuffers.resize(m_swapChain->getTextureCount());
  for (auto& commandBuffer : m_commandBuffers) {
      commandBuffer = m_commandPool->allocateCommandBuffer();
  }
}

/*
*/
void
Renderer::createRenderPass() {
  auto& graphicsAPI = IGraphicsAPI::instance();

  AttachmentDescription colorAttachment{.format = m_swapChain->getFormat(),
                                        .loadOp = LoadOp::Clear,
                                        .storeOp = StoreOp::Store,
                                        .stencilLoadOp = LoadOp::DontCare,
                                        .stencilStoreOp = StoreOp::DontCare,
                                        .initialLayout = TextureLayout::Undefined,
                                        .finalLayout = TextureLayout::PresentSrc};

  AttachmentDescription depthAttachment{
      .format = Format::D32_SFLOAT,
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
}

/*
 */
void
Renderer::renderModel(const SPtr<ICommandBuffer>& commandBuffer,
                      float deltaTime) {
  m_currentModel->updateTransforms();

  if (bIsModelRotating) {
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
      CH_LOG_ERROR(RendererSystem, "No render resources found for node: {0}", node->getName());
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
Renderer::bindInputEvents() {

  EventDispatcherManager& eventDispatcher = EventDispatcherManager::instance();
  HEvent listenResize = eventDispatcher.OnResize.connect(
    [&](uint32 width,uint32 height) {
      m_width = width;
      m_height = height;
      resize();

      m_camera->setViewportSize(width, height);
      m_camera->updateMatrices();
      projectionViewMatrix.projectionMatrix = m_camera->getProjectionMatrix();
  });

  HEvent listenKeyDown = eventDispatcher.OnKeyDown.connect([&](const KeyBoardData& keydata) {
    if (keydata.key == Key::P) {
      Vector3 cameraPosition = m_camera->getPosition();
      CH_LOG_INFO(RendererSystem, "Camera Position: ({0}, {1}, {2})",
                  cameraPosition.x, cameraPosition.y, cameraPosition.z);
      return;
    }

    if (keydata.key == Key::Num1) {
      NodeIndex = (NodeIndex + 1) % NodeNames.size();
      CH_LOG_INFO(RendererSystem, "Node Rotating: {0}", NodeNames[NodeIndex]);
    }

    if (keydata.key == Key::Num2) {
      NodeIndex = (NodeIndex - 1) % NodeNames.size();
      CH_LOG_INFO(RendererSystem, "Node Rotating: {0}", NodeNames[NodeIndex]);
    }

    if (keydata.key == Key::Num3) {
      bIsModelRotating = !bIsModelRotating;
      if (bIsModelRotating) {
        CH_LOG_INFO(RendererSystem, "Model rotation enabled");
      }
      else {
        CH_LOG_INFO(RendererSystem, "Model rotation disabled");
      }
    }

    if (keydata.key == Key::Num9) {
      ModelIndex = (ModelIndex + 1) % ModelPaths.size();
      CH_LOG_INFO(RendererSystem, "Loading model: {0}", ModelPaths[ModelIndex]);
      loadModel();
    }

  });

  HEvent listenKeys = eventDispatcher.OnKeyPressed.connect([&](const KeyBoardData& keydata) {
    float moveSpeed = g_cameraMoveSpeed * 0.1f;
    switch (keydata.key) {
      case Key::W: m_camera->moveForward(moveSpeed); break;
      case Key::S: m_camera->moveForward(-moveSpeed); break;
      case Key::A: m_camera->moveRight(-moveSpeed); break;
      case Key::D: m_camera->moveRight(moveSpeed); break;
      case Key::Q: m_camera->moveUp(moveSpeed); break;
      case Key::E: m_camera->moveUp(-moveSpeed); break;
      case Key::R:
        m_camera->setPosition(initialCameraPos);
        m_camera->lookAt(Vector3::ZERO);
        break;
      default: return;
    }

    projectionViewMatrix.viewMatrix = m_camera->getViewMatrix();
  });

  HEvent listenWheel = eventDispatcher.OnMouseWheel.connect([&](const MouseWheelData& wheelData) {
    if (wheelData.deltaY != 0) {
      m_camera->moveForward(wheelData.deltaY * g_cameraMoveSpeed);
    }
  });

  HEvent listenMouse = eventDispatcher.OnMouseMove.connect([&](const MouseMoveData& mouseData) {
    const bool isMouseButtonDown = eventDispatcher.isMouseButtonDown(MouseButton::Right);
    const bool isMouseButtonDownMiddle = eventDispatcher.isMouseButtonDown(MouseButton::Middle);
    if (!isMouseButtonDown && !isMouseButtonDownMiddle) {
      return;
    }

    if (mouseData.deltaX != 0 || mouseData.deltaY != 0) {
      if (isMouseButtonDownMiddle) {
        m_camera->pan(-mouseData.deltaX * g_cameraPanSpeed, -mouseData.deltaY * g_cameraPanSpeed);
      }
      if (isMouseButtonDown) {
        m_camera->rotate(mouseData.deltaY * g_rotationSpeed,
                         mouseData.deltaX * g_rotationSpeed,
                         0.0f);
      }
      projectionViewMatrix.viewMatrix = m_camera->getViewMatrix();
    }
  });
}

void
Renderer::cleanupModelResources() {
  // Limpiar buffers de mallas
  m_meshVertexBuffers.clear();
  m_meshIndexBuffers.clear();
  m_meshIndexCounts.clear();
  m_meshIndexTypes.clear();
  m_meshToIndexMap.clear();

  // Limpiar recursos de nodos
  for (auto& pair : m_nodeResources) {
    pair.second.uniformBuffer.reset();
    pair.second.descriptorSet.reset();
  }
  m_nodeResources.clear();

  // Limpiar nombres de nodos
  NodeNames.clear();
  NodeIndex = 0;

  // Resetear el modelo actual
  m_currentModel.reset();
}

} // namespace chEngineSDK
