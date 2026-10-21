/************************************************************************/
/**
 * @file chDX12Uploader.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Copies CPU data into GPU-only buffers and textures without waiting for the GPU.
 */
/************************************************************************/
#include "chDX12Uploader.h"

#include <cstring>

#include <D3D12MemAlloc.h>

#include "chDX12API.h"
#include "chDX12DeletionQueue.h"
#include "chDX12Texture.h"
#include "chMath.h"

namespace chEngineSDK {
namespace {
NODISCARD D3D12_RESOURCE_DESC1
getStagingDesc(uint64 size)
{
  return {.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
          .Alignment = 0,
          .Width = size,
          .Height = 1,
          .DepthOrArraySize = 1,
          .MipLevels = 1,
          .Format = DXGI_FORMAT_UNKNOWN,
          .SampleDesc = {.Count = 1, .Quality = 0},
          .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
          .Flags = D3D12_RESOURCE_FLAG_NONE,
          .SamplerFeedbackMipRegion = {}};
}

void
recordGlobalBarrier(ID3D12GraphicsCommandList7* commandList,
                    D3D12_BARRIER_SYNC syncBefore,
                    D3D12_BARRIER_SYNC syncAfter,
                    D3D12_BARRIER_ACCESS accessBefore,
                    D3D12_BARRIER_ACCESS accessAfter)
{
  const D3D12_GLOBAL_BARRIER barrier{.SyncBefore = syncBefore,
                                     .SyncAfter = syncAfter,
                                     .AccessBefore = accessBefore,
                                     .AccessAfter = accessAfter};
  D3D12_BARRIER_GROUP group{};
  group.Type = D3D12_BARRIER_TYPE_GLOBAL;
  group.NumBarriers = 1;
  group.pGlobalBarriers = &barrier;
  commandList->Barrier(1, &group);
}

void
recordTextureBarrier(ID3D12GraphicsCommandList7* commandList,
                     ID3D12Resource* texture,
                     D3D12_BARRIER_SYNC syncBefore,
                     D3D12_BARRIER_SYNC syncAfter,
                     D3D12_BARRIER_ACCESS accessBefore,
                     D3D12_BARRIER_ACCESS accessAfter,
                     D3D12_BARRIER_LAYOUT layoutBefore,
                     D3D12_BARRIER_LAYOUT layoutAfter)
{
  const D3D12_TEXTURE_BARRIER barrier{
      .SyncBefore = syncBefore,
      .SyncAfter = syncAfter,
      .AccessBefore = accessBefore,
      .AccessAfter = accessAfter,
      .LayoutBefore = layoutBefore,
      .LayoutAfter = layoutAfter,
      .pResource = texture,
      // 0xFFFFFFFF in the first index covers every mip, layer and plane.
      .Subresources = {.IndexOrFirstMipLevel = 0xFFFFFFFF,
                       .NumMipLevels = 0,
                       .FirstArraySlice = 0,
                       .NumArraySlices = 0,
                       .FirstPlane = 0,
                       .NumPlanes = 0},
      .Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE};
  D3D12_BARRIER_GROUP group{};
  group.Type = D3D12_BARRIER_TYPE_TEXTURE;
  group.NumBarriers = 1;
  group.pTextureBarriers = &barrier;
  commandList->Barrier(1, &group);
}
} // namespace

/*
 */
DX12Uploader::~DX12Uploader()
{
  CH_ASSERT(!m_ringBuffer);
}

/*
 */
void
DX12Uploader::initialize(ID3D12Device4* device,
                         D3D12MA::Allocator* allocator,
                         DX12DeletionQueue* deletionQueue)
{
  m_device = device;
  m_allocator = allocator;
  m_deletionQueue = deletionQueue;

  const D3D12_RESOURCE_DESC1 ringDesc = getStagingDesc(RING_SIZE);
  D3D12MA::ALLOCATION_DESC allocationDesc{};
  allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
  DX12_CHECK(m_allocator->CreateResource3(&allocationDesc, &ringDesc,
                                          D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, 0, nullptr,
                                          &m_ringAllocation, IID_ID3D12Resource,
                                          outPtr(m_ringBuffer)));
  const D3D12_RANGE noRead{.Begin = 0, .End = 0};
  void* ringData = nullptr;
  DX12_CHECK(m_ringBuffer->Map(0, &noRead, &ringData));
  m_ringData = static_cast<uint8*>(ringData);

  const DX12API& dx12API = g_dx12API();
  dx12API.setDebugName(m_ringBuffer.Get(), "Staging Ring");

  for (uint32 i = 0; i < GraphicsLimits::MAX_FRAMES_IN_FLIGHT; ++i) {
    UploadSlot& slot = m_slots[i];
    DX12_CHECK(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                IID_ID3D12CommandAllocator,
                                                outPtr(slot.allocator)));
    // Made closed, so getCommandList() can reset it like every time after.
    DX12_CHECK(m_device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                            D3D12_COMMAND_LIST_FLAG_NONE,
                                            IID_ID3D12GraphicsCommandList7,
                                            outPtr(slot.commandList)));

