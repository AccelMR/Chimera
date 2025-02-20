/************************************************************************/
/**
 * @file chDX12Texture.cpp
 * @author AccelMR
 * @date 2022/11/13
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chDX12Texture.h"

#include "chDX12Translator.h"

#include "chDX12GraphicsModule.h"

namespace chEngineSDK{

/*
*/
DX12Texture::~DX12Texture() {
  if (m_freeResources) {
    SafeRelease(m_resource);
    SafeRelease(m_rtvHeap);
    SafeRelease(m_srvHeap);
    SafeRelease(m_dsvHeap);
  }
}

/*
*/
void
DX12Texture::_internalInit(const chGPUDesc::TextureDesc& desc) {
  ID3D12Device* device = g_DX12GraphicsModule().getDevice();
  D3D12_RESOURCE_DESC resourceDesc;
  D3D12_RESOURCE_DIMENSION dim;

  switch (desc.type) {
  default:
    break;
  case chGPUDesc::TextureDesc::TYPE::kTYPE_1D:
    dim = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    break;
  case chGPUDesc::TextureDesc::TYPE::kTYPE_2D:
    dim = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    break;
  case chGPUDesc::TextureDesc::TYPE::kTYPE_3D:
    dim = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    break;
  }
  resourceDesc.Dimension = dim;
  resourceDesc.Width = static_cast<uint32>(desc.dimensions.x);
  resourceDesc.Height = static_cast<uint32>(desc.dimensions.y);
  resourceDesc.MipLevels = static_cast<uint16>(desc.mips);
  resourceDesc.Format = DX12Translator::get(desc.format);
  resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.SampleDesc.Quality = 0;

  CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
  ThrowIfFailed(
  device->CreateCommittedResource(&heapProperties,
                                  D3D12_HEAP_FLAG_NONE,
                                  &resourceDesc,
                                  D3D12_RESOURCE_STATE_COPY_DEST,
                                  nullptr,
                                  IID_PPV_ARGS(&m_resource)));

  createView(desc.layers, desc.usage);
}

/*
*/
void
DX12Texture::_crateFromResource(ID3D12Resource* resource, 
                                uint32 layers, 
                                chGPUDesc::TextUsageFlag usage) {
  m_resource = resource;
  createView(layers, usage);
}

/*
*/
void
DX12Texture::createView(uint32 layers, chGPUDesc::TextUsageFlag usage){
  ID3D12Device* device = g_DX12GraphicsModule().getDevice();

  // Describe and create a render target view (RTV) descriptor heap.
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
  rtvHeapDesc.NumDescriptors = layers;
  
  D3D12_DESCRIPTOR_HEAP_TYPE heapType = {};
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  //
  if(usage.isSetAny(TEXTURE_USAGE::kRENDER_TARGET)){
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

    uint32 m_rtvDescriptorSize = 
    device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    auto rtv1 = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    device->CreateRenderTargetView(m_resource, nullptr, rtv1);
    rtv1.Offset(1, m_rtvDescriptorSize);

    m_rtv = rtv1;
  }
  //
  if(usage.isSetAny(TEXTURE_USAGE::kSAMPLED)){
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));
    
    uint32 m_srvDescriptorSize = 
    device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    m_srv = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateShaderResourceView(m_resource, nullptr, m_srv);
    m_srv.ptr += (1 * m_srvDescriptorSize);
  }
  //
  if(usage.isSetAny(TEXTURE_USAGE::kDEPTH_STENCIL)){
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

    uint32 m_dsvDescriptorSize = 
    device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    m_dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateDepthStencilView(m_resource, nullptr, m_dsv);
    m_dsv.ptr += (1 * m_dsvDescriptorSize);
  }
}

}


