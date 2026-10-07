/************************************************************************/
/**
 * @file chDX12API.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of the graphics API interface.
 */
/************************************************************************/
#include "chDX12API.h"

#include "chDX12SwapChain.h"
#include "chUnicode.h"

namespace chEngineSDK {
namespace {
void WINAPI
onDebugMessage(D3D12_MESSAGE_CATEGORY category,
               D3D12_MESSAGE_SEVERITY severity,
               D3D12_MESSAGE_ID id,
               LPCSTR description,
               void* context)
{
  CH_PARAMETER_UNUSED(category);
  CH_PARAMETER_UNUSED(context);
  switch (severity) {
  case D3D12_MESSAGE_SEVERITY_CORRUPTION:
  case D3D12_MESSAGE_SEVERITY_ERROR:
    CH_LOG_ERROR(DX12, "D3D12 ERROR: [{0}] {1}", id, description);
    break;
  case D3D12_MESSAGE_SEVERITY_WARNING:
    CH_LOG_WARNING(DX12, "D3D12 WARNING: [{0}] {1}", id, description);
    break;
  default:
    CH_LOG_DEBUG(DX12, "D3D12: [{0}] {1}", id, description);
    break;
  }
}
} // namespace

/*
 */
DX12API::~DX12API()
{
  if (m_queue) {
    waitIdle();
  }

  for (FrameData& frame : m_frames) {
    frame.commandList.reset();
    frame.allocator.Reset();
  }
  m_deletionQueue.destroy();
  m_depthTargetHeap.destroy();
  m_renderTargetHeap.destroy();
  m_samplerHeap.destroy();
  m_resourceHeap.destroy();
  m_rootSignature.Reset();
  m_idleFence.Reset();
  m_queue.Reset();

  if (m_device && m_debugLayerEnabled) {
    // Every engine object is gone by now, so anything listed besides the device leaked.
    ComPtr<ID3D12DebugDevice> debugDevice;
    if (SUCCEEDED(m_device.As(&debugDevice))) {
      debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
    }
  }

  if (m_messageCallbackCookie != 0) {
    ComPtr<ID3D12InfoQueue1> infoQueue;
    if (SUCCEEDED(m_device.As(&infoQueue))) {
      infoQueue->UnregisterMessageCallback(m_messageCallbackCookie);
    }
    m_messageCallbackCookie = 0;
  }

  m_device.Reset();
  m_adapter.Reset();
  m_factory.Reset();
}

/*
 */
void
DX12API::initialize(const GraphicsAPIInfo& graphicsAPIInfo)
{
  CH_LOG_DEBUG(DX12, "Initializing Direct3D 12");

  if (graphicsAPIInfo.enableValidationLayer) {
    enableDebugLayer();
  }

  DX12_CHECK(CreateDXGIFactory2(m_debugLayerEnabled ? DXGI_CREATE_FACTORY_DEBUG : 0,
                                IID_IDXGIFactory6, outPtr(m_factory)));

  if (!pickAdapter()) {
    CH_EXCEPT(DX12ErrorException,
              "No GPU supports Direct3D 12 with the features the engine needs; the warnings "
              "above list what each GPU is missing.");
  }

  if (m_debugLayerEnabled) {
    registerMessageCallback();
  }

  const D3D12_COMMAND_QUEUE_DESC queueDesc{.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
                                           .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
                                           .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
                                           .NodeMask = 0};
  DX12_CHECK(m_device->CreateCommandQueue(&queueDesc, IID_ID3D12CommandQueue,
                                          outPtr(m_queue)));
  setDebugName(m_queue.Get(), "Direct Queue");

  DX12_CHECK(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_ID3D12Fence,
                                  outPtr(m_idleFence)));
  setDebugName(m_idleFence.Get(), "Idle Fence");

  m_deletionQueue.initialize(m_device.Get());
  m_resourceHeap.initialize(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                            GraphicsLimits::MAX_BINDLESS_RESOURCES, true,
                            "Bindless Resources");
  m_samplerHeap.initialize(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                           GraphicsLimits::MAX_BINDLESS_SAMPLERS, true, "Bindless Samplers");
  m_renderTargetHeap.initialize(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                                RENDER_TARGET_HEAP_SIZE, false, "Render Target Heap");
  m_depthTargetHeap.initialize(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
                               DEPTH_TARGET_HEAP_SIZE, false, "Depth Target Heap");

  createRootSignature();
  createFrames();

  CH_LOG_DEBUG(DX12, "Direct3D 12 initialized successfully");
  CH_LOG_DEBUG(DX12, "Using Adapter : {0}", m_adapterName);
}

/*
 */
