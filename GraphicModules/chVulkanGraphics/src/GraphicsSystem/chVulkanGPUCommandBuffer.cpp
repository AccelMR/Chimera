/************************************************************************/
/**
 * @file chVulkanGPUCommanderBuffer.cpp
 * @author AccelMR
 * @date 2025/01/26
 *   Vulkan specific implementation for GPUCommandBuffer.
 */
 /************************************************************************/

/************************************************************************/
/*
  * Includes
  */
/************************************************************************/
#include "chVulkanGPUCommandBuffer.h"

#include "chDebug.h"
#include "chVulkanIndexBuffer.h"
#include "chVulkanFrameBuffer.h"
#include "chVulkanGPUPipelineState.h"
#include "chVulkanGPUBuffer.h"
#include "chVulkanGraphicsModule.h"
#include "chVulkanTranslator.h"
#include "chVulkanTexture.h"
#include "chVulkanRenderPass.h"
#include "chVulkanSwapChain.h"
#include "chVulkanSampler.h"
#include "chVulkanVertexBuffer.h"

namespace chEngineSDK {
/*
*/
VulkanGPUCommandBuffer::~VulkanGPUCommandBuffer() {
  if (m_commandBuffer != VK_NULL_HANDLE) {
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_commandBuffer);
  }
}

/*
*/
void
VulkanGPUCommandBuffer::_init(chGPUDesc::COMMAND_BUFFER_TYPES commandBufferType) {
  m_device = g_VulkanGraphicsModule().getDevice();
  m_commandPool = g_VulkanGraphicsModule().commandPool;

  CH_ASSERT(m_commandPool != VK_NULL_HANDLE);

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = m_commandPool;
  allocInfo.level = (commandBufferType == COMMAND_BUFFER_TYPES::kDIRECT) ? 
                    VK_COMMAND_BUFFER_LEVEL_PRIMARY : 
                    VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  allocInfo.commandBufferCount = 1;
  throwIfFailed(vkAllocateCommandBuffers(m_device, &allocInfo, &m_commandBuffer));
}

/*
*/
void
VulkanGPUCommandBuffer::_internalBegin() {
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  throwIfFailed(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));
}

/*
 * TODO: Implement flags
 */
