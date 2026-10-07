/************************************************************************/
/**
 * @file chDX12Buffer.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of IBuffer.
 */
/************************************************************************/
#include "chDX12Buffer.h"

#include <cstring>

#include <D3D12MemAlloc.h>

#include "chDX12API.h"
#include "chMath.h"

namespace chEngineSDK {
namespace {
NODISCARD D3D12_HEAP_TYPE
toHeapType(MemoryUsage memoryUsage)
{
  switch (memoryUsage) {
  case MemoryUsage::CpuOnly:
  case MemoryUsage::CpuToGpu:
    return D3D12_HEAP_TYPE_UPLOAD;
  case MemoryUsage::GpuToCpu:
    return D3D12_HEAP_TYPE_READBACK;
  case MemoryUsage::GpuOnly:
  default:
    return D3D12_HEAP_TYPE_DEFAULT;
  }
}

// Raw views address the buffer in 4-byte words.
constexpr uint64 kRawViewAlignment = 4;
} // namespace

/*
 */
DX12Buffer::DX12Buffer(D3D12MA::Allocator* allocator, const BufferCreateInfo& createInfo)
  : m_size(createInfo.size)
{
  const bool isUniform = createInfo.usage.isSet(BufferUsage::UniformBuffer);
  const bool isStorage = createInfo.usage.isSet(BufferUsage::StorageBuffer);
  const D3D12_HEAP_TYPE heapType = toHeapType(createInfo.memoryUsage);

  // Constant buffer views cover whole 256-byte blocks, so the memory behind them must too.
  uint64 allocatedSize = m_size;
  if (isUniform) {
    allocatedSize =
        Math::alignUp(allocatedSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
  }
  else if (isStorage) {
    allocatedSize = Math::alignUp(allocatedSize, kRawViewAlignment);
  }

  // Shaders can only write buffers of the default heap.
  const bool isWritable = isStorage && !isUniform && heapType == D3D12_HEAP_TYPE_DEFAULT;
  const D3D12_RESOURCE_DESC1 desc{
      .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
      .Alignment = 0,
      .Width = allocatedSize,
      .Height = 1,
      .DepthOrArraySize = 1,
      .MipLevels = 1,
      .Format = DXGI_FORMAT_UNKNOWN,
      .SampleDesc = {.Count = 1, .Quality = 0},
      .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
      .Flags = isWritable ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                          : D3D12_RESOURCE_FLAG_NONE,
      .SamplerFeedbackMipRegion = {}};

  D3D12MA::ALLOCATION_DESC allocationDesc{};
  allocationDesc.HeapType = heapType;
  // Buffers have no layout; every access is allowed and only needs synchronization.
  DX12_CHECK(allocator->CreateResource3(&allocationDesc, &desc, D3D12_BARRIER_LAYOUT_UNDEFINED,
                                        nullptr, 0, nullptr, &m_allocation, IID_ID3D12Resource,
                                        outPtr(m_resource)));
  m_gpuAddress = m_resource->GetGPUVirtualAddress();

  if (heapType != D3D12_HEAP_TYPE_DEFAULT) {
    // The CPU of an upload heap only writes; a null range lets a readback heap be read.
    const D3D12_RANGE noRead{.Begin = 0, .End = 0};
    DX12_CHECK(m_resource->Map(0, heapType == D3D12_HEAP_TYPE_UPLOAD ? &noRead : nullptr,
                               &m_mappedData));
  }

  if (isUniform || isStorage) {
    createBindlessView(createInfo, allocatedSize, isWritable);
  }

  if (createInfo.initialData) {
    update(createInfo.initialData, createInfo.initialDataSize);
  }
}

/*
 */
DX12Buffer::~DX12Buffer()
{
  DX12API& dx12API = g_dx12API();
  DX12DeletionQueue& deletionQueue = dx12API.getDeletionQueue();
  deletionQueue.enqueueDescriptor(dx12API.getResourceHeap(), m_bindlessIndex);
  deletionQueue.enqueue(m_resource);
  deletionQueue.enqueueObject(m_allocation);
}

/*
 */
void
DX12Buffer::update(const void* data, SIZE_T size, uint32 offset)
{
  CH_ASSERT(offset + size <= m_size);
  if (m_mappedData == nullptr) {
    g_dx12API().getUploader().uploadBuffer(m_resource.Get(), offset, data, size);
    return;
  }
  // Upload heaps are write combined and coherent, so nothing needs flushing.
  memcpy(static_cast<uint8*>(m_mappedData) + offset, data, size);
}

/*
 */
void
DX12Buffer::createBindlessView(const BufferCreateInfo& createInfo,
                               uint64 allocatedSize,
                               bool isWritable)
{
  DX12API& dx12API = g_dx12API();
  ID3D12Device* device = dx12API.getDevice();
  DX12DescriptorHeap& heap = dx12API.getResourceHeap();
  m_bindlessIndex = heap.allocate();
  const D3D12_CPU_DESCRIPTOR_HANDLE handle = heap.getCpuHandle(m_bindlessIndex);

  // A buffer can hold one descriptor per index; uniform wins when both are asked for.
  if (createInfo.usage.isSet(BufferUsage::UniformBuffer)) {
    const D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc{
        .BufferLocation = m_gpuAddress,
        .SizeInBytes = static_cast<UINT>(allocatedSize)};
    device->CreateConstantBufferView(&viewDesc, handle);
    return;
  }

  // Storage buffers are raw, read in HLSL as RWByteAddressBuffer (ByteAddressBuffer when
  // the CPU writes them).
  const UINT wordCount = static_cast<UINT>(allocatedSize / kRawViewAlignment);
  if (isWritable) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};
    viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    viewDesc.Buffer.NumElements = wordCount;
    viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    device->CreateUnorderedAccessView(m_resource.Get(), nullptr, &viewDesc, handle);
    return;
  }

  D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};
  viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
  viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  viewDesc.Buffer.NumElements = wordCount;
  viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
  device->CreateShaderResourceView(m_resource.Get(), &viewDesc, handle);
}

} // namespace chEngineSDK
