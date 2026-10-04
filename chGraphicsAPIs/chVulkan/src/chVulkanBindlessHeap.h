/************************************************************************/
/**
 * @file chVulkanBindlessHeap.h
 * @author AccelMR
 * @date 2026/10/03
 * @brief
 * The descriptor heap every shader reads resources from.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"
#include "chSTDThreading.h"

namespace chEngineSDK {

/**
 * One descriptor set shared by every pipeline, laid out the way DXC compiles
 * ResourceDescriptorHeap (set 0, binding 0) and SamplerDescriptorHeap (set 0, binding 1).
 * Binding 0 is mutable, so one index space holds sampled images, storage images, uniform
 * and storage buffers, like a DX12 CBV/SRV/UAV heap. It also owns the single pipeline
 * layout (this set plus the push constants), which is why switching pipelines never
 * rebinds descriptors.
 */
class VulkanBindlessHeap
{
 public:
  VulkanBindlessHeap() = default;
  ~VulkanBindlessHeap();

  VulkanBindlessHeap(const VulkanBindlessHeap&) = delete;
  VulkanBindlessHeap&
  operator=(const VulkanBindlessHeap&) = delete;

  void
  initialize(VkDevice device);

  /**
   * Every index must have been freed and the device must be idle.
   */
  void
  destroy();

  NODISCARD uint32
  allocateResourceIndex();

  NODISCARD uint32
  allocateSamplerIndex();

  /**
   * Called by the deletion queue once no submitted work can read the index.
   */
  void
  freeResourceIndex(uint32 index);

  void
  freeSamplerIndex(uint32 index);

  void
  writeSampledImage(uint32 index, VkImageView view, VkImageLayout layout) const;

  void
  writeUniformBuffer(uint32 index, VkBuffer buffer, VkDeviceSize size) const;

  void
  writeStorageBuffer(uint32 index, VkBuffer buffer, VkDeviceSize size) const;

  void
  writeSampler(uint32 index, VkSampler sampler) const;

  NODISCARD FORCEINLINE VkDescriptorSet
  getDescriptorSet() const
  {
    return m_descriptorSet;
  }

  NODISCARD FORCEINLINE VkPipelineLayout
  getPipelineLayout() const
  {
    return m_pipelineLayout;
  }

 private:
  /**
   * Index allocator: reuses freed indexes before growing.
   */
  struct IndexPool
  {
    Vector<uint32> freeIndexes;
    uint32 nextIndex = 0;
    uint32 capacity = 0;
  };

  NODISCARD uint32
  allocate(IndexPool& pool, const ANSICHAR* poolName);

  void
  writeBuffer(uint32 index, VkDescriptorType type, VkBuffer buffer, VkDeviceSize size) const;

  VkDevice m_device = VK_NULL_HANDLE;
  VkDescriptorSetLayout m_setLayout = VK_NULL_HANDLE;
  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

  IndexPool m_resourceIndexes;
  IndexPool m_samplerIndexes;
  Mutex m_mutex;
};

} // namespace chEngineSDK
