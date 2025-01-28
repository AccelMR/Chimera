/************************************************************************/
/**
 * @file chVulkanGPUResourceModule.cpp
 * @author AccelMR
 * @date 2024/10/15
 *    Vulkan implementation of a GPU resource.
 *   
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chVulkanGPUResourceModule.h"
#include "chVulkanGraphicsModule.h"
#include "chVulkanVertexBuffer.h"
#include "chVulkanIndexBuffer.h"
#include "chVulkanGPUBuffer.h"
#include "chVulkanShader.h"
#include "chVulkanSampler.h"

namespace chEngineSDK{
using std::static_pointer_cast;

/*
*/
VulkanResourceModule::~VulkanResourceModule() = default;

/*
*/
SPtr<Texture>
VulkanResourceModule::_internalCreateTexture(const chGPUDesc::TextureDesc& desc) {
  return nullptr;
}

/*
*/
SPtr<Shader>
VulkanResourceModule::_internalCreateShader(const chGPUDesc::ShaderDesc& desc) {
  SPtr<VulkanShader> shader = ch_shared_ptr_new<VulkanShader>();
  shader->_init(desc);
  return static_pointer_cast<Shader>(shader);
}

/*
*/
SPtr<GPUBuffer>
VulkanResourceModule::_internalCreateBuffer(const SIZE_T& size) {
  auto vulkanBuffer = ch_shared_ptr_new<VulkanGPUBuffer>();
  vulkanBuffer->init(size);
  return static_pointer_cast<GPUBuffer>(vulkanBuffer);
}

/*
*/
SPtr<Sampler>
VulkanResourceModule::_internalCreateSampler(const chGPUDesc::SamplerDesc& desc) {
  auto vulkanSampler = ch_shared_ptr_new<VulkanSampler>();
  vulkanSampler->init(desc);
  return static_pointer_cast<Sampler>(vulkanSampler);
}

/*
*/
SPtr<VertexBuffer>
VulkanResourceModule::_internalCreateVertexBuffer(const chGPUDesc::VertexBufferDesc& desc) {
  auto vulkanVertexBuffer = ch_shared_ptr_new<VulkanVertexBuffer>();
  vulkanVertexBuffer->init(desc);
  return static_pointer_cast<VertexBuffer>(vulkanVertexBuffer);
}

/*
*/
SPtr<IndexBuffer>
VulkanResourceModule::_internalCreateIndexBuffer(const chGPUDesc::IndexBufferDesc& desc) {
  auto vulkanIndexBuffer = ch_shared_ptr_new<VulkanIndexBuffer>();
  vulkanIndexBuffer->init(desc);
  return static_pointer_cast<IndexBuffer>(vulkanIndexBuffer);
}
} // namespace chEngineSDK