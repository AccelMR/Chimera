/************************************************************************/
/**
 * @file chDX12VertexBuffer.cpp
 * @author AccelMR
 * @date 2022/11/13
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chDX12VertexBuffer.h"

#include "chGPUResourceDescriptors.h"

namespace chEngineSDK{

/*
*/
DX12VertexBuffer::~DX12VertexBuffer() {
}

/*
*/
void
DX12VertexBuffer::_init(const chGPUDesc::VertexBufferDesc& desc, ID3D12Device* device) {
  CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
  auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(desc.size);
  ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_vertexBuffer)));
}

/*
*/
void
DX12VertexBuffer::_internalUpdate(const chGPUDesc::VertexBufferDesc& des,
                                  const VertexBufferData data) {
  // Copy the triangle data to the vertex buffer.
  UINT8 *pVertexDataBegin;
  CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
  ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pVertexDataBegin)));
  memcpy(pVertexDataBegin, data, des.size);
  m_vertexBuffer->Unmap(0, nullptr);

  // Initialize the vertex buffer view.
  m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
  m_vertexBufferView.StrideInBytes = des.strideInBytes;
  m_vertexBufferView.SizeInBytes = des.size;
}

}