    const String name = StringUtils::format("Upload Command List {0}", i);
    dx12API.setDebugName(slot.commandList.Get(), name.c_str());
  }
}

/*
 */
void
DX12Uploader::destroy()
{
  for (UploadSlot& slot : m_slots) {
    slot = {};
  }

  m_ringBuffer.Reset();
  if (m_ringAllocation != nullptr) {
    m_ringAllocation->Release();
    m_ringAllocation = nullptr;
  }
  m_ringData = nullptr;
  m_pendingSpans.clear();
  m_isRecording = false;
}

/*
 */
void
DX12Uploader::uploadBuffer(ID3D12Resource* buffer,
                           uint64 offset,
                           const void* data,
                           SIZE_T size)
{
  CH_ASSERT(data != nullptr && size > 0);

  const StagingRegion staging = allocateStaging(size, BUFFER_STAGING_ALIGNMENT);
  memcpy(staging.mappedData, data, size);
  getCommandList()->CopyBufferRegion(buffer, offset, staging.buffer, staging.offset, size);
}

/*
 */
void
DX12Uploader::uploadTexture(const DX12Texture& texture, const void* data, SIZE_T size)
{
  CH_ASSERT(data != nullptr && size > 0);
  const Format format = texture.getFormat();
  CH_ASSERT(!FormatUtils::isDepth(format) && "Depth textures cannot be uploaded");

  ID3D12Resource* resource = texture.getHandle();
  const D3D12_RESOURCE_DESC desc = resource->GetDesc();
  const uint32 mipLevels = texture.getMipLevels();
  // The layers of a 3D texture are its depth slices, which the footprints already cover.
  const uint32 layers =
      texture.getType() == TextureType::Texture3D ? 1 : texture.getArrayLayers();
  const uint32 subresourceCount = mipLevels * layers;
  if (m_footprints.size() < subresourceCount) {
    m_footprints.resize(subresourceCount);
    m_rowCounts.resize(subresourceCount);
    m_rowSizes.resize(subresourceCount);
  }

  // Rows of a texture copy start at 256-byte steps, so each row is placed on its own.
  UINT64 stagingSize = 0;
  m_device->GetCopyableFootprints(&desc, 0, subresourceCount, 0, m_footprints.data(),
                                  m_rowCounts.data(), m_rowSizes.data(), &stagingSize);

  // Mips are copied while the data holds them whole.
  uint32 uploadedMips = 0;
  SIZE_T dataSize = 0;
  for (uint32 mip = 0; mip < mipLevels; ++mip) {
    const uint32 width = Math::max(texture.getWidth() >> mip, 1u);
    const uint32 height = Math::max(texture.getHeight() >> mip, 1u);
    const uint32 depth = Math::max(texture.getDepth() >> mip, 1u);
    const SIZE_T mipSize = FormatUtils::getMipSize(format, width, height, depth) * layers;
    if (dataSize + mipSize > size) {
      break;
    }
    dataSize += mipSize;
    ++uploadedMips;
  }
  if (uploadedMips == 0) {
    CH_LOG_ERROR(DX12, "Texture upload of {0} bytes is smaller than its first mip level",
                 size);
    return;
  }

  const StagingRegion staging =
      allocateStaging(stagingSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
  const auto* source = static_cast<const uint8*>(data);
  SIZE_T mipOffset = 0;
  for (uint32 mip = 0; mip < uploadedMips; ++mip) {
    const SIZE_T layerSize = static_cast<SIZE_T>(m_rowSizes[mip]) * m_rowCounts[mip] *
                             m_footprints[mip].Footprint.Depth;
    for (uint32 layer = 0; layer < layers; ++layer) {
      const uint32 subresource = mip + layer * mipLevels;
      const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint = m_footprints[subresource];
      const SIZE_T rowSize = m_rowSizes[subresource];
      const uint32 rowCount = m_rowCounts[subresource] * footprint.Footprint.Depth;
      const uint8* sourceLayer = source + mipOffset + layer * layerSize;
      uint8* destination = staging.mappedData + footprint.Offset;
      for (uint32 row = 0; row < rowCount; ++row) {
        memcpy(destination + static_cast<SIZE_T>(row) * footprint.Footprint.RowPitch,
               sourceLayer + row * rowSize, rowSize);
      }
    }
    mipOffset += layerSize * layers;
  }

  ID3D12GraphicsCommandList7* commandList = getCommandList();
  // The old contents are replaced, so the texture starts from the undefined layout.
  recordTextureBarrier(commandList, resource, D3D12_BARRIER_SYNC_NONE,
                       D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_NO_ACCESS,
                       D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_LAYOUT_UNDEFINED,
                       D3D12_BARRIER_LAYOUT_COPY_DEST);

  for (uint32 mip = 0; mip < uploadedMips; ++mip) {
    for (uint32 layer = 0; layer < layers; ++layer) {
      const uint32 subresource = mip + layer * mipLevels;
      D3D12_TEXTURE_COPY_LOCATION destination{};
      destination.pResource = resource;
      destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
      destination.SubresourceIndex = subresource;

      D3D12_TEXTURE_COPY_LOCATION sourceLocation{};
      sourceLocation.pResource = staging.buffer;
      sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
      sourceLocation.PlacedFootprint = m_footprints[subresource];
      sourceLocation.PlacedFootprint.Offset += staging.offset;

      commandList->CopyTextureRegion(&destination, 0, 0, 0, &sourceLocation, nullptr);
    }
  }

  recordTextureBarrier(commandList, resource, D3D12_BARRIER_SYNC_COPY,
                       D3D12_BARRIER_SYNC_ALL_SHADING, D3D12_BARRIER_ACCESS_COPY_DEST,
                       D3D12_BARRIER_ACCESS_SHADER_RESOURCE, D3D12_BARRIER_LAYOUT_COPY_DEST,
                       D3D12_BARRIER_LAYOUT_SHADER_RESOURCE);
}

/*
 */
ID3D12CommandList*
DX12Uploader::endRecording(uint64 submitValue)
{
  if (!m_isRecording) {
    return nullptr;
  }

  UploadSlot& slot = m_slots[m_slotIndex];

  // The frame's list comes after these copies in the same submit and may read the buffers
  // they wrote.
  recordGlobalBarrier(slot.commandList.Get(), D3D12_BARRIER_SYNC_COPY,
                      D3D12_BARRIER_SYNC_ALL, D3D12_BARRIER_ACCESS_COPY_DEST,
                      D3D12_BARRIER_ACCESS_VERTEX_BUFFER | D3D12_BARRIER_ACCESS_INDEX_BUFFER |
                          D3D12_BARRIER_ACCESS_CONSTANT_BUFFER |
                          D3D12_BARRIER_ACCESS_SHADER_RESOURCE |
                          D3D12_BARRIER_ACCESS_COPY_SOURCE);
  DX12_CHECK(slot.commandList->Close());

  slot.submitValue = submitValue;
  const uint64 lastSpanEnd = m_pendingSpans.empty() ? m_ringTail : m_pendingSpans.back().end;
  if (m_ringHead > lastSpanEnd) {
    m_pendingSpans.push_back({.end = m_ringHead, .submitValue = submitValue});
  }

  m_slotIndex = (m_slotIndex + 1) % GraphicsLimits::MAX_FRAMES_IN_FLIGHT;
  m_isRecording = false;
  return slot.commandList.Get();
}

/*
 */
DX12Uploader::StagingRegion
DX12Uploader::allocateStaging(uint64 size, uint64 alignment)
{
  if (size <= RING_SIZE) {
    releaseFinishedSpans();

    uint64 offset = Math::alignUp(m_ringHead, alignment);
    // A region never wraps around the end of the ring, so it skips to the start instead.
    if (offset % RING_SIZE + size > RING_SIZE) {
      offset = Math::alignUp(offset, RING_SIZE);
    }
    if (offset + size - m_ringTail <= RING_SIZE) {
      m_ringHead = offset + size;
      const uint64 ringOffset = offset % RING_SIZE;
      return {.buffer = m_ringBuffer.Get(),
              .offset = ringOffset,
              .mappedData = m_ringData + ringOffset};
    }
  }

  // Too big, or the ring is still in use: a buffer of its own, released after the submit
  // that carries these copies, which is the next one.
  const D3D12_RESOURCE_DESC1 desc = getStagingDesc(size);
  D3D12MA::ALLOCATION_DESC allocationDesc{};
  allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
  D3D12MA::Allocation* allocation = nullptr;
  ComPtr<ID3D12Resource> buffer;
  DX12_CHECK(m_allocator->CreateResource3(&allocationDesc, &desc,
                                          D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, 0, nullptr,
                                          &allocation, IID_ID3D12Resource, outPtr(buffer)));
  const D3D12_RANGE noRead{.Begin = 0, .End = 0};
  void* mappedData = nullptr;
  DX12_CHECK(buffer->Map(0, &noRead, &mappedData));

  // The queue keeps the buffer alive past this call; the copy reads it on the GPU.
  ID3D12Resource* rawBuffer = buffer.Get();
  m_deletionQueue->enqueue(buffer);
  m_deletionQueue->enqueueObject(allocation);

  return {.buffer = rawBuffer, .offset = 0, .mappedData = static_cast<uint8*>(mappedData)};
}

/*
 */
ID3D12GraphicsCommandList7*
DX12Uploader::getCommandList()
{
  UploadSlot& slot = m_slots[m_slotIndex];
  if (m_isRecording) {
    return slot.commandList.Get();
  }

  // The slot's list is recorded again only once its last submit has finished.
  m_deletionQueue->waitForValue(slot.submitValue);
  DX12_CHECK(slot.allocator->Reset());
  DX12_CHECK(slot.commandList->Reset(slot.allocator.Get(), nullptr));

  // Earlier submits may still read or write what these copies overwrite.
  recordGlobalBarrier(slot.commandList.Get(), D3D12_BARRIER_SYNC_ALL,
                      D3D12_BARRIER_SYNC_COPY,
                      D3D12_BARRIER_ACCESS_COPY_DEST | D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
                      D3D12_BARRIER_ACCESS_COPY_DEST);

  m_isRecording = true;
  return slot.commandList.Get();
}

/*
 */
void
DX12Uploader::releaseFinishedSpans()
{
  if (m_pendingSpans.empty()) {
    return;
  }

  const uint64 completedValue = m_deletionQueue->getFence()->GetCompletedValue();
  SIZE_T finished = 0;
  while (finished < m_pendingSpans.size() &&
         m_pendingSpans[finished].submitValue <= completedValue) {
    m_ringTail = m_pendingSpans[finished].end;
    ++finished;
  }
  m_pendingSpans.erase(m_pendingSpans.begin(),
                       m_pendingSpans.begin() + static_cast<int64>(finished));
}

} // namespace chEngineSDK
