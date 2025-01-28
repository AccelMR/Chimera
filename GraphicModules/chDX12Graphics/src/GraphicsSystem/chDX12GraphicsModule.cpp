/************************************************************************/
/**
 * @file chGraphicsModuleDX12.h
 * @author AccelMR
 * @date 2022/09/13
 *   Render hardware implementation specific for Directx12.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chDX12GraphicsModule.h"

#include <chMath.h>
#include <chScreen.h>
#include <chStringUtils.h>
#include <chDebug.h>
#include <chUnicode.h>
#include <chLinearColor.h>

#include "chDX12GPUPipelineState.h" 
#include "chDX12GPUCommandBuffer.h"
#include "chDX12Texture.h"
#include "chDX12SwapChain.h"

namespace chEngineSDK {
  using namespace chGPUDesc;

const float BYTE_TO_MB = 1024 * 1024;

/*
*/
GraphicsModuleDX12::GraphicsModuleDX12(const SPtr<Screen> _screen) 
  : m_screen(_screen),
    m_pipelineState(nullptr),
    m_commandQueue(nullptr),
    m_device(nullptr),
    m_debugController(nullptr),
    m_DebugDevice(nullptr),
    m_factory(nullptr),
    m_adapter(nullptr)
{}

/*
*/
GraphicsModuleDX12::GraphicsModuleDX12() {

}

/*
*/
GraphicsModuleDX12::~GraphicsModuleDX12() {

}

/*
*/
void
GraphicsModuleDX12::_init() {
  CH_ASSERT(m_screen.get());

  // Create Factory
  UINT dxgiFactoryFlags = 0;

#if USING(CH_DEBUG_MODE)
  ID3D12Debug *debugController;
  ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
  ThrowIfFailed(debugController->QueryInterface(IID_PPV_ARGS(&m_debugController)));

  m_debugController->EnableDebugLayer();
  m_debugController->SetEnableGPUBasedValidation(true);

  dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

  debugController->Release();
  debugController = nullptr;

#endif //#if USING(CH_DEBUG_MODE)
  ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));

  // Create Adapter

  DXGI_ADAPTER_DESC1 desc;
  for (uint32 adapterIndex = 0; 
       DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapters1(adapterIndex, &m_adapter);
       ++adapterIndex)
  {
    m_adapter->GetDesc1(&desc);

    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
    {
      // Don't select the Basic Render Driver adapter.
      continue;
    }

    // Check to see if the adapter supports Direct3D 12, but don't create
    // the actual device yet.
    if (SUCCEEDED(D3D12CreateDevice(m_adapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
    {
      break;
    }

    // We won't use this adapter, so release it
    m_adapter->Release();
  }

  // Create Device
  ThrowIfFailed(D3D12CreateDevice(m_adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));

  LOG_INFO(StringUtils::format("Adapter descriptor: [{0}]", UTF8::fromWide(desc.Description)));
  LOG_INFO(StringUtils::format("GPU Vendor ID:  [{0}]", desc.VendorId));
  LOG_INFO(StringUtils::format("GPU Device ID:  [{0}]", desc.DeviceId));
  LOG_INFO(StringUtils::format("System ID: [{0}]", desc.SubSysId));
  LOG_INFO(StringUtils::format("Revision ID: [{0}]", desc.Revision));

  LOG_INFO(StringUtils::format("Dedicated Video Memory: [{0} MB]", desc.DedicatedVideoMemory / BYTE_TO_MB));
  m_dedicatedVideoMemory = desc.DedicatedVideoMemory;
  LOG_INFO(StringUtils::format("Dedicated System Memory: [{0} MB]", desc.DedicatedSystemMemory / BYTE_TO_MB));
  m_dedicatedSystemMemory = desc.DedicatedSystemMemory;
  LOG_INFO(StringUtils::format("Dedicated Shared Memory: [{0} MB]", desc.SharedSystemMemory / BYTE_TO_MB));
  m_sharedSystemMemory = desc.SharedSystemMemory;

  m_device->SetName(L"Chimera DX12 Device");

#if USING(CH_DEBUG_MODE)
  // Get debug device
  ThrowIfFailed(m_device->QueryInterface(&m_DebugDevice));
  ID3D12InfoQueue* pInfoQueue = nullptr;
  ThrowIfFailed(m_device->QueryInterface(IID_PPV_ARGS(&pInfoQueue)));
//pInfoQueue->RegisterMessageCallback();

#endif //#if USING(CH_DEBUG_MODE)

  // Create Command Queue
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

  ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
  _setupSwapchain(m_screen->getWidth(), m_screen->getHeight());
  //OnResize(m_screen->getWidth(), m_screen->getHeight());

//   ID3D12CommandAllocator* TextAlloc = nullptr;
//   ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 
//                                                  IID_PPV_ARGS(&TextAlloc)));
//   m_commandAllocators.emplace(TEXTURE_ALLOC_ID, TextAlloc);
// 
//   ID3D12CommandAllocator* CommandAlloc = nullptr;
//   ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 
//                                                   IID_PPV_ARGS(&CommandAlloc)));
//   m_commandAllocators.emplace(COMMAND_LIST_ALLOC_ID, TextAlloc );
// 
//   
//   ID3D12CommandAllocator* ConstantBufAlloc = nullptr;
//   ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 
//                                                   IID_PPV_ARGS(&ConstantBufAlloc)));
//   m_commandAllocators.emplace(CONSTANT_BUFFER_ALLOC_ID, TextAlloc);
}

/*
*/
SPtr<GPUPipelineState>
GraphicsModuleDX12::_internalcreatePipelineState(const PipelineStateDesc& pipelineStateDesc) {
  auto ret = ch_shared_ptr_new<DX12GPUPipelineState>();
  ret->_init(pipelineStateDesc, m_device);
  return std::reinterpret_pointer_cast<GPUPipelineState>(ret);
}

/*
*/
SPtr<SwapChain>
GraphicsModuleDX12::_internalGetSwapChain() {
  return m_swapChain;
}

/*
*/
void
GraphicsModuleDX12::_internalResetSwapChainAllocator() {
  ThrowIfFailed(m_frameAlloc[m_frameIndex]->Reset());
}

/*
*/
void
GraphicsModuleDX12::createFence() {
    // Create synchronization objects.
  ThrowIfFailed(m_device->CreateFence(m_fenceValues[m_frameIndex],
                                      D3D12_FENCE_FLAG_NONE, 
                                      IID_PPV_ARGS(&m_fence)));
  m_fenceValues[m_frameIndex]++;

  // Create an event handle to use for frame synchronization.
  m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (m_fenceEvent == nullptr)
  {
      ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
  }

  waitGPU();
}

/*
*/
void
GraphicsModuleDX12::_internalOnResize(uint32 width, uint32 height) {
  m_swapChain->m_width = width;
  m_swapChain->m_height = height;
  m_swapChain->_internalResize(m_swapChain->m_frameCount, width, height, FORMAT::kB8G8R8A8_UNORM);

  waitGPU();
}

/*
*/
void
GraphicsModuleDX12::_internalWaitGPU() {
  // Schedule a Signal command in the queue.
  ThrowIfFailed(m_commandQueue->Signal(m_fence, m_fenceValues[m_frameIndex]));
  
  // Wait until the fence has been processed.
  ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex],
                                              m_fenceEvent));
  WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
  
  // Increment the fence value for the current frame.
  m_fenceValues[m_frameIndex]++;
}

