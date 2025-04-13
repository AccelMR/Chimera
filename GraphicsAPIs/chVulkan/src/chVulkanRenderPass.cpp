/************************************************************************/
/**
 * @file chVulkanRenderPass.cpp
 * @author AccelMR
 * @date 2025/04/10
 * @brief 
 * Vulkan render pass implementation.
 * This file contains the implementation of the render pass
 * interface for Vulkan.
 */
/************************************************************************/

#include "chVulkanRenderPass.h"


namespace chEngineSDK {
/*
*/
VulkanRenderPass::VulkanRenderPass(VkDevice device,
                                   const RenderPassCreateInfo &createInfo)
    : m_device(device)
{

  // Convertir los attachments a formato Vulkan
  Vector<VkAttachmentDescription> attachments;
  attachments.reserve(createInfo.attachments.size());

  for (const auto &attachment : createInfo.attachments)
  {
    VkAttachmentDescription vkAttachment = {};

    // Convertir formato
    vkAttachment.format = chFormatToVkFormat(attachment.format);

    // Configurar operaciones de carga/almacenamiento
    switch (attachment.loadOp)
    {
    case LoadOp::Load:
      vkAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      break;
    case LoadOp::Clear:
      vkAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      break;
    case LoadOp::DontCare:
      vkAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      break;
    default:
      vkAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }

    switch (attachment.storeOp)
    {
    case StoreOp::Store:
      vkAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      break;
    case StoreOp::DontCare:
      vkAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      break;
    default:
      vkAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    }

    // Configurar operaciones de stencil (si aplica)
    switch (attachment.stencilLoadOp)
    {
    case LoadOp::Load:
      vkAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      break;
    case LoadOp::Clear:
      vkAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      break;
    case LoadOp::DontCare:
      vkAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      break;
    default:
      vkAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }

    switch (attachment.stencilStoreOp)
    {
    case StoreOp::Store:
      vkAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
      break;
    case StoreOp::DontCare:
      vkAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      break;
    default:
      vkAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    }

    // Configurar layouts
    vkAttachment.initialLayout = textureLayoutToVkImageLayout(attachment.initialLayout);
    vkAttachment.finalLayout = textureLayoutToVkImageLayout(attachment.finalLayout);

    // Configurar muestreo (por defecto 1)
    vkAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    attachments.push_back(vkAttachment);
  }

  // Convertir subpasses y referencias
  Vector<VkSubpassDescription> vkSubpasses;
  vkSubpasses.reserve(createInfo.subpasses.size());

  // Vectores temporales para almacenar las referencias
  Vector<Vector<VkAttachmentReference>> inputAttachmentRefs;
  Vector<Vector<VkAttachmentReference>> colorAttachmentRefs;
  Vector<Vector<VkAttachmentReference>> resolveAttachmentRefs;
  Vector<Optional<VkAttachmentReference>> depthStencilAttachmentRefs;
  Vector<Vector<uint32_t>> preserveAttachmentIndices;

  inputAttachmentRefs.resize(createInfo.subpasses.size());
  colorAttachmentRefs.resize(createInfo.subpasses.size());
  resolveAttachmentRefs.resize(createInfo.subpasses.size());
  depthStencilAttachmentRefs.resize(createInfo.subpasses.size());
  preserveAttachmentIndices.resize(createInfo.subpasses.size());

  for (size_t i = 0; i < createInfo.subpasses.size(); ++i)
  {
    const auto &subpass = createInfo.subpasses[i];

    // Configurar referencias de input attachments
    inputAttachmentRefs[i].reserve(subpass.inputAttachments.size());
    for (const auto &ref : subpass.inputAttachments)
    {
      VkAttachmentReference vkRef = {
          .attachment = ref.attachment,
          .layout = textureLayoutToVkImageLayout(ref.layout)};
      inputAttachmentRefs[i].push_back(vkRef);
    }

    // Configurar referencias de color attachments
    colorAttachmentRefs[i].reserve(subpass.colorAttachments.size());
    for (const auto &ref : subpass.colorAttachments)
    {
      VkAttachmentReference vkRef = {
          .attachment = ref.attachment,
          .layout = textureLayoutToVkImageLayout(ref.layout)};
      colorAttachmentRefs[i].push_back(vkRef);
    }

    // Configurar referencias de resolve attachments
    resolveAttachmentRefs[i].reserve(subpass.resolveAttachments.size());
    for (const auto &ref : subpass.resolveAttachments)
    {
      VkAttachmentReference vkRef = {
          .attachment = ref.attachment,
          .layout = textureLayoutToVkImageLayout(ref.layout)};
      resolveAttachmentRefs[i].push_back(vkRef);
    }

    // Configurar referencia de depth stencil attachment
    if (subpass.depthStencilAttachment.has_value())
    {
      const auto &ref = subpass.depthStencilAttachment.value();
      VkAttachmentReference vkRef = {
          .attachment = ref.attachment,
          .layout = textureLayoutToVkImageLayout(ref.layout)};
      depthStencilAttachmentRefs[i] = vkRef;
    }

    // Configurar índices de preserve attachments
    preserveAttachmentIndices[i] = subpass.preserveAttachments;

    // Crear la descripción del subpass
    VkSubpassDescription vkSubpass = {};

    // Configurar tipo de binding
    switch (subpass.pipelineBindPoint)
    {
    case PipelineBindPoint::Graphics:
      vkSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      break;
    case PipelineBindPoint::Compute:
      vkSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
      break;
    default:
      vkSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    }

    // Asignar referencias
    vkSubpass.inputAttachmentCount = static_cast<uint32>(inputAttachmentRefs[i].size());
    vkSubpass.pInputAttachments = inputAttachmentRefs[i].empty() ? nullptr : inputAttachmentRefs[i].data();

    vkSubpass.colorAttachmentCount = static_cast<uint32>(colorAttachmentRefs[i].size());
    vkSubpass.pColorAttachments = colorAttachmentRefs[i].empty() ? nullptr : colorAttachmentRefs[i].data();

    vkSubpass.pResolveAttachments = resolveAttachmentRefs[i].empty() ? nullptr : resolveAttachmentRefs[i].data();

    vkSubpass.pDepthStencilAttachment = depthStencilAttachmentRefs[i].has_value() ? &depthStencilAttachmentRefs[i].value() : nullptr;

    vkSubpass.preserveAttachmentCount = static_cast<uint32>(preserveAttachmentIndices[i].size());
    vkSubpass.pPreserveAttachments = preserveAttachmentIndices[i].empty() ? nullptr : preserveAttachmentIndices[i].data();

    vkSubpasses.push_back(vkSubpass);
  }

  // Convertir dependencias
  Vector<VkSubpassDependency> vkDependencies;
  vkDependencies.reserve(createInfo.dependencies.size());

  for (const auto &dependency : createInfo.dependencies)
  {
    VkSubpassDependency vkDependency = {};

    vkDependency.srcSubpass = dependency.srcSubpass;
    vkDependency.dstSubpass = dependency.dstSubpass;

    // Convertir flags de stage y access mask
    vkDependency.srcStageMask = pipelineStageToVkPipelineStage(dependency.srcStageMask);
    vkDependency.dstStageMask = pipelineStageToVkPipelineStage(dependency.dstStageMask);
    vkDependency.srcAccessMask = accessFlagsToVkAccessFlags(dependency.srcAccessMask);
    vkDependency.dstAccessMask = accessFlagsToVkAccessFlags(dependency.dstAccessMask);

    vkDependency.dependencyFlags = dependency.byRegion ? VK_DEPENDENCY_BY_REGION_BIT : 0;

    vkDependencies.push_back(vkDependency);
  }

  // Crear la estructura de creación del render pass
  VkRenderPassCreateInfo vkCreateInfo = {};
  vkCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  vkCreateInfo.attachmentCount = static_cast<uint32>(attachments.size());
  vkCreateInfo.pAttachments = attachments.data();
  vkCreateInfo.subpassCount = static_cast<uint32>(vkSubpasses.size());
  vkCreateInfo.pSubpasses = vkSubpasses.data();
  vkCreateInfo.dependencyCount = static_cast<uint32>(vkDependencies.size());
  vkCreateInfo.pDependencies = vkDependencies.data();

  // Crear el render pass
  VK_CHECK(vkCreateRenderPass(device, &vkCreateInfo, nullptr, &m_renderPass));
}

/*
*/
VulkanRenderPass::~VulkanRenderPass() {
  if (m_renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    m_renderPass = VK_NULL_HANDLE;
  }
}
} // namespace chEngineSDK