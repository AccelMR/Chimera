/************************************************************************/
/**
 * @file chDX12TextureView.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of ITextureView.
 */
/************************************************************************/
#pragma once

#include "chDX12Prerequisites.h"

#include "chITextureView.h"

namespace chEngineSDK {
class DX12Texture;

/**
 * Part of a texture as the GPU sees it. Direct3D 12 has no view objects, so it holds the
 * descriptors its texture usage needs: a render target or depth target descriptor in the
 * CPU heaps of the API, and a shader resource descriptor in the bindless heap.
 */
class DX12TextureView : public ITextureView
{
 public:
  DX12TextureView(const DX12Texture& texture, const TextureViewCreateInfo& createInfo);
  ~DX12TextureView() override;

  DX12TextureView(const DX12TextureView&) = delete;
  DX12TextureView&
  operator=(const DX12TextureView&) = delete;

  NODISCARD Format
  getFormat() const override
  {
    return m_format;
  }

  NODISCARD TextureViewType
  getViewType() const override
  {
    return m_viewType;
  }

  NODISCARD uint32
  getBaseMipLevel() const override
  {
    return m_baseMipLevel;
  }

  NODISCARD uint32
  getMipLevelCount() const override
  {
    return m_mipLevelCount;
  }

  NODISCARD uint32
  getBaseArrayLayer() const override
  {
    return m_baseArrayLayer;
  }

  NODISCARD uint32
  getArrayLayerCount() const override
  {
    return m_arrayLayerCount;
  }

  NODISCARD uint32
  getBindlessIndex() const override
  {
    return m_bindlessIndex;
  }

  NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE
  getRenderTargetHandle() const;

  NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE
  getDepthTargetHandle() const;

 private:
  void
  createRenderTarget(ID3D12Resource* resource);

  void
  createDepthTarget(ID3D12Resource* resource);

  void
  createShaderResource(ID3D12Resource* resource);

  Format m_format = Format::Unknown;
  TextureViewType m_viewType = TextureViewType::View2D;
  uint32 m_baseMipLevel = 0;
  uint32 m_mipLevelCount = 1;
  uint32 m_baseArrayLayer = 0;
  uint32 m_arrayLayerCount = 1;
  uint32 m_renderTargetIndex = GraphicsLimits::INVALID_BINDLESS_INDEX;
  uint32 m_depthTargetIndex = GraphicsLimits::INVALID_BINDLESS_INDEX;
  uint32 m_bindlessIndex = GraphicsLimits::INVALID_BINDLESS_INDEX;
};

} // namespace chEngineSDK
