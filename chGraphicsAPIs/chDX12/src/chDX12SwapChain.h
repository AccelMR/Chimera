/************************************************************************/
/**
 * @file chDX12SwapChain.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of ISwapChain.
 */
/************************************************************************/
#pragma once

#include "chDX12Prerequisites.h"

#include "chISwapChain.h"
#include "chDX12Texture.h"
#include "chDX12TextureView.h"

namespace chEngineSDK {

/**
 * DXGI flip model swap chain of one window. Presents run on the same queue as the frame
 * submits, after them, so the swap chain needs no synchronization objects of its own.
 */
class DX12SwapChain : public ISwapChain
{
 public:
  DX12SwapChain(IDXGIFactory6* factory, ID3D12CommandQueue* queue, const SwapChainDesc& desc);
  ~DX12SwapChain() override;

  DX12SwapChain(const DX12SwapChain&) = delete;
  DX12SwapChain&
  operator=(const DX12SwapChain&) = delete;

  NODISCARD SwapChainStatus
  acquireNextImage() override;

  NODISCARD SwapChainStatus
  present() override;

  void
  resize(uint32 width, uint32 height) override;

  NODISCARD const ITexture&
  getCurrentTexture() const override
  {
    return *m_textures[m_currentImageIndex];
  }

  NODISCARD const ITextureView&
  getCurrentTextureView() const override
  {
    return *m_textureViews[m_currentImageIndex];
  }

  NODISCARD uint32
  getTextureCount() const override
  {
    return BUFFER_COUNT;
  }

  NODISCARD Format
  getFormat() const override
  {
    return FORMAT;
  }

  NODISCARD uint32
  getWidth() const override
  {
    return m_width;
  }

  NODISCARD uint32
  getHeight() const override
  {
    return m_height;
  }

 private:
  void
  createTextures();

  void
  releaseTextures();

  static constexpr uint32 BUFFER_COUNT = 3;
  // Flip model swap chains take no sRGB format; every window shares this one, so the UI
  // draws all of them with one pipeline.
  static constexpr Format FORMAT = Format::B8G8R8A8_UNORM;

  ComPtr<IDXGISwapChain3> m_swapChain;
  HWND m_window = nullptr;
  uint32 m_width = 0;
  uint32 m_height = 0;
  uint32 m_currentImageIndex = 0;
  bool m_vsync = false;
  bool m_allowTearing = false;
  String m_debugName;

  Array<UniquePtr<DX12Texture>, BUFFER_COUNT> m_textures;
  Array<UniquePtr<DX12TextureView>, BUFFER_COUNT> m_textureViews;
};

} // namespace chEngineSDK
