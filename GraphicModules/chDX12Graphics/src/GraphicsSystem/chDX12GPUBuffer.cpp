/************************************************************************/
/**
 * @file chDX12GPUBuffer.cpp
 * @author AccelMR
 * @date 2022/11/23
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chDX12GPUBuffer.h"

#include "chDX12GraphicsModule.h"
namespace chEngineSDK{

/*
*/
DX12GPUBuffer::~DX12GPUBuffer() {
  SafeRelease(m_buffer);
}

/*
*/
void
DX12GPUBuffer::_init(const SIZE_T& size) {
  CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
  auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
  auto dx12Device = g_DX12GraphicsModule().getDevice();
  ThrowIfFailed(dx12Device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_buffer)));
}

/*
*/
void
DX12GPUBuffer::_internalUpdate(const SIZE_T& size,
                               const GPUBufferData data) {
  UINT8* pDataBegin;
  CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
  ThrowIfFailed(m_buffer->Map(0, &readRange, reinterpret_cast<void **>(&pDataBegin)));
  memcpy(pDataBegin, data, size);
  m_buffer->Unmap(0, nullptr);
}

}


