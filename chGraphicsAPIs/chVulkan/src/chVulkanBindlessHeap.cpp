/************************************************************************/
/**
 * @file chVulkanBindlessHeap.cpp
 * @author AccelMR
 * @date 2026/10/03
 * @brief
 * The descriptor heap every shader reads resources from.
 */
/************************************************************************/
#include "chVulkanBindlessHeap.h"

namespace chEngineSDK {
namespace {
// Where DXC puts ResourceDescriptorHeap and SamplerDescriptorHeap by default.
constexpr uint32 kResourceBinding = 0;
constexpr uint32 kSamplerBinding = 1;

constexpr Array<VkDescriptorType, 4> kResourceDescriptorTypes = {
    VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};
} // namespace

/*
 */
VulkanBindlessHeap::~VulkanBindlessHeap()
{
  CH_ASSERT(m_device == VK_NULL_HANDLE);
}

/*
 */
void
VulkanBindlessHeap::initialize(VkDevice device)
{
  m_device = device;
  m_resourceIndexes.capacity = GraphicsLimits::MAX_BINDLESS_RESOURCES;
  m_samplerIndexes.capacity = GraphicsLimits::MAX_BINDLESS_SAMPLERS;

  const Array<VkMutableDescriptorTypeListEXT, 2> mutableTypeLists = {
      VkMutableDescriptorTypeListEXT{
          .descriptorTypeCount = static_cast<uint32>(kResourceDescriptorTypes.size()),
          .pDescriptorTypes = kResourceDescriptorTypes.data()},
      VkMutableDescriptorTypeListEXT{.descriptorTypeCount = 0, .pDescriptorTypes = nullptr}};

  const VkMutableDescriptorTypeCreateInfoEXT mutableInfo{
      .sType = VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT,
      .pNext = nullptr,
      .mutableDescriptorTypeListCount = static_cast<uint32>(mutableTypeLists.size()),
      .pMutableDescriptorTypeLists = mutableTypeLists.data()};

  // Partially bound: most indexes are empty. Update after bind and while pending: new
  // resources are written while earlier frames that use the set are still running.
  constexpr VkDescriptorBindingFlags bindingFlags =
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
      VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
  const Array<VkDescriptorBindingFlags, 2> flags = {bindingFlags, bindingFlags};

  const VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
      .pNext = &mutableInfo,
      .bindingCount = static_cast<uint32>(flags.size()),
      .pBindingFlags = flags.data()};

  const Array<VkDescriptorSetLayoutBinding, 2> bindings = {
      VkDescriptorSetLayoutBinding{.binding = kResourceBinding,
                                   .descriptorType = VK_DESCRIPTOR_TYPE_MUTABLE_EXT,
                                   .descriptorCount = m_resourceIndexes.capacity,
                                   .stageFlags = VK_SHADER_STAGE_ALL,
                                   .pImmutableSamplers = nullptr},
      VkDescriptorSetLayoutBinding{.binding = kSamplerBinding,
                                   .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                                   .descriptorCount = m_samplerIndexes.capacity,
                                   .stageFlags = VK_SHADER_STAGE_ALL,
                                   .pImmutableSamplers = nullptr}};

  const VkDescriptorSetLayoutCreateInfo layoutInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = &flagsInfo,
      .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
      .bindingCount = static_cast<uint32>(bindings.size()),
      .pBindings = bindings.data()};
  VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_setLayout));

  const Array<VkDescriptorPoolSize, 2> poolSizes = {
      VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_MUTABLE_EXT,
                           .descriptorCount = m_resourceIndexes.capacity},
      VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_SAMPLER,
                           .descriptorCount = m_samplerIndexes.capacity}};

  // The pool must also list the types a mutable descriptor may hold.
  const VkMutableDescriptorTypeCreateInfoEXT poolMutableInfo{
      .sType = VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT,
      .pNext = nullptr,
      .mutableDescriptorTypeListCount = static_cast<uint32>(mutableTypeLists.size()),
      .pMutableDescriptorTypeLists = mutableTypeLists.data()};

  const VkDescriptorPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = &poolMutableInfo,
      .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
      .maxSets = 1,
      .poolSizeCount = static_cast<uint32>(poolSizes.size()),
      .pPoolSizes = poolSizes.data()};
  VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool));

  const VkDescriptorSetAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
      .descriptorPool = m_descriptorPool,
      .descriptorSetCount = 1,
      .pSetLayouts = &m_setLayout};
  VK_CHECK(vkAllocateDescriptorSets(m_device, &allocateInfo, &m_descriptorSet));

  const VkPushConstantRange pushConstants{.stageFlags = VK_SHADER_STAGE_ALL,
                                          .offset = 0,
                                          .size = GraphicsLimits::PUSH_CONSTANTS_SIZE};
  const VkPipelineLayoutCreateInfo pipelineLayoutInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .setLayoutCount = 1,
      .pSetLayouts = &m_setLayout,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &pushConstants};
  VK_CHECK(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));
}

