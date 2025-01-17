/************************************************************************/
/**
 * @file DX12IndexBuffer.cpp
 * @author AccelMR
 * @date 2022/11/23
 *    Direct X 12 implementation of a index buffer that hold actual data.
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chDX12IndexBuffer.h"

#include "chDX12GraphicsModule.h"
#include "chDX12Translator.h"

namespace chEngineSDK{

/*
*/
DX12IndexBuffer::~DX12IndexBuffer() {

}

/*
*/
void
DX12IndexBuffer::_internalInit(const chGPUDesc::IndexBufferDesc& des) {
  auto dx12Device = g_DX12GraphicsModule().getDevice();
  CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
  auto tmpBuff = CD3DX12_RESOURCE_DESC::Buffer(des.size);
  ThrowIfFailed(dx12Device->CreateCommittedResource(&heapProperties,
                                                    D3D12_HEAP_FLAG_NONE,
                                                    &tmpBuff,
                                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                                    nullptr,
                                                    IID_PPV_ARGS(&m_indexBuffer)));

}

/*
*/
void
DX12IndexBuffer::_internalUpdate(const chGPUDesc::IndexBufferDesc& des, 
                                 const IndexBufferData data) {
                                   // Copy the triangle data to the vertex buffer.
  UINT8* pIndexDataBegin;
  CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
  ThrowIfFailed(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&pIndexDataBegin)));
  memcpy(pIndexDataBegin, data, des.size);
  m_indexBuffer->Unmap(0, nullptr);

  // Create index buffer view.
  m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
  m_indexBufferView.Format = DX12Translator::get(des.format);
  m_indexBufferView.SizeInBytes = des.size;
}

}