/*
*/
void
GraphicsModuleDX12::_setupSwapchain(uint32 width, uint32 height) {
  SwapChainDesc swapDesc = {};
  swapDesc.width = width;
  swapDesc.height = height;
  swapDesc.format = FORMAT::kB8G8R8A8_UNORM;//TODO
  
  m_swapChain = ch_shared_ptr_new<DX12SwapChain>();
  m_swapChain->init(swapDesc);
  
  m_frameIndex = m_swapChain->getCurrentFrameIndex();
  
  m_frameAlloc.resize(swapDesc.frameCount);
  for (uint32 i = 0; i < swapDesc.frameCount; i++) {
    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 
                                                   IID_PPV_ARGS(&m_frameAlloc[i])));
  }
}

/*
*/
void
GraphicsModuleDX12::onStartUp() {
  _init();
}

/*
*/
void
GraphicsModuleDX12::onShutDown() {}

/*
*/
SPtr<GPUCommandBuffer>
GraphicsModuleDX12::_internalCreateGPUCommandBuffer() {
  auto dx12cf = ch_shared_ptr_new<DX12GPUCommandBuffer>();
  return std::reinterpret_pointer_cast<GPUCommandBuffer>(dx12cf);
}

/*
*/
void
GraphicsModuleDX12::_internalExecuteCommandBuffers(const Vector<SPtr<GPUCommandBuffer>>& commandBuffers) {
  // Execute the command list.
  Vector<ID3D12CommandList*> lists;
  for (const auto& list: commandBuffers) {
    auto dxList = std::reinterpret_pointer_cast<DX12GPUCommandBuffer>(list);
    lists.push_back(dxList->m_commandList);
  }
  m_commandQueue->ExecuteCommandLists(lists.size(), &lists[0]);
}

/*
*/
void
GraphicsModuleDX12::_internalMoveToNextFrame() {
  // Schedule a Signal command in the queue.
  const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
  ThrowIfFailed(m_commandQueue->Signal(m_fence, currentFenceValue));
  
  // Update the frame index.
  m_frameIndex = m_swapChain->getCurrentFrameIndex();
  
  // If the next frame is not ready to be rendered yet, wait until it is ready.
  if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
  {
    ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex],m_fenceEvent));
    WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
  }
  
  // Set the fence value for the next frame.
  m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}

/*
*/
GraphicsModuleDX12&
g_DX12GraphicsModule() {
  return reinterpret_cast<GraphicsModuleDX12&>(GraphicsModule::instance());
}

}