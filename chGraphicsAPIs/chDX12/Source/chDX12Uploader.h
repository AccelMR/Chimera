/************************************************************************/
/**
 * @file chDX12Uploader.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Copies CPU data into GPU-only buffers and textures without waiting for the GPU.
 */
/************************************************************************/
#pragma once

#include "chDX12Prerequisites.h"

namespace chEngineSDK {
class DX12DeletionQueue;
class DX12Texture;

/**
 * Copies data to the GPU without stalling the CPU. The data goes into a mapped staging
 * ring in the upload heap and the copies are recorded into an upload command list that
 * DX12API::endFrame submits before the frame's own list, in the same call. A part of the
 * ring is used again once the frame fence shows its submit has finished; data that does
 * not fit gets a staging buffer of its own, released through the deletion queue. Main
 * thread only.
 */
class DX12Uploader
{
 public:
  DX12Uploader() = default;
  ~DX12Uploader();

  DX12Uploader(const DX12Uploader&) = delete;
  DX12Uploader&
  operator=(const DX12Uploader&) = delete;

  void
  initialize(ID3D12Device4* device,
             D3D12MA::Allocator* allocator,
             DX12DeletionQueue* deletionQueue);

  /**
   * The GPU must be idle.
   */
  void
  destroy();

  void
  uploadBuffer(ID3D12Resource* buffer, uint64 offset, const void* data, SIZE_T size);

  /**
   * Leaves the whole texture in the shader resource layout. See ITexture::uploadData for
   * the layout of the data.
   */
  void
  uploadTexture(const DX12Texture& texture, const void* data, SIZE_T size);

  /**
   * Closes the copies recorded since the last call and returns their list, which must go
   * into the submit that signals submitValue; nullptr when nothing was recorded.
   */
  NODISCARD ID3D12CommandList*
  endRecording(uint64 submitValue);

 private:
  struct StagingRegion
  {
    ID3D12Resource* buffer = nullptr;
    uint64 offset = 0;
    uint8* mappedData = nullptr;
  };

  // Part of the ring a submit uses, released once the fence reaches its value.
  struct RingSpan
  {
    uint64 end = 0;
    uint64 submitValue = 0;
  };

  struct UploadSlot
  {
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList7> commandList;
    uint64 submitValue = 0;
  };

  NODISCARD StagingRegion
  allocateStaging(uint64 size, uint64 alignment);

  NODISCARD ID3D12GraphicsCommandList7*
  getCommandList();

  void
  releaseFinishedSpans();

  // Big enough for a 2048x2048 RGBA8 texture with its mips.
  static constexpr uint64 RING_SIZE = 32ull * 1024 * 1024;
  // Covers the texel size of every uncompressed format.
  static constexpr uint64 BUFFER_STAGING_ALIGNMENT = 16;

  ID3D12Device4* m_device = nullptr;
  D3D12MA::Allocator* m_allocator = nullptr;
  DX12DeletionQueue* m_deletionQueue = nullptr;

  ComPtr<ID3D12Resource> m_ringBuffer;
  D3D12MA::Allocation* m_ringAllocation = nullptr;
  uint8* m_ringData = nullptr;
  // Offsets grow forever; the place in the ring is the offset modulo RING_SIZE.
  uint64 m_ringHead = 0;
  uint64 m_ringTail = 0;
  Vector<RingSpan> m_pendingSpans;

  // Reused by every texture upload, so they grow only for a texture with more parts than
  // any before.
  Vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> m_footprints;
  Vector<UINT> m_rowCounts;
  Vector<UINT64> m_rowSizes;

  Array<UploadSlot, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_slots;
  uint32 m_slotIndex = 0;
  bool m_isRecording = false;
};

} // namespace chEngineSDK
