/************************************************************************/
/**
 * @file chDX12API.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of the graphics API interface.
 */
/************************************************************************/
#pragma once

#include "chDX12Prerequisites.h"

#include "chIGraphicsAPI.h"
#include "chDX12CommandList.h"
#include "chDX12DeletionQueue.h"
#include "chDX12DescriptorHeap.h"
#include "chDX12Uploader.h"

namespace chEngineSDK {

/**
 * Direct3D 12 device with one direct queue. Every pipeline shares one root signature: the
 * bindless heaps are indexed straight from the shaders and the push constants are its root
 * constants.
 */
class DX12API : public IGraphicsAPI
{
 public:
  DX12API() = default;
  ~DX12API() override;

  void
  initialize(const GraphicsAPIInfo& graphicsAPIInfo) override;

  NODISCARD String
  getAdapterName() const override
  {
    return m_adapterName;
  }

  NODISCARD uint64
  getPlatformWindowFlags() const override
  {
    return 0;
  }

  NODISCARD ShaderBinaryFormat
  getShaderBinaryFormat() const override
  {
    return {.folder = "DXIL", .extension = "dxil"};
  }

  NODISCARD SPtr<ISwapChain>
  createSwapChain(const SwapChainDesc& desc) override;

  NODISCARD SPtr<IBuffer>
  createBuffer(const BufferCreateInfo& createInfo) override;

  NODISCARD SPtr<ITexture>
  createTexture(const TextureCreateInfo& createInfo) override;

  NODISCARD SPtr<IShader>
  createShader(const ShaderCreateInfo& createInfo) override;

  NODISCARD SPtr<IPipeline>
  createGraphicsPipeline(const GraphicsPipelineDesc& desc) override;

  NODISCARD SPtr<ISampler>
  createSampler(const SamplerCreateInfo& createInfo) override;

  NODISCARD ICommandList&
  beginFrame() override;

  void
  endFrame() override;

  NODISCARD uint32
  getFrameIndex() const override
  {
    return m_frameIndex;
  }

  void
  waitIdle() override;

  /**
   * Logs why the device was lost, after a call failed with DXGI_ERROR_DEVICE_REMOVED.
   */
  void
  logDeviceRemovedReason() const;

  /**
   * Names an object for the debug layer messages and for PIX and RenderDoc.
   */
  void
  setDebugName(ID3D12Object* object, const ANSICHAR* name) const;

  NODISCARD FORCEINLINE ID3D12Device4*
  getDevice() const
  {
    return m_device.Get();
  }

  NODISCARD FORCEINLINE DX12Uploader&
  getUploader()
  {
    return m_uploader;
  }

  NODISCARD FORCEINLINE DX12DeletionQueue&
  getDeletionQueue()
  {
    return m_deletionQueue;
  }

  NODISCARD FORCEINLINE DX12DescriptorHeap&
  getResourceHeap()
  {
    return m_resourceHeap;
  }

  NODISCARD FORCEINLINE DX12DescriptorHeap&
  getSamplerHeap()
  {
    return m_samplerHeap;
  }

  NODISCARD FORCEINLINE DX12DescriptorHeap&
  getRenderTargetHeap()
  {
    return m_renderTargetHeap;
  }

  NODISCARD FORCEINLINE DX12DescriptorHeap&
  getDepthTargetHeap()
  {
    return m_depthTargetHeap;
  }

 private:
  void
  enableDebugLayer();

  NODISCARD bool
  pickAdapter();

  NODISCARD bool
  isDeviceSuitable(ID3D12Device* device, const String& adapterName) const;

  void
  registerMessageCallback();

  void
  createAllocator();

  void
  createRootSignature();

  void
  createFrames();

  struct FrameData
  {
    ComPtr<ID3D12CommandAllocator> allocator;
    UniquePtr<DX12CommandList> commandList;
    // Fence value once the GPU finishes the last frame of the slot.
    uint64 submitValue = 0;
  };

  // Render targets of every swap chain image plus the engine's own targets.
  static constexpr uint32 RENDER_TARGET_HEAP_SIZE = 256;
  static constexpr uint32 DEPTH_TARGET_HEAP_SIZE = 64;

  ComPtr<IDXGIFactory6> m_factory;
  ComPtr<IDXGIAdapter1> m_adapter;
  ComPtr<ID3D12Device4> m_device;
  ComPtr<ID3D12CommandQueue> m_queue;
  ComPtr<ID3D12RootSignature> m_rootSignature;
  ComPtr<ID3D12Fence> m_idleFence;
  uint64 m_idleValue = 0;
  String m_adapterName;
  bool m_debugLayerEnabled = false;
  DWORD m_messageCallbackCookie = 0;

  D3D12MA::Allocator* m_allocator = nullptr;
  DX12DeletionQueue m_deletionQueue;
  DX12Uploader m_uploader;
  // Shader visible: ResourceDescriptorHeap and SamplerDescriptorHeap in HLSL.
  DX12DescriptorHeap m_resourceHeap;
  DX12DescriptorHeap m_samplerHeap;
  DX12DescriptorHeap m_renderTargetHeap;
  DX12DescriptorHeap m_depthTargetHeap;

  Array<FrameData, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_frames;
  uint32 m_frameIndex = 0;
};

DX12API&
g_dx12API();

} // namespace chEngineSDK
