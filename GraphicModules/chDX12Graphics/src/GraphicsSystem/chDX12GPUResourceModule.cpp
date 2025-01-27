/************************************************************************/
/**
 * @file chDX12GPUResourceModule.cpp
 * @author AccelMR
 * @date 2022/10/18
 *    DirectX 12 implementation of a GPU resource.
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chDX12GPUResourceModule.h"

#include "chDX12GraphicsModule.h"
#include "chDX12Shader.h"
#include "chDX12VertexBuffer.h"
#include "chDX12IndexBuffer.h"
#include "chDX12GPUBuffer.h"

namespace chEngineSDK{

/*
*/
DX12GPUResourceModule::~DX12GPUResourceModule() = default;

/*
*/
SPtr<Texture>
DX12GPUResourceModule::_internalCreateTexture(const chGPUDesc::TextureDesc& desc) {
    return nullptr;
}

/*
*/
SPtr<Shader>
DX12GPUResourceModule::_internalCreateShader(const chGPUDesc::ShaderDesc& desc) {
  SPtr<DX12Shader> shader = ch_shared_ptr_new<DX12Shader>();
  shader->_init(desc);
  return std::reinterpret_pointer_cast<Shader>(shader);
}

/*
*/
SPtr<GPUBuffer>
DX12GPUResourceModule::_internalCreateBuffer(const SIZE_T& size) {
  auto dx12Buffer = ch_shared_ptr_new<DX12GPUBuffer>();
  dx12Buffer->init(size);
  return std::reinterpret_pointer_cast<GPUBuffer>(dx12Buffer);
}

/*
*/
SPtr<Sampler>
DX12GPUResourceModule::_internalCreateSampler(const chGPUDesc::SamplerDesc& desc) {
    return nullptr;
}

/*
*/
SPtr<VertexBuffer>
DX12GPUResourceModule::_internalCreateVertexBuffer(const chGPUDesc::VertexBufferDesc& desc) {
  auto dx12VertexBuffer = ch_shared_ptr_new<DX12VertexBuffer>();
  auto dx12Device = g_DX12GraphicsModule().getDevice();
  dx12VertexBuffer->_init(desc, dx12Device);
  return std::reinterpret_pointer_cast<VertexBuffer>(dx12VertexBuffer);
}

/*
*/
SPtr<IndexBuffer>
DX12GPUResourceModule::_internalCreateIndexBuffer(const IndexBufferDesc& desc) {
  auto dx12IndexBuffer = ch_shared_ptr_new<DX12IndexBuffer>();
  dx12IndexBuffer->init(desc);
  return std::reinterpret_pointer_cast<IndexBuffer>(dx12IndexBuffer);
}

}


