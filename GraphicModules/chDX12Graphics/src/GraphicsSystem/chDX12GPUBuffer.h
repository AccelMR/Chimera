/************************************************************************/
/**
 * @file chDX12GPUBuffer.h
 * @author AccelMR
 * @date 2022/10/18
 *    DirectX 12 implementation of a GPU buffer.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesDX12.h"

#include <chGPUBuffer.h>

namespace chEngineSDK{
class DX12GPUBuffer: public GPUBuffer
{
 public:
 /*
  *   Default constructor
  */
  DX12GPUBuffer() = default;

 /*
  *   Default destructor
  */
  ~DX12GPUBuffer();

 private:
 
 protected:
  friend class DX12GPUCommandBuffer;

  void 
  _init(const SIZE_T& size) override;

  void 
  _internalUpdate(const SIZE_T& size, const GPUBufferData data ) override;

 private:
  ID3D12Resource* m_buffer;


};
}