SPtr<ISwapChain>
DX12API::createSwapChain(const SwapChainDesc& desc)
{
  return chMakeShared<DX12SwapChain>(m_factory.Get(), m_queue.Get(), desc);
}

/*
 */
SPtr<IBuffer>
DX12API::createBuffer(const BufferCreateInfo& createInfo)
{
  CH_PARAMETER_UNUSED(createInfo);
  CH_EXCEPT(DX12ErrorException, "Buffers are not implemented yet.");
}

/*
 */
SPtr<ITexture>
DX12API::createTexture(const TextureCreateInfo& createInfo)
{
  CH_PARAMETER_UNUSED(createInfo);
  CH_EXCEPT(DX12ErrorException, "Textures are not implemented yet.");
}

/*
 */
SPtr<IShader>
DX12API::createShader(const ShaderCreateInfo& createInfo)
{
  CH_PARAMETER_UNUSED(createInfo);
  CH_EXCEPT(DX12ErrorException, "Shaders are not implemented yet.");
}

/*
 */
SPtr<IPipeline>
DX12API::createGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
  CH_PARAMETER_UNUSED(desc);
  CH_EXCEPT(DX12ErrorException, "Pipelines are not implemented yet.");
}

/*
 */
SPtr<ISampler>
DX12API::createSampler(const SamplerCreateInfo& createInfo)
{
  CH_PARAMETER_UNUSED(createInfo);
  CH_EXCEPT(DX12ErrorException, "Samplers are not implemented yet.");
}

/*
 */
ICommandList&
DX12API::beginFrame()
{
  FrameData& frame = m_frames[m_frameIndex];
  m_deletionQueue.waitForValue(frame.submitValue);

  DX12_CHECK(frame.allocator->Reset());
  frame.commandList->begin(frame.allocator.Get());
  return *frame.commandList;
}

/*
 */
void
DX12API::endFrame()
{
  FrameData& frame = m_frames[m_frameIndex];
  frame.commandList->end();

  const Array<ID3D12CommandList*, 1> commandLists = {frame.commandList->getHandle()};
  m_queue->ExecuteCommandLists(static_cast<UINT>(commandLists.size()), commandLists.data());

  frame.submitValue = m_deletionQueue.nextSubmitValue();
  DX12_CHECK(m_queue->Signal(m_deletionQueue.getFence(), frame.submitValue));

  m_frameIndex = (m_frameIndex + 1) % GraphicsLimits::MAX_FRAMES_IN_FLIGHT;
  m_deletionQueue.collect();
}

/*
 */
void
DX12API::waitIdle()
{
  // A fence of its own: signaling the frame fence would count as a submit and free objects
  // that commands recorded and not submitted yet still use. Signaled after any present, so
  // the swap chain buffers are free too.
  ++m_idleValue;
  DX12_CHECK(m_queue->Signal(m_idleFence.Get(), m_idleValue));
  // A null event makes the call block until the fence reaches the value.
  DX12_CHECK(m_idleFence->SetEventOnCompletion(m_idleValue, nullptr));
  m_deletionQueue.collect();
}

/*
 */
void
DX12API::logDeviceRemovedReason() const
{
  const HRESULT reason = m_device->GetDeviceRemovedReason();
  if (FAILED(reason)) {
    CH_LOG_ERROR(DX12, "The device was removed: 0x{0:X}", static_cast<uint32>(reason));
  }
}

/*
 */
void
DX12API::setDebugName(ID3D12Object* object, const ANSICHAR* name) const
{
  if (object == nullptr || name == nullptr) {
    return;
  }
  DX12_CHECK(object->SetName(UTF8::toWide(String(name)).c_str()));
}

/*
 */
void
DX12API::enableDebugLayer()
{
  ComPtr<ID3D12Debug> debug;
  if (FAILED(D3D12GetDebugInterface(IID_ID3D12Debug, outPtr(debug)))) {
    CH_LOG_WARNING(DX12, "The Direct3D 12 debug layer is not installed (Windows optional "
                         "feature \"Graphics Tools\").");
    return;
  }
  debug->EnableDebugLayer();
  m_debugLayerEnabled = true;
}

/*
 */