void
VulkanGPUCommandBuffer::_internalReset(const SPtr<GPUPipelineState>& pipelineState) {
  CH_ASSERT(m_commandBuffer != VK_NULL_HANDLE);
  
  m_renderPass.reset();
  m_framebuffer.reset();

  m_descriptorSet = VK_NULL_HANDLE;
  m_pipelineState.reset();

  throwIfFailed(
    vkResetCommandBuffer(m_commandBuffer, 
                         VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

  _internalBegin();

  //CH_ASSERT(pipelineState != nullptr);
  //_internalSetPipeLineState(pipelineState);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalClose() {
  throwIfFailed(vkEndCommandBuffer(m_commandBuffer));
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetPipeLineState(const SPtr<GPUPipelineState>& pipelineState) {
  CH_ASSERT(pipelineState != nullptr);

  m_pipelineState = std::static_pointer_cast<VulkanGPUPipelineState>(pipelineState);
  VkPipeline pipeline = m_pipelineState->getPipeline();
  CH_ASSERT(pipeline != VK_NULL_HANDLE);

  m_descriptorSet = m_pipelineState->getDescriptorSet();
  CH_ASSERT(m_descriptorSet != VK_NULL_HANDLE);

  vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  const auto& bindingGroups = m_pipelineState->getBindingGroups();
  for (uint32 i = 0; i < bindingGroups.size(); ++i) {
    _internalSetBindingBufferGroup(bindingGroups[i]);
  }
}

/*
*/
void
VulkanGPUCommandBuffer::_internalBeginRenderPass(const SPtr<RenderPass>& renderPass, 
                                                 const SPtr<Framebuffer>& frameBuffer,
                                                 const Vector<LinearColor>& clearColors) {
  auto vulkanRenderPass = std::static_pointer_cast<VulkanRenderPass>(renderPass);
  CH_ASSERT(vulkanRenderPass != nullptr);
  m_renderPass = vulkanRenderPass;
  auto vulkanFramebuffer = std::static_pointer_cast<VulkanFramebuffer>(frameBuffer);
  CH_ASSERT(vulkanFramebuffer != nullptr);
  m_framebuffer = vulkanFramebuffer;

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = vulkanRenderPass->getRenderPass();
  renderPassInfo.framebuffer = vulkanFramebuffer->getFramebuffer();
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = { 
    vulkanFramebuffer->getWidth(), 
    vulkanFramebuffer->getHeight() };

  Vector<VkClearValue> clearValues(clearColors.size());
  for (size_t i = 0; i < clearColors.size(); ++i) {
    clearValues[i].color = {
      clearColors[i].r, 
      clearColors[i].g, 
      clearColors[i].b,
      clearColors[i].a
    };
  }

  renderPassInfo.clearValueCount = static_cast<uint32>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.empty() ?
                                nullptr :  
                                clearValues.data();

  vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetSubpassIndex(uint32 index) {
  CH_ASSERT(m_renderPass);
  CH_ASSERT(m_framebuffer);

  vkCmdEndRenderPass(m_commandBuffer);

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = m_renderPass->getRenderPass();
  renderPassInfo.framebuffer = m_framebuffer->getFramebuffer();
  renderPassInfo.renderArea.extent = { 
    m_framebuffer->getWidth(), 
    m_framebuffer->getHeight() 
  };
  renderPassInfo.clearValueCount = static_cast<uint32>(m_framebuffer->getAttachments().size());

  vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  for (uint32 i = 0; i < index; ++i) {
    vkCmdNextSubpass(m_commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
  }
}

/*
*/
void
VulkanGPUCommandBuffer::_internalNextSubpass() {
  vkCmdNextSubpass(m_commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalEndRenderPass() {
  vkCmdEndRenderPass(m_commandBuffer);
} 

/*
*/
void
VulkanGPUCommandBuffer::_internalSetGPUBuffer(const SPtr<GPUBuffer>& buff, uint32 slot) {
  Vector<SPtr<GPUBuffer>> buffers = {buff};
  _internalSetGPUBuffers(buffers, slot);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetGPUBuffers(const Vector<SPtr<GPUBuffer>>& buffers, uint32 slot) {
  if (buffers.empty()) {
    CH_LOG_ERROR("No buffers provided.");
    return;
  }

  auto vulkanPipelineState = std::static_pointer_cast<VulkanGPUPipelineState>(m_pipelineState);
  VkDescriptorSet descriptorSet = vulkanPipelineState->getDescriptorSet();

  Vector<VkDescriptorBufferInfo> bufferInfos(buffers.size());
  for (size_t i = 0; i < buffers.size(); ++i) {
    auto vulkanBuffer = std::static_pointer_cast<VulkanGPUBuffer>(buffers[i]);
    bufferInfos[i].buffer = vulkanBuffer->getBuffer();
    bufferInfos[i].offset = 0;
    bufferInfos[i].range = VK_WHOLE_SIZE;
  }

  // Actualizar descriptores
  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = descriptorSet;
  descriptorWrite.dstBinding = slot;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorCount = static_cast<uint32>(buffers.size());
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrite.pBufferInfo = bufferInfos.data();

  vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);

  vkCmdBindDescriptorSets(m_commandBuffer, 
                          VK_PIPELINE_BIND_POINT_GRAPHICS, 
                          vulkanPipelineState->getPipelineLayout(),
                          0, 1, &descriptorSet, 0, nullptr);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetRect(const Box2D& rect) {
  VkViewport viewport{};
  viewport.x = static_cast<float>(rect.minPoint.x);
  viewport.y = static_cast<float>(rect.minPoint.y);
  viewport.width =  static_cast<float>(rect.getSize().x);
  viewport.height = static_cast<float>(rect.getSize().y);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetScissorRect(const Box2D& rect) {
   VkViewport viewport = {};
  viewport.x = static_cast<float>(rect.minPoint.x);
  viewport.y = static_cast<float>(rect.minPoint.y);
  viewport.width = static_cast<float>(rect.getSize().x);
  viewport.height = static_cast<float>(rect.getSize().y);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetRenderTarget(const SPtr<Texture>& rt) {
  _internalSetRenderTargets({rt});
}
/*
*/
void
VulkanGPUCommandBuffer::_internalSetRenderTargets(const Vector<SPtr<Texture>>& rts) {
  if (rts.empty()) {
    CH_LOG_ERROR("No render targets provided.");
    return;
  }

  auto vulkanTexture = std::static_pointer_cast<VulkanTexture>(rts[0]);
  VkFramebuffer framebuffer = vulkanTexture->getFrameBuffer();
  CH_ASSERT(framebuffer);
  CH_ASSERT(m_renderPass);

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = m_renderPass->getRenderPass();
  renderPassInfo.framebuffer = framebuffer;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = vulkanTexture->getExtent();

  Vector<VkClearValue> clearValues(rts.size());
  for (size_t i = 0; i < rts.size(); ++i) {
    clearValues[i].color = {0.0f, 0.0f, 0.0f, 1.0f};
  }

  renderPassInfo.clearValueCount = static_cast<uint32>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalClearRenderTarget(const SPtr<Texture>& rt, 
                                                   const LinearColor& color, 
                                                   const bool bIsInRenderPass) {
  _internalClearRenderTargets({rt}, color, bIsInRenderPass);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalClearRenderTargets(const Vector<SPtr<Texture>>& rts, 
                                                    const LinearColor& color, 
                                                    const bool bIsInRenderPass) {
  if (rts.empty()) {
    CH_LOG_ERROR("No render targets provided.");
    return;
  }

  auto clearColorImage = [&](const SPtr<VulkanTexture>& vulkanTexture) {
    VkClearValue clearValue{};
    clearValue.color = {color.r, color.g, color.b, color.a};

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    vkCmdClearColorImage(m_commandBuffer, 
                         vulkanTexture->getImage(), 
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                         &clearValue.color, 
                         1, 
                         &subresourceRange);
  };

  auto  clearColorAttachment = [&](const SPtr<VulkanTexture>& vulkanTexture) {
    VkClearAttachment clearAttachment{};
    clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    clearAttachment.colorAttachment = 0;
    clearAttachment.clearValue = {color.r, color.g, color.b, color.a};

    VkClearRect clearRect{};
    clearRect.rect.offset = {0, 0};
    clearRect.rect.extent = vulkanTexture->getExtent();
    clearRect.baseArrayLayer = 0;
    clearRect.layerCount = 1;

    vkCmdClearAttachments(m_commandBuffer, 1, &clearAttachment, 1, &clearRect);
  };

  for (auto& rt : rts) {
    auto vulkanTexture = std::static_pointer_cast<VulkanTexture>(rt);
    if (bIsInRenderPass) {
      clearColorAttachment(vulkanTexture);
    }
    else {
      clearColorImage(vulkanTexture);
    }
  }
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetTopology(PRIMITIVE_TOPOLOGY_TYPE topology) {
  CH_ASSERT(m_commandBuffer != VK_NULL_HANDLE);

  if (m_pipelineState->getTopology() != PRIMITIVE_TOPOLOGY_TYPE::kDYNAMIC) {
    CH_LOG_ERROR("Pipeline satate is not set to dynamic topology, cannot set topology.");
    return;
  }

  VkPrimitiveTopology vkTopology = VulkanTranslator::get(topology);
  vkCmdSetPrimitiveTopology(m_commandBuffer, vkTopology);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetVertexBuffer(uint32 slot,
                                                 const SPtr<VertexBuffer>& buffer) {
  _internalSetVertexBuffers(slot, {buffer});
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetVertexBuffers(uint32 startSlot, 
                                                  const Vector<SPtr<VertexBuffer>>& buffers) {
  CH_ASSERT(m_commandBuffer != VK_NULL_HANDLE);
  CH_ASSERT(!buffers.empty() && "Vertex buffers are empty.");

  Vector<VkBuffer> vertexBuffers(buffers.size());
  Vector<VkDeviceSize> offsets(buffers.size());
  for (size_t i = 0; i < buffers.size(); ++i) {
    auto vulkanVertexBuffer = std::static_pointer_cast<VulkanVertexBuffer>(buffers[i]);
    vertexBuffers[i] = vulkanVertexBuffer->getBuffer();
    offsets[i] = 0;
  }

  vkCmdBindVertexBuffers(m_commandBuffer, startSlot, 
                         static_cast<uint32>(vertexBuffers.size()), 
                         vertexBuffers.data(), offsets.data());
}

/*
*/
void
VulkanGPUCommandBuffer::_internalSetIndexBuffer(const SPtr<IndexBuffer>& buffer) {
  SPtr<VulkanIndexBuffer> vulkanIndexBuff = 
    std::reinterpret_pointer_cast<VulkanIndexBuffer>(buffer);
  VkBuffer indexBuffer = vulkanIndexBuff->getBuffer();
  vkCmdBindIndexBuffer(m_commandBuffer, indexBuffer, 0, vulkanIndexBuff->getIndexType());
}

/*
 */
void
VulkanGPUCommandBuffer::_internalSetBindingBufferGroup(const chGPUDesc::BindingGroup& bindingGroup) {
  VkDescriptorSet descriptorSet = m_pipelineState->getDescriptorSet(bindingGroup.descriptorSetIndex);
  CH_ASSERT(descriptorSet != VK_NULL_HANDLE);

  Vector<VkWriteDescriptorSet> descriptorWrites;
  Vector<VkDescriptorBufferInfo> bufferInfos;
  Vector<VkDescriptorImageInfo> imageInfos;

  descriptorWrites.reserve(bindingGroup.bindings.size());
  bufferInfos.reserve(bindingGroup.bindings.size());
  imageInfos.reserve(bindingGroup.bindings.size());

  for (const auto& binding : bindingGroup.bindings) {
    VkWriteDescriptorSet write = createDescriptorWrite(binding, descriptorSet, 
                                                      bufferInfos, imageInfos);
    descriptorWrites.push_back(write);
  }

  // Actualizar descriptores
  if (!descriptorWrites.empty()) {
    vkUpdateDescriptorSets(m_device, 
                          static_cast<uint32>(descriptorWrites.size()), 
                          descriptorWrites.data(), 
                          0, nullptr);
  }

  // Vincular el descriptor set actualizado
  vkCmdBindDescriptorSets(m_commandBuffer,
                         VK_PIPELINE_BIND_POINT_GRAPHICS,
                         m_pipelineState->getPipelineLayout(),
                         bindingGroup.descriptorSetIndex,
                         1, &descriptorSet,
                         0, nullptr);
}

/*
*/
VkWriteDescriptorSet 
VulkanGPUCommandBuffer::createDescriptorWrite(const DescriptorBinding& binding,
                                              VkDescriptorSet descriptorSet,
                                              Vector<VkDescriptorBufferInfo>& bufferInfos,
                                              Vector<VkDescriptorImageInfo>& imageInfos) {
  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet = descriptorSet;
  write.dstBinding = binding.slot;
  write.dstArrayElement = 0;
  write.descriptorCount = 1;

  if (std::holds_alternative<SPtr<GPUBuffer>>(binding.resource)) {
    auto buffer = std::get<SPtr<GPUBuffer>>(binding.resource);
    auto vulkanBuffer = std::static_pointer_cast<VulkanGPUBuffer>(buffer);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = vulkanBuffer->getBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;
    bufferInfos.push_back(bufferInfo);

    write.descriptorType = binding.type == DescriptorBinding::TYPE::kUNIFORM_BUFFER ?
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo = &bufferInfos.back();
  }
  else if (std::holds_alternative<SPtr<Texture>>(binding.resource)) {
    auto texture = std::get<SPtr<Texture>>(binding.resource);
    auto vulkanTexture = std::static_pointer_cast<VulkanTexture>(texture);

    VkDescriptorImageInfo imageInfo{};

    if (binding.type == DescriptorBinding::TYPE::kSAMPLED_TEXTURE) {
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    else if (binding.type == DescriptorBinding::TYPE::kSTORAGE_TEXTURE) {
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }
    else if (binding.type == DescriptorBinding::TYPE::kSAMPLER) {
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    else if (binding.type == DescriptorBinding::TYPE::kINPUT_ATTACHMENT) {
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    else {
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    
    imageInfo.imageView = vulkanTexture->getImageView();
    imageInfos.push_back(imageInfo);

    switch (binding.type) {
      case DescriptorBinding::TYPE::kSAMPLED_TEXTURE:
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        break;
      case DescriptorBinding::TYPE::kSTORAGE_TEXTURE:
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        break;
      case DescriptorBinding::TYPE::kINPUT_ATTACHMENT:
        write.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        break;
      default:
        CH_LOG_ERROR("Unsupported texture binding type");
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        break;
    }
    
    write.pImageInfo = &imageInfos.back();
  }
  else if (std::holds_alternative<SPtr<Sampler>>(binding.resource)) {
    auto sampler = std::get<SPtr<Sampler>>(binding.resource);
    auto vulkanSampler = std::static_pointer_cast<VulkanSampler>(sampler);

    VkDescriptorImageInfo samplerInfo{};
    samplerInfo.sampler = vulkanSampler->getSampler();
    imageInfos.push_back(samplerInfo);

    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    write.pImageInfo = &imageInfos.back();
  }

  return write;
}

/*
*/
void
VulkanGPUCommandBuffer::_internalDrawIndexed(uint32 indexCountPerInstance,
                                             uint32 instanceCount,
                                             uint32 startIndexLocation,
                                             int32 baseVertexLocation,
                                             uint32 startIstanceLocation) {
  vkCmdDrawIndexed(m_commandBuffer, 
                   indexCountPerInstance, 
                   instanceCount, 
                   startIndexLocation, 
                   baseVertexLocation, 
                   startIstanceLocation);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalDrawInstanced(uint32 vertexCountPerInstance,
                                      uint32 instanceCount,
                                      uint32 startVertexLocation,
                                      uint32 startInstanceLocation) {
  vkCmdDraw(m_commandBuffer, 
            vertexCountPerInstance, 
            instanceCount, 
            startVertexLocation, 
            startInstanceLocation);
}

/*
*/
void
VulkanGPUCommandBuffer::_internalResourceBarrier(const Vector<chGPUDesc::GPUBarrier>& barriers) {
  Vector<VkImageMemoryBarrier> imageBarriers;

  for (const auto& barrier : barriers) {
    if (barrier.transition.resource) {
      // Intenta convertir directamente a VulkanTexture, asumiendo que sabes que es una textura
      auto vulkanTexture = std::static_pointer_cast<VulkanTexture>(barrier.transition.resource);
    
      VkImageMemoryBarrier imgBarrier{};
      imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      imgBarrier.oldLayout = VulkanTranslator::get(barrier.transition.stateBefore);
      imgBarrier.newLayout = VulkanTranslator::get(barrier.transition.stateAfter);
      imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imgBarrier.image = vulkanTexture->getImage();
      
      // Determinar aspectMask basado en el formato
      VkFormat format = vulkanTexture->getFormat();
      if (format == VK_FORMAT_D32_SFLOAT || 
          format == VK_FORMAT_D32_SFLOAT_S8_UINT || 
          format == VK_FORMAT_D24_UNORM_S8_UINT) {
        imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || 
          format == VK_FORMAT_D24_UNORM_S8_UINT) {
          imgBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
      }
      else {
        imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      }
      
      imgBarrier.subresourceRange.baseMipLevel = 0;
      imgBarrier.subresourceRange.levelCount = 1;
      imgBarrier.subresourceRange.baseArrayLayer = 0;
      imgBarrier.subresourceRange.layerCount = 1;
      
      // Configurar access masks
      if (barrier.transition.stateBefore == ResourceStates::kRENDER_TARGET) {
        imgBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      } 
      else if (barrier.transition.stateBefore == ResourceStates::kSHADER_RESOURCE) {
        imgBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      } 
      else if (barrier.transition.stateBefore == ResourceStates::kPRESENT) {
        imgBarrier.srcAccessMask = 0;
      }
      
      if (barrier.transition.stateAfter == ResourceStates::kRENDER_TARGET) {
        imgBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      } 
      else if (barrier.transition.stateAfter == ResourceStates::kSHADER_RESOURCE) {
        imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      } 
      else if (barrier.transition.stateAfter == ResourceStates::kPRESENT) {
        imgBarrier.dstAccessMask = 0;
      }
      
      imageBarriers.push_back(imgBarrier);
    }
  }
  
  if (!imageBarriers.empty()) {
    vkCmdPipelineBarrier(m_commandBuffer,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data());
  }
}

/*
*/
void
VulkanGPUCommandBuffer::_internalResourceBarrierSwapChain(const chGPUDesc::GPUBarrier& barriers) {
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VulkanTranslator::get(barriers.transition.stateBefore);
  barrier.newLayout = VulkanTranslator::get(barriers.transition.stateAfter);
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  auto swapChain = std::reinterpret_pointer_cast<VulkanSwapChain>(g_VulkanGraphicsModule().getSwapChain());
  CH_ASSERT(swapChain);
  CH_ASSERT(swapChain->getSwapChainImage() != VK_NULL_HANDLE);
  barrier.image = swapChain->getSwapChainImage();
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage = 
    VulkanTranslator::getPipelineStageFlags(barriers.transition.stateBefore);
  VkPipelineStageFlags destinationStage = 
    VulkanTranslator::getPipelineStageFlags(barriers.transition.stateAfter);

  vkCmdPipelineBarrier(m_commandBuffer, 
                       sourceStage, 
                       destinationStage, 
                       0, 
                       0, 
                       nullptr, 
                       0, 
                       nullptr, 
                       1, 
                       &barrier);
}

/*
*/
bool
VulkanGPUCommandBuffer::_internalPresent(int32 syncInterval, int32 flags) {
  GraphicsModuleVulkan& rhi = g_VulkanGraphicsModule();
  auto swapChain = std::reinterpret_pointer_cast<VulkanSwapChain>(rhi.getSwapChain());
  return swapChain->present(syncInterval, flags, this);
}
} // namespace chEngineSDK