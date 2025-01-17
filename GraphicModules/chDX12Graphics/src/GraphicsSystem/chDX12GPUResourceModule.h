/************************************************************************/
/**
 * @file chDX12GPUResourceModule.h
 * @author AccelMR
 * @date 2022/10/18
 *    DirectX 12 implementation of a GPU resource.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesDX12.h"

#include <chGPUResourceModule.h>

namespace chEngineSDK{
using namespace chGPUDesc;

class DX12GPUResourceModule final: public GPUResourceModule
{
 public:
 /*
  *   Default constructor
  */
  DX12GPUResourceModule() = default;

 /*
  *   Default destructor
  */
  ~DX12GPUResourceModule();

 private:
 
protected:
  SPtr<Texture>
  _internalCreateTexture(const TextureDesc &desc) override;

  SPtr<Shader>
  _internalCreateShader(const ShaderDesc &desc) override;

  SPtr<GPUBuffer>
  _internalCreateBuffer(const SIZE_T& size) override;

  SPtr<Sampler>
  _internalCreateSampler(const SampleDesc& desc) override;

  SPtr<VertexBuffer> 
  _internalCreateVertexBuffer(const VertexBufferDesc& desc) override;

  SPtr<IndexBuffer> 
  _internalCreateIndexBuffer(const IndexBufferDesc& desc) override;

};
}

