/************************************************************************/
/**
 * @file chVulkanRenderPass.cpp
 * @author AccelMR
 * @date 2025/02/02
 *  Vulkan-specific implementation of a GPU RenderPass.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chVulkanRenderPass.h"

#include "chVulkanGraphicsModule.h"
#include "chVulkanTranslator.h"

namespace chEngineSDK {

/*
*/
VulkanRenderPass::~VulkanRenderPass() {
  const VkDevice& device = g_VulkanGraphicsModule().getDevice();
  if (m_renderPass) {
    vkDestroyRenderPass(device, m_renderPass, nullptr);
  }
}

/*
*/
void
VulkanRenderPass::_internalInit(const chGPUDesc::RenderPassDesc& desc) {
  GraphicsModuleVulkan& VulkanAPI = g_VulkanGraphicsModule();
  const VkDevice& device = VulkanAPI.getDevice();

  Vector<VkAttachmentDescription> attachments;
  for(const auto& att: desc.attachments) {
    VkAttachmentDescription attachment{};
    attachment.format = VulkanTranslator::get(att.format);
    attachment.samples = VulkanTranslator::get(att.sampleCount);
    attachment.loadOp = VulkanTranslator::get(att.loadOp);
    attachment.storeOp = VulkanTranslator::get(att.storeOp);
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    attachment.initialLayout = VulkanTranslator::get(att.initialLayout);
    attachment.finalLayout = VulkanTranslator::get(att.finalLayout);

    attachments.push_back(attachment);
  }

  Vector<VkSubpassDescription> subpasses;
  Vector<Vector<VkAttachmentReference>> colorAttachments(desc.subpasses.size());

  for (uint32 i = 0; i < desc.subpasses.size(); ++i) {
    auto& subpass = desc.subpasses[i];
    auto& colorRefs = colorAttachments[i];

    for (uint32 colorIndex: subpass.colorAttachments) {
      VkAttachmentReference colorRef{};
      colorRef.attachment = colorIndex;
      colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      colorRefs.push_back(colorRef);
    }

    VkSubpassDescription subpassDesc{};
    subpassDesc.pipelineBindPoint = 
      VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 
      static_cast<uint32>(colorRefs.size());
    subpassDesc.pColorAttachments = colorRefs.data();

    subpasses.push_back(subpassDesc);
  }

  Vector<VkSubpassDependency> dependencies;
  for (const auto& dep: desc.dependencies) {
    VkSubpassDependency dependency{};
    dependency.srcSubpass = dep.srcSubpass;
    dependency.dstSubpass = dep.dstSubpass;
    dependency.srcAccessMask = VulkanTranslator::get(dep.srcAccessMask);
    dependency.dstAccessMask = VulkanTranslator::get(dep.dstAccessMask);
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies.push_back(dependency);
  }

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32>(attachments.size());
  renderPassInfo.pAttachments = attachments.empty() ? nullptr : attachments.data();
  renderPassInfo.subpassCount = static_cast<uint32>(subpasses.size());
  renderPassInfo.pSubpasses = subpasses.empty() ? nullptr : subpasses.data();
  renderPassInfo.dependencyCount = static_cast<uint32>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.empty() ? nullptr : dependencies.data();

  throwIfFailed(vkCreateRenderPass(device,
                                   &renderPassInfo,
                                   nullptr,
                                   &m_renderPass));

  m_subPassCount = static_cast<uint32>(subpasses.size());
  m_subpasses = desc.subpasses;
}

/*
*/
bool
VulkanRenderPass::_internalIsValid() const {
  return m_renderPass != VK_NULL_HANDLE;
}

/*
*/
const chGPUDesc::SubpassDesc&
VulkanRenderPass::_internalGetSubpassDesc(uint32 index) const {
  CH_ASSERT(index < m_subpasses.size() && "Invalid subpass index.");
  return m_subpasses[index];
}

} // namespace chEngineSDK