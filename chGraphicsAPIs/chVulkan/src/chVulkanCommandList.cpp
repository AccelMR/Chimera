/************************************************************************/
/**
 * @file chVulkanCommandList.cpp
 * @author AccelMR
 * @date 2025/04/09
 * @brief
 * Vulkan implementation of ICommandList.
 */
/************************************************************************/
#include "chVulkanCommandList.h"

#include "chVulkanAPI.h"
#include "chVulkanBuffer.h"
#include "chVulkanPipeline.h"
#include "chVulkanTexture.h"
#include "chVulkanTextureView.h"

namespace chEngineSDK {
namespace {
struct VulkanResourceState
{
  VkPipelineStageFlags2 stages = VK_PIPELINE_STAGE_2_NONE;
  VkAccessFlags2 access = VK_ACCESS_2_NONE;
  VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
};

constexpr VkPipelineStageFlags2 kShaderStages = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                                                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                                                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
constexpr VkPipelineStageFlags2 kDepthTestStages =
    VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

// Barriers recorded per vkCmdPipelineBarrier2 call; larger batches are split.
constexpr uint32 kMaxBarriersPerCall = 16;

VulkanResourceState
toVulkanState(ResourceState state, bool isDepth)
{
  // The depth-stencil layouts also hold depth-only formats, without needing the
  // separateDepthStencilLayouts feature.
  const VkImageLayout readOnlyLayout = isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                                               : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  switch (state) {
  case ResourceState::RenderTarget:
    return {VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  case ResourceState::DepthWrite:
    return {kDepthTestStages,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
  case ResourceState::DepthRead:
    return {kDepthTestStages | kShaderStages,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
  case ResourceState::ShaderRead:
    return {kShaderStages, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, readOnlyLayout};
  case ResourceState::UnorderedAccess:
    return {kShaderStages,
            VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
            VK_IMAGE_LAYOUT_GENERAL};
  case ResourceState::CopySource:
    return {VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL};
  case ResourceState::CopyDestination:
    return {VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL};
  case ResourceState::Present:
    return {VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};
  case ResourceState::Undefined:
  default:
    return {VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED};
  }
}

VkAttachmentLoadOp
toVkLoadOp(LoadOp loadOp)
{
  switch (loadOp) {
  case LoadOp::Load:
    return VK_ATTACHMENT_LOAD_OP_LOAD;
  case LoadOp::Clear:
    return VK_ATTACHMENT_LOAD_OP_CLEAR;
  case LoadOp::DontCare:
  default:
    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  }
}

VkAttachmentStoreOp
toVkStoreOp(StoreOp storeOp)
{
  return storeOp == StoreOp::Store ? VK_ATTACHMENT_STORE_OP_STORE
                                   : VK_ATTACHMENT_STORE_OP_DONT_CARE;
}
} // namespace

/*
 */
VulkanCommandList::VulkanCommandList(VkDevice device, VkCommandPool commandPool)
  : m_device(device),
    m_commandPool(commandPool)
{
  const VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1};
  VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &m_commandBuffer));

  const VulkanBindlessHeap& bindlessHeap = g_vulkanAPI().getBindlessHeap();
  m_pipelineLayout = bindlessHeap.getPipelineLayout();
  m_bindlessSet = bindlessHeap.getDescriptorSet();
}

/*
 */
VulkanCommandList::~VulkanCommandList()
{
  if (m_commandBuffer != VK_NULL_HANDLE) {
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_commandBuffer);
    m_commandBuffer = VK_NULL_HANDLE;
  }
}

/*
 */
void
VulkanCommandList::begin()
{
  const VkCommandBufferBeginInfo beginInfo{.sType =
                                               VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                           .pNext = nullptr,
                                           .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                                           .pInheritanceInfo = nullptr};
  VK_CHECK(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));

  // Every pipeline uses the same layout, so the heap stays bound for the whole buffer.
  vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
                          0, 1, &m_bindlessSet, 0, nullptr);
  vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0,
                          1, &m_bindlessSet, 0, nullptr);
}

/*
 */
void
VulkanCommandList::end()
{
  VK_CHECK(vkEndCommandBuffer(m_commandBuffer));
}

/*
 */
void
VulkanCommandList::beginRendering(const RenderingDesc& desc)
{
  CH_ASSERT(desc.colorAttachmentCount <= GraphicsLimits::MAX_COLOR_ATTACHMENTS);

  Array<VkRenderingAttachmentInfo, GraphicsLimits::MAX_COLOR_ATTACHMENTS> colorInfos{};
  for (uint32 i = 0; i < desc.colorAttachmentCount; ++i) {
    const ColorAttachment& attachment = desc.colorAttachments[i];
    const auto* view = static_cast<const VulkanTextureView*>(attachment.view);
    CH_ASSERT(view);
    colorInfos[i] = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                     .pNext = nullptr,
                     .imageView = view->getHandle(),
                     .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     .resolveMode = VK_RESOLVE_MODE_NONE,
                     .resolveImageView = VK_NULL_HANDLE,
                     .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                     .loadOp = toVkLoadOp(attachment.loadOp),
                     .storeOp = toVkStoreOp(attachment.storeOp),
                     .clearValue = {.color = {{attachment.clearColor.r, attachment.clearColor.g,
                                               attachment.clearColor.b,
                                               attachment.clearColor.a}}}};
  }

  VkRenderingAttachmentInfo depthInfo{};
  const DepthAttachment& depth = desc.depthAttachment;
  if (depth.view) {
    const auto* view = static_cast<const VulkanTextureView*>(depth.view);
    depthInfo = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                 .pNext = nullptr,
                 .imageView = view->getHandle(),
                 .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                 .resolveMode = VK_RESOLVE_MODE_NONE,
                 .resolveImageView = VK_NULL_HANDLE,
                 .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                 .loadOp = toVkLoadOp(depth.loadOp),
                 .storeOp = toVkStoreOp(depth.storeOp),
                 .clearValue = {.depthStencil = {depth.clearDepth, 0}}};
  }

  const VkRenderingInfo renderingInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .pNext = nullptr,
      .flags = 0,
      .renderArea = {.offset = {0, 0}, .extent = {desc.width, desc.height}},
      .layerCount = 1,
      .viewMask = 0,
      .colorAttachmentCount = desc.colorAttachmentCount,
      .pColorAttachments = colorInfos.data(),
      .pDepthAttachment = depth.view ? &depthInfo : nullptr,
      .pStencilAttachment = nullptr};
  vkCmdBeginRendering(m_commandBuffer, &renderingInfo);
}