/*
 */
void
VulkanBindlessHeap::destroy()
{
  if (m_device == VK_NULL_HANDLE) {
    return;
  }

  CH_ASSERT(m_resourceIndexes.freeIndexes.size() == m_resourceIndexes.nextIndex &&
            "Bindless resource indexes were not freed");
  CH_ASSERT(m_samplerIndexes.freeIndexes.size() == m_samplerIndexes.nextIndex &&
            "Bindless sampler indexes were not freed");

  vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
  vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_setLayout, nullptr);
  m_pipelineLayout = VK_NULL_HANDLE;
  m_descriptorPool = VK_NULL_HANDLE;
  m_descriptorSet = VK_NULL_HANDLE;
  m_setLayout = VK_NULL_HANDLE;
  m_device = VK_NULL_HANDLE;
}

/*
 */
uint32
VulkanBindlessHeap::allocateResourceIndex()
{
  return allocate(m_resourceIndexes, "resource");
}

/*
 */
uint32
VulkanBindlessHeap::allocateSamplerIndex()
{
  return allocate(m_samplerIndexes, "sampler");
}

/*
 */
void
VulkanBindlessHeap::freeResourceIndex(uint32 index)
{
  LockGuard<Mutex> lock(m_mutex);
  m_resourceIndexes.freeIndexes.push_back(index);
}

/*
 */
void
VulkanBindlessHeap::freeSamplerIndex(uint32 index)
{
  LockGuard<Mutex> lock(m_mutex);
  m_samplerIndexes.freeIndexes.push_back(index);
}

/*
 */
uint32
VulkanBindlessHeap::allocate(IndexPool& pool, const ANSICHAR* poolName)
{
  LockGuard<Mutex> lock(m_mutex);
  if (!pool.freeIndexes.empty()) {
    const uint32 index = pool.freeIndexes.back();
    pool.freeIndexes.pop_back();
    return index;
  }

  if (pool.nextIndex == pool.capacity) {
    CH_EXCEPT(VulkanErrorException,
              StringUtils::format("The bindless {0} heap is full ({1} indexes)", poolName,
                                  pool.capacity));
  }
  return pool.nextIndex++;
}

/*
 */
void
VulkanBindlessHeap::writeSampledImage(uint32 index, VkImageView view, VkImageLayout layout) const
{
  const VkDescriptorImageInfo imageInfo{.sampler = VK_NULL_HANDLE,
                                        .imageView = view,
                                        .imageLayout = layout};
  const VkWriteDescriptorSet write{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                   .pNext = nullptr,
                                   .dstSet = m_descriptorSet,
                                   .dstBinding = kResourceBinding,
                                   .dstArrayElement = index,
                                   .descriptorCount = 1,
                                   .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                   .pImageInfo = &imageInfo,
                                   .pBufferInfo = nullptr,
                                   .pTexelBufferView = nullptr};
  vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
}

/*
 */
void
VulkanBindlessHeap::writeUniformBuffer(uint32 index, VkBuffer buffer, VkDeviceSize size) const
{
  writeBuffer(index, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffer, size);
}

/*
 */
void
VulkanBindlessHeap::writeStorageBuffer(uint32 index, VkBuffer buffer, VkDeviceSize size) const
{
  writeBuffer(index, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, buffer, size);
}

/*
 */
void
VulkanBindlessHeap::writeBuffer(uint32 index,
                                VkDescriptorType type,
                                VkBuffer buffer,
                                VkDeviceSize size) const
{
  const VkDescriptorBufferInfo bufferInfo{.buffer = buffer, .offset = 0, .range = size};
  const VkWriteDescriptorSet write{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                   .pNext = nullptr,
                                   .dstSet = m_descriptorSet,
                                   .dstBinding = kResourceBinding,
                                   .dstArrayElement = index,
                                   .descriptorCount = 1,
                                   .descriptorType = type,
                                   .pImageInfo = nullptr,
                                   .pBufferInfo = &bufferInfo,
                                   .pTexelBufferView = nullptr};
  vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
}

/*
 */
void
VulkanBindlessHeap::writeSampler(uint32 index, VkSampler sampler) const
{
  const VkDescriptorImageInfo imageInfo{.sampler = sampler,
                                        .imageView = VK_NULL_HANDLE,
                                        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED};
  const VkWriteDescriptorSet write{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                   .pNext = nullptr,
                                   .dstSet = m_descriptorSet,
                                   .dstBinding = kSamplerBinding,
                                   .dstArrayElement = index,
                                   .descriptorCount = 1,
                                   .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                                   .pImageInfo = &imageInfo,
                                   .pBufferInfo = nullptr,
                                   .pTexelBufferView = nullptr};
  vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
}

} // namespace chEngineSDK
