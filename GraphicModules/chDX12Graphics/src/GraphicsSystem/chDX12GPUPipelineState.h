/************************************************************************/
/**
 * @file chDX12GPUPipelineState.h
 * @author AccelMR
 * @date 2022/09/29
 *    
 *      DirectX 12 specific pipeline state implementation.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesDX12.h"

#include <chGPUPipelineState.h>

namespace chEngineSDK {
class DX12GPUPipelineState: public GPUPipelineState
{
 public:
 /*
  *   Default constructor
  */
  DX12GPUPipelineState(): m_pipeline(nullptr) {}

 /*
  *   Default destructor
  */
  ~DX12GPUPipelineState();
 
 protected:
  friend class GraphicsModuleDX12;
  friend class DX12GPUCommandBuffer;

  void 
  _init(const chGPUDesc::PipelineStateDesc& desc, ID3D12Device* device);

 private:
   ID3D12PipelineState *m_pipeline;
   ID3D12RootSignature *m_rs;
};
}