bool
DX12API::pickAdapter()
{
  for (UINT i = 0;; ++i) {
    ComPtr<IDXGIAdapter1> adapter;
    const HRESULT result = m_factory->EnumAdapterByGpuPreference(
        i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_IDXGIAdapter1, outPtr(adapter));
    if (result == DXGI_ERROR_NOT_FOUND) {
      return false;
    }
    DX12_CHECK(result);

    DXGI_ADAPTER_DESC1 desc{};
    DX12_CHECK(adapter->GetDesc1(&desc));
    if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) {
      continue;
    }
    const String adapterName = UTF8::fromWide(WString(desc.Description));

    ComPtr<ID3D12Device4> device;
    if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0,
                                 IID_ID3D12Device4,
                                 outPtr(device)))) {
      CH_LOG_WARNING(DX12, "Skipping {0}: it has no Direct3D 12 feature level 12_0 device.",
                     adapterName);
      continue;
    }
    if (!isDeviceSuitable(device.Get(), adapterName)) {
      continue;
    }

    m_adapter = std::move(adapter);
    m_device = std::move(device);
    m_adapterName = adapterName;
    setDebugName(m_device.Get(), "Device");
    return true;
  }
}

/*
 */
bool
DX12API::isDeviceSuitable(ID3D12Device* device, const String& adapterName) const
{
  String missing;
  auto require = [&missing](bool supported, const ANSICHAR* feature) {
    if (supported) {
      return;
    }
    if (!missing.empty()) {
      missing += ", ";
    }
    missing += feature;
  };

  // Shaders take every resource from ResourceDescriptorHeap and SamplerDescriptorHeap.
  D3D12_FEATURE_DATA_SHADER_MODEL shaderModel{.HighestShaderModel = D3D_SHADER_MODEL_6_6};
  require(SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel,
                                                sizeof(shaderModel))) &&
              shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_6,
          "Shader Model 6.6");

  D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
  require(SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options,
                                                sizeof(options))) &&
              options.ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_3,
          "resource binding tier 3");

  // Barriers state layouts like the engine's ResourceState, Undefined included.
  D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12{};
  require(SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options12,
                                                sizeof(options12))) &&
              options12.EnhancedBarriersSupported,
          "enhanced barriers");

  if (!missing.empty()) {
    CH_LOG_WARNING(DX12, "Skipping {0}: it lacks {1}.", adapterName, missing);
    return false;
  }
  return true;
}

/*
 */
void
DX12API::registerMessageCallback()
{
  // Older runtimes lack ID3D12InfoQueue1; their messages only reach the debugger output.
  ComPtr<ID3D12InfoQueue1> infoQueue;
  if (FAILED(m_device.As(&infoQueue))) {
    CH_LOG_WARNING(DX12, "Debug layer messages go only to the debugger output.");
    return;
  }
  DX12_CHECK(infoQueue->RegisterMessageCallback(&onDebugMessage,
                                                D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr,
                                                &m_messageCallbackCookie));
}

/*
 */
void
DX12API::createRootSignature()
{
  D3D12_ROOT_PARAMETER1 pushConstants{};
  pushConstants.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  pushConstants.Constants = {.ShaderRegister = 0,
                             .RegisterSpace = 0,
                             .Num32BitValues = GraphicsLimits::PUSH_CONSTANTS_SIZE / 4};
  pushConstants.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc{};
  desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
  desc.Desc_1_1 = {.NumParameters = 1,
                   .pParameters = &pushConstants,
                   .NumStaticSamplers = 0,
                   .pStaticSamplers = nullptr,
                   .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                            D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
                            D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED};

  ComPtr<ID3DBlob> blob;
  ComPtr<ID3DBlob> error;
  if (FAILED(D3D12SerializeVersionedRootSignature(&desc, &blob, &error))) {
    CH_EXCEPT(DX12ErrorException,
              StringUtils::format("Failed to build the root signature: {0}",
                                  error ? static_cast<const ANSICHAR*>(
                                              error->GetBufferPointer())
                                        : "unknown error"));
  }
  DX12_CHECK(m_device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
                                           IID_ID3D12RootSignature, outPtr(m_rootSignature)));
  setDebugName(m_rootSignature.Get(), "Bindless Root Signature");
}

/*
 */
void
DX12API::createFrames()
{
  for (uint32 i = 0; i < GraphicsLimits::MAX_FRAMES_IN_FLIGHT; ++i) {
    FrameData& frame = m_frames[i];
    DX12_CHECK(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                IID_ID3D12CommandAllocator,
                                                outPtr(frame.allocator)));
    frame.commandList = chMakeUnique<DX12CommandList>(
        m_device.Get(), m_rootSignature.Get(), m_resourceHeap.getHeap(),
        m_samplerHeap.getHeap());

    const String allocatorName = StringUtils::format("Frame Command Allocator {0}", i);
    setDebugName(frame.allocator.Get(), allocatorName.c_str());
    const String listName = StringUtils::format("Frame Command List {0}", i);
    setDebugName(frame.commandList->getHandle(), listName.c_str());
  }
}

/*
 */
DX12API&
g_dx12API()
{
  return static_cast<DX12API&>(IGraphicsAPI::instance());
}

} // namespace chEngineSDK
