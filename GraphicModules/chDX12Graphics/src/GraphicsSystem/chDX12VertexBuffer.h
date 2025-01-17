/************************************************************************/
/**
 * @file chDX12VertexBuffer.h
 * @author AccelMR
 * @date 2022/11/13
 *    Vertex buffer with DirectX 12 specific implementation.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesDX12.h"

#include "chVertexBuffer.h"

namespace chEngineSDK{
class DX12VertexBuffer final: public VertexBuffer
{
 public:
 /*
  *   Default constructor
  */
  DX12VertexBuffer() = default;

 /*
  *   Default destructor
  */
  ~DX12VertexBuffer();

 protected:
  friend class DX12GPUCommandBuffer;

  void 
  _internalUpdate(const chGPUDesc::VertexBufferDesc& des,
                  const VertexBufferData data ) override;

 protected:
 friend class DX12GPUResourceModule;

 void
 _init(const chGPUDesc::VertexBufferDesc& desc, ID3D12Device* device);

 private:
  ID3D12Resource* m_vertexBuffer;
  D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
 

};
}

