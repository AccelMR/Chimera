/************************************************************************/
/**
 * @file chDX12SwapChain.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of ISwapChain.
 */
/************************************************************************/
#include "chDX12SwapChain.h"

#include "chDX12API.h"
#include "chMath.h"

#if USING(CH_DISPLAY_SDL3)
# include <SDL3/SDL_properties.h>
# include <SDL3/SDL_video.h>
#endif // USING(CH_DISPLAY_SDL3)

namespace chEngineSDK {
namespace {
NODISCARD HWND
getWindowHandle(PlatformDisplay window)
{
  CH_ASSERT(window != nullptr);
#if USING(CH_DISPLAY_SDL3)
  return static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                                  SDL_PROP_WINDOW_WIN32_HWND_POINTER,
                                                  nullptr));
#else
  return static_cast<HWND>(window);
#endif // USING(CH_DISPLAY_SDL3)
}

struct WindowSize
{
  uint32 width = 0;
  uint32 height = 0;
};

NODISCARD WindowSize
getClientSize(HWND window)
{
  RECT rect{};
  if (!GetClientRect(window, &rect)) {
    return {};
  }
  return {static_cast<uint32>(rect.right - rect.left),
          static_cast<uint32>(rect.bottom - rect.top)};
}

NODISCARD bool
isTearingSupported(IDXGIFactory6* factory)
{
  BOOL allowTearing = FALSE;
  const HRESULT result = factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                                      &allowTearing, sizeof(allowTearing));
  return SUCCEEDED(result) && allowTearing == TRUE;
}
} // namespace

/*
 */
DX12SwapChain::DX12SwapChain(IDXGIFactory6* factory,
                             ID3D12CommandQueue* queue,
                             const SwapChainDesc& desc)
  : m_window(getWindowHandle(desc.window)),
    m_vsync(desc.vsync),
    m_allowTearing(isTearingSupported(factory)),
    m_debugName(desc.debugName)
{
  if (m_window == nullptr) {
    CH_EXCEPT(DX12ErrorException,
              StringUtils::format("{0} has no native window.", m_debugName));
  }

  // The window decides the size, as a surface does in the other graphics APIs; the size
  // asked for is only used while the window reports none yet.
  const WindowSize clientSize = getClientSize(m_window);
  m_width = Math::max(clientSize.width != 0 ? clientSize.width : desc.width, 1u);
  m_height = Math::max(clientSize.height != 0 ? clientSize.height : desc.height, 1u);

  const DXGI_SWAP_CHAIN_DESC1 swapChainDesc{
      .Width = m_width,
      .Height = m_height,
      .Format = chFormatToDxgiFormat(FORMAT),
      .Stereo = FALSE,
      .SampleDesc = {.Count = 1, .Quality = 0},
      .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
      .BufferCount = BUFFER_COUNT,
      .Scaling = DXGI_SCALING_STRETCH,
      .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
      .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
      .Flags = m_allowTearing ? static_cast<UINT>(DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) : 0u};

  ComPtr<IDXGISwapChain1> swapChain;
  DX12_CHECK(factory->CreateSwapChainForHwnd(queue, m_window, &swapChainDesc, nullptr, nullptr,
                                             &swapChain));
  DX12_CHECK(swapChain.As(&m_swapChain));
  // The window layer handles full screen, so DXGI must not switch it on Alt+Enter.
  DX12_CHECK(factory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));

  createTextures();
}

/*
 */
DX12SwapChain::~DX12SwapChain()
{
  // The buffers may still be used by frames in flight.
  g_dx12API().waitIdle();
  releaseTextures();
}

/*
 */
SwapChainStatus
DX12SwapChain::acquireNextImage()
{
  m_currentImageIndex = m_swapChain->GetCurrentBackBufferIndex();
  return SwapChainStatus::Ready;
}

/*
 */
SwapChainStatus
DX12SwapChain::present()
{
  // Tearing lets an image show at once, which is what no vsync means in a window.
  const UINT syncInterval = m_vsync ? 1 : 0;
  const UINT flags = (!m_vsync && m_allowTearing) ? DXGI_PRESENT_ALLOW_TEARING : 0;
  const HRESULT result = m_swapChain->Present(syncInterval, flags);
  if (FAILED(result)) {
    CH_LOG_ERROR(DX12, "Failed to present {0}: 0x{1:X}", m_debugName,
                 static_cast<uint32>(result));
    g_dx12API().logDeviceRemovedReason();
    return SwapChainStatus::Failed;
  }
  return SwapChainStatus::Ready;
}

/*
 */
void
DX12SwapChain::resize(uint32 width, uint32 height)
{
  // A minimized window has no size; the buffers are kept until it is restored. The window
  // decides the new size, as in the constructor.
  const WindowSize clientSize = getClientSize(m_window);
  if (width == 0 || height == 0 || clientSize.width == 0 || clientSize.height == 0 ||
      (clientSize.width == m_width && clientSize.height == m_height)) {
    return;
  }

  // ResizeBuffers fails while the GPU or anyone else still holds a buffer.
  g_dx12API().waitIdle();
  releaseTextures();

  const UINT flags =
      m_allowTearing ? static_cast<UINT>(DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) : 0u;
  DX12_CHECK(m_swapChain->ResizeBuffers(BUFFER_COUNT, clientSize.width, clientSize.height,
                                        chFormatToDxgiFormat(FORMAT), flags));
  m_width = clientSize.width;
  m_height = clientSize.height;
  createTextures();
}

/*
 */
void
DX12SwapChain::createTextures()
{
  const DX12API& dx12API = g_dx12API();
  for (uint32 i = 0; i < BUFFER_COUNT; ++i) {
    ComPtr<ID3D12Resource> buffer;
    DX12_CHECK(m_swapChain->GetBuffer(i, IID_ID3D12Resource, outPtr(buffer)));
    const String name = StringUtils::format("{0} Image {1}", m_debugName, i);
    dx12API.setDebugName(buffer.Get(), name.c_str());

    m_textures[i] = chMakeUnique<DX12Texture>(std::move(buffer), FORMAT, m_width, m_height);
    m_textureViews[i] = chMakeUnique<DX12TextureView>(*m_textures[i],
                                                      TextureViewCreateInfo{});
  }
  m_currentImageIndex = m_swapChain->GetCurrentBackBufferIndex();
}

/*
 */
void
DX12SwapChain::releaseTextures()
{
  for (uint32 i = 0; i < BUFFER_COUNT; ++i) {
    m_textureViews[i].reset();
    m_textures[i].reset();
  }
}

} // namespace chEngineSDK
