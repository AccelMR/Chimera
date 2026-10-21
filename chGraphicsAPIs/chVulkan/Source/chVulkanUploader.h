/************************************************************************/
/**
 * @file chVulkanUploader.h
 * @author AccelMR
 * @date 2026/10/04
 * @brief
 * Copies CPU data into GPU-only buffers and textures without waiting for the GPU.
 */
/************************************************************************/
#pragma once

#include "chVulkanPrerequisites.h"

namespace chEngineSDK {
class VulkanDeletionQueue;
class VulkanTexture;

/**
 * Copies data to the GPU without stalling the CPU. The data goes into a mapped staging
 * ring and the copies are recorded into an upload command buffer that VulkanAPI::endFrame
 * submits before the frame's own commands, in the same submit. A part of the ring is used
 * again once the deletion queue timeline shows its submit has finished; data that does not
 * fit gets a staging buffer of its own, freed through the deletion queue. Main thread only.
 */
class VulkanUploader
{
 public:
  VulkanUploader() = default;
  ~VulkanUploader();

  VulkanUploader(const VulkanUploader&) = delete;
  VulkanUploader&
  operator=(const VulkanUploader&) = delete;

  void
  initialize(VkDevice device,
             VmaAllocator allocator,
             uint32 queueFamilyIndex,
             VulkanDeletionQueue* deletionQueue);

  /**
   * The device must be idle.
   */
  void
  destroy();

  void
  uploadBuffer(VkBuffer buffer, uint64 offset, const void* data, SIZE_T size);

  /**
   * Leaves the whole texture in SHADER_READ_ONLY_OPTIMAL. See ITexture::uploadData for the
   * layout of the data.
   */
  void
  uploadTexture(const VulkanTexture& texture, const void* data, SIZE_T size);

  /**
   * Ends the copies recorded since the last call and returns their command buffer, which
   * must go into the submit that signals submitValue; VK_NULL_HANDLE when nothing was
   * recorded.
   */
  NODISCARD VkCommandBuffer
  endRecording(uint64 submitValue);

 private:
  struct StagingRegion
  {
    VkBuffer buffer = VK_NULL_HANDLE;
    uint64 offset = 0;
    void* mappedData = nullptr;
    VmaAllocation allocation = nullptr;
  };

  // Part of the ring a submit uses, released once the timeline reaches its value.
  struct RingSpan
  {
    uint64 end = 0;
    uint64 submitValue = 0;
  };

  struct UploadSlot
  {
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    uint64 submitValue = 0;
  };

  NODISCARD StagingRegion
  allocateStaging(SIZE_T size);

  NODISCARD VkCommandBuffer
  getCommandBuffer();

  void
  releaseFinishedSpans();

  // Big enough for a 2048x2048 RGBA8 texture with its mips.
  static constexpr uint64 RING_SIZE = 32ull * 1024 * 1024;
  // Covers the texel size of every uncompressed format and the 4 bytes depth copies need.
  static constexpr uint64 STAGING_ALIGNMENT = 16;

  VkDevice m_device = VK_NULL_HANDLE;
  VmaAllocator m_allocator = nullptr;
  VulkanDeletionQueue* m_deletionQueue = nullptr;

  VkBuffer m_ringBuffer = VK_NULL_HANDLE;
  VmaAllocation m_ringAllocation = nullptr;
  uint8* m_ringData = nullptr;
  // Offsets grow forever; the place in the ring is the offset modulo RING_SIZE.
  uint64 m_ringHead = 0;
  uint64 m_ringTail = 0;
  Vector<RingSpan> m_pendingSpans;

  Array<UploadSlot, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_slots;
  uint32 m_slotIndex = 0;
  bool m_isRecording = false;
};

} // namespace chEngineSDK
