/************************************************************************/
/**
 * @file chDX12Texture.h
 * @author AccelMR
 * @date 2022/11/13
 * @brief 
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesDX12.h"

#include "chTexture.h"
#include "chGPUResourceDescriptors.h"

namespace chEngineSDK{
class DX12Texture final: public Texture
{
 public:
 /*
  *   Default constructor
  */
  DX12Texture() = default;

  DX12Texture(ID3D12Resource* _resource, ID3D12DescriptorHeap* _rtvHeap)
    : m_resource(_resource),
      m_rtvHeap(_rtvHeap),
      m_freeResources(false)
    {}

 /*
  *   Default destructor
  */
  ~DX12Texture();


protected:
  friend class GraphicsModuleDX12;
  friend class DX12GPUCommandBuffer;
  friend class DX12SwapChain;

  void
  _internalInit(const chGPUDesc::TextureDesc& desc) override;

  void
  _crateFromResource(ID3D12Resource* resource, 
                    uint32 layers, 
                    chGPUDesc::TextUsageFlag usage);

 private:

 void 
 createView(uint32 layers, chGPUDesc::TextUsageFlag usage);

 protected:
  bool m_freeResources = true;
  ID3D12Resource* m_resource;
  ID3D12DescriptorHeap* m_rtvHeap;
  ID3D12DescriptorHeap* m_srvHeap;
  ID3D12DescriptorHeap* m_dsvHeap;

  D3D12_CPU_DESCRIPTOR_HANDLE m_rtv;
  D3D12_CPU_DESCRIPTOR_HANDLE m_srv;
  D3D12_CPU_DESCRIPTOR_HANDLE m_dsv;


 
};
}

