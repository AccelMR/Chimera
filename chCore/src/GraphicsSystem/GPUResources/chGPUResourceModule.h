/************************************************************************/
/**
 * @file chGPUResourceModule.h
 * @author AccelMR
 * @date 2022/09/14
 *   GPU Resource module interface, to handle every resource.
 */
 /************************************************************************/
 #pragma once

 /************************************************************************/
 /*
  * Includes
  */                                                                     
 /************************************************************************/
 #include "chPrerequisitesCore.h"

 #include <chModule.h>

 #include "chGPUResourceDescriptors.h"

 #include "chGPUResource.h"

 #include "chTexture.h"
 #include "chShader.h"
 #include "chGPUBuffer.h"
 #include "chVertexBuffer.h"
 #include "chIndexBuffer.h"

namespace chEngineSDK{
using namespace chGPUDesc;

class CH_CORE_EXPORT GPUResourceModule: public Module<GPUResourceModule>
{
 public: 
 /*
  *   Default destructor
  */
  virtual ~GPUResourceModule() = default;

  SPtr<Texture>
  createTexture(const TextureDesc& desc);

  SPtr<Shader>
  createShader(const ShaderDesc& desc);

  SPtr<GPUBuffer>
  createBuffer(const SIZE_T& size);

  SPtr<Sampler>
  createSampler(const SamplerDesc desc);

  //TODO: instead of this return a mesh that wrapps vertex and index info.
  SPtr<VertexBuffer>
  createVertexBuffer(const VertexBufferDesc& desc);

  SPtr<IndexBuffer>
  createIndexBuffer(const IndexBufferDesc& desc);

 protected:

  virtual SPtr<Texture>
  _internalCreateTexture(const TextureDesc& desc) = 0;

  virtual SPtr<Shader>
  _internalCreateShader(const ShaderDesc& desc) = 0;

  virtual SPtr<GPUBuffer>
  _internalCreateBuffer(const SIZE_T& size) = 0;

  virtual SPtr<Sampler>
  _internalCreateSampler(const SamplerDesc& desc) = 0;

  virtual SPtr<VertexBuffer>
  _internalCreateVertexBuffer(const VertexBufferDesc& desc) = 0;

  virtual SPtr<IndexBuffer>
  _internalCreateIndexBuffer(const IndexBufferDesc& desc) = 0;
};
}