/*
 */
void
VulkanCommandList::endRendering()
{
  vkCmdEndRendering(m_commandBuffer);
}

/*
 */
void
VulkanCommandList::barrier(Span<const TextureBarrier> textureBarriers)
{
  Array<VkImageMemoryBarrier2, kMaxBarriersPerCall> imageBarriers{};
  uint32 count = 0;

  auto flush = [&]() {
    if (count == 0) {
      return;
    }
    const VkDependencyInfo dependencyInfo{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                          .pNext = nullptr,
                                          .dependencyFlags = 0,
                                          .memoryBarrierCount = 0,
                                          .pMemoryBarriers = nullptr,
                                          .bufferMemoryBarrierCount = 0,
                                          .pBufferMemoryBarriers = nullptr,
                                          .imageMemoryBarrierCount = count,
                                          .pImageMemoryBarriers = imageBarriers.data()};
    vkCmdPipelineBarrier2(m_commandBuffer, &dependencyInfo);
    count = 0;
  };

  for (const TextureBarrier& textureBarrier : textureBarriers) {
    const auto* texture = static_cast<const VulkanTexture*>(textureBarrier.texture);
    CH_ASSERT(texture);
    const Format format = texture->getFormat();
    const bool isDepth = FormatUtils::isDepth(format);
    VulkanResourceState before = toVulkanState(textureBarrier.before, isDepth);
    const VulkanResourceState after = toVulkanState(textureBarrier.after, isDepth);
    // Contents are dropped, but the layout change must still run after earlier work in the
    // destination stages: the last frame's use of the same target, or the wait for a swap
    // chain image, which happens at the color output stage.
    if (textureBarrier.before == ResourceState::Undefined) {
      before.stages = after.stages;
    }

    imageBarriers[count++] = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = before.stages,
        .srcAccessMask = before.access,
        .dstStageMask = after.stages,
        .dstAccessMask = after.access,
        .oldLayout = before.layout,
        .newLayout = after.layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = texture->getHandle(),
        .subresourceRange = {.aspectMask = getImageAspects(format),
                             .baseMipLevel = 0,
                             .levelCount = VK_REMAINING_MIP_LEVELS,
                             .baseArrayLayer = 0,
                             .layerCount = VK_REMAINING_ARRAY_LAYERS}};

    if (count == kMaxBarriersPerCall) {
      flush();
    }
  }
  flush();
}

/*
 */
void
VulkanCommandList::bindPipeline(const IPipeline& pipeline)
{
  vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    static_cast<const VulkanPipeline&>(pipeline).getHandle());
}

/*
 */
void
VulkanCommandList::pushConstants(const void* data, uint32 size, uint32 offset)
{
  CH_ASSERT(offset + size <= GraphicsLimits::PUSH_CONSTANTS_SIZE);
  vkCmdPushConstants(m_commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_ALL, offset, size,
                     data);
}

/*
 */
void
VulkanCommandList::bindVertexBuffer(const IBuffer& buffer, uint32 binding, uint64 offset)
{
  const VkBuffer vkBuffer = static_cast<const VulkanBuffer&>(buffer).getHandle();
  vkCmdBindVertexBuffers(m_commandBuffer, binding, 1, &vkBuffer, &offset);
}

/*
 */
void
VulkanCommandList::bindIndexBuffer(const IBuffer& buffer, IndexType indexType, uint64 offset)
{
  const VkIndexType vkIndexType =
      indexType == IndexType::UInt16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
  vkCmdBindIndexBuffer(m_commandBuffer, static_cast<const VulkanBuffer&>(buffer).getHandle(),
                       offset, vkIndexType);
}

/*
 */
void
VulkanCommandList::draw(uint32 vertexCount,
                          uint32 instanceCount,
                          uint32 firstVertex,
                          uint32 firstInstance)
{
  vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

/*
 */
void
VulkanCommandList::drawIndexed(uint32 indexCount,
                                 uint32 instanceCount,
                                 uint32 firstIndex,
                                 int32 vertexOffset,
                                 uint32 firstInstance)
{
  vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset,
                   firstInstance);
}

/*
 */
void
VulkanCommandList::setViewport(float x,
                                 float y,
                                 float width,
                                 float height,
                                 float minDepth,
                                 float maxDepth)
{
  // The engine projects into Direct3D's clip space (Y up) and Vulkan's Y points down, so
  // a negative height starting at the bottom edge flips it back (core since Vulkan 1.1).
  const VkViewport viewport{.x = x,
                            .y = y + height,
                            .width = width,
                            .height = -height,
                            .minDepth = minDepth,
                            .maxDepth = maxDepth};
  vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
}

/*
 */
void
VulkanCommandList::setScissor(uint32 x, uint32 y, uint32 width, uint32 height)
{
  const VkRect2D scissor{.offset = {static_cast<int32>(x), static_cast<int32>(y)},
                         .extent = {width, height}};
  vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
}

} // namespace chEngineSDK
