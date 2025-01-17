/************************************************************************/
/**
 * @file chVulkanGPUResourceModule.h
 * @author AccelMR
 * @date 2024/10/15
 *    Vulkan implementation of a GPU resource.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesVulkan.h"

#include <chGPUResourceModule.h>

namespace chEngineSDK{
using namespace chGPUDesc;

class VulkanResourceModule final: public GPUResourceModule
{
 public:
 /*
  *   Default constructor
  */
  VulkanResourceModule() = default;

 /*
  *   Default destructor
  */
  ~VulkanResourceModule();

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