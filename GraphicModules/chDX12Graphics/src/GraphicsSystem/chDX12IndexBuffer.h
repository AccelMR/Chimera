/************************************************************************/
/**
 * @file DX12IndexBuffer.h
 * @author AccelMR
 * @date 2022/11/23
 *    Direct X 12 implementation of a index buffer that hold actual data. 
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesDX12.h"

#include "chIndexBuffer.h"

namespace chEngineSDK{
class DX12IndexBuffer final: public IndexBuffer {
 public:
 /*
  *   Default constructor
  */
  DX12IndexBuffer() = default;

 /*
  *   Default destructor
  */
  ~DX12IndexBuffer();

  void
  _internalUpdate(const chGPUDesc::IndexBufferDesc& des, 
                  const IndexBufferData data) override;

  void 
  _internalInit(const chGPUDesc::IndexBufferDesc& des) override;


protected:
  friend class DX12GPUResourceModule;
  friend class DX12GPUCommandBuffer;

protected:
  ID3D12Resource* m_indexBuffer;
  D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
 
};
}

