/************************************************************************/
/**
 * @file chGPUResourceModule.h
 * @author AccelMR
 * @date 2022/09/14
 *   GPU Resource module interface, to handle every resource.
 */
 /************************************************************************/
 #include "chGPUResourceModule.h"

namespace chEngineSDK{
using namespace chGPUDesc;
 
/*
*/
SPtr<Texture>
GPUResourceModule::createTexture(const TextureDesc& desc) {
 return _internalCreateTexture(desc);
}

/*
*/
SPtr<Shader>
GPUResourceModule::createShader(const ShaderDesc& desc) {
  return _internalCreateShader(desc);
}

/*
*/
SPtr<GPUBuffer>
GPUResourceModule::createBuffer(const SIZE_T& size) {
  return _internalCreateBuffer(size);
}

/*
*/
SPtr<Sampler>
GPUResourceModule::createSampler(const SamplerDesc desc) {
  return _internalCreateSampler(desc);
}

/*
*/
SPtr<VertexBuffer>
GPUResourceModule::createVertexBuffer(const chGPUDesc::VertexBufferDesc& desc) {
  return _internalCreateVertexBuffer(desc);
}

/*
*/
SPtr<IndexBuffer>
GPUResourceModule::createIndexBuffer(const IndexBufferDesc& desc) {
  return _internalCreateIndexBuffer(desc);
}

}