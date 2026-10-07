/************************************************************************/
/**
 * @file chDX12TextureView.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of ITextureView.
 */
/************************************************************************/
#include "chDX12TextureView.h"

#include "chDX12API.h"
#include "chDX12Texture.h"
#include "chMath.h"

namespace chEngineSDK {

/*
 */
DX12TextureView::DX12TextureView(const DX12Texture& texture,
                                 const TextureViewCreateInfo& createInfo)
  : m_format(createInfo.format != Format::Unknown ? createInfo.format : texture.getFormat()),
    m_viewType(createInfo.viewType),
    m_baseMipLevel(createInfo.baseMipLevel),
    m_baseArrayLayer(createInfo.baseArrayLayer)
{
  m_mipLevelCount = Math::min(createInfo.mipLevelCount,
                              texture.getMipLevels() - m_baseMipLevel);
  m_arrayLayerCount = Math::min(createInfo.arrayLayerCount,
                                texture.getArrayLayers() - m_baseArrayLayer);

  const TextureUsageFlags usage = texture.getUsage();
  if (usage.isSet(TextureUsage::ColorAttachment)) {
    createRenderTarget(texture.getHandle());
  }
  if (usage.isSet(TextureUsage::DepthStencil)) {
    createDepthTarget(texture.getHandle());
  }
}

/*
 */
DX12TextureView::~DX12TextureView()
{
  // Render and depth target descriptors are copied into the command list when it records
  // them, so they can be reused at once.
  DX12API& dx12API = g_dx12API();
  if (m_renderTargetIndex != GraphicsLimits::INVALID_BINDLESS_INDEX) {
    dx12API.getRenderTargetHeap().free(m_renderTargetIndex);
  }
  if (m_depthTargetIndex != GraphicsLimits::INVALID_BINDLESS_INDEX) {
    dx12API.getDepthTargetHeap().free(m_depthTargetIndex);
  }
}

/*
 */
D3D12_CPU_DESCRIPTOR_HANDLE
DX12TextureView::getRenderTargetHandle() const
{
  CH_ASSERT(m_renderTargetIndex != GraphicsLimits::INVALID_BINDLESS_INDEX);
  return g_dx12API().getRenderTargetHeap().getCpuHandle(m_renderTargetIndex);
}

/*
 */
D3D12_CPU_DESCRIPTOR_HANDLE
DX12TextureView::getDepthTargetHandle() const
{
  CH_ASSERT(m_depthTargetIndex != GraphicsLimits::INVALID_BINDLESS_INDEX);
  return g_dx12API().getDepthTargetHeap().getCpuHandle(m_depthTargetIndex);
}

/*
 */
void
DX12TextureView::createRenderTarget(ID3D12Resource* resource)
{
  D3D12_RENDER_TARGET_VIEW_DESC desc{};
  desc.Format = chFormatToDxgiFormat(m_format);
  if (m_viewType == TextureViewType::View2DArray) {
    desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
    desc.Texture2DArray = {.MipSlice = m_baseMipLevel,
                           .FirstArraySlice = m_baseArrayLayer,
                           .ArraySize = m_arrayLayerCount,
                           .PlaneSlice = 0};
  }
  else {
    desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    desc.Texture2D = {.MipSlice = m_baseMipLevel, .PlaneSlice = 0};
  }

  DX12API& dx12API = g_dx12API();
  DX12DescriptorHeap& heap = dx12API.getRenderTargetHeap();
  m_renderTargetIndex = heap.allocate();
  dx12API.getDevice()->CreateRenderTargetView(resource, &desc,
                                              heap.getCpuHandle(m_renderTargetIndex));
}

/*
 */
void
DX12TextureView::createDepthTarget(ID3D12Resource* resource)
{
  D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
  desc.Format = chFormatToDxgiFormat(m_format);
  desc.Flags = D3D12_DSV_FLAG_NONE;
  if (m_viewType == TextureViewType::View2DArray) {
    desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
    desc.Texture2DArray = {.MipSlice = m_baseMipLevel,
                           .FirstArraySlice = m_baseArrayLayer,
                           .ArraySize = m_arrayLayerCount};
  }
  else {
    desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    desc.Texture2D = {.MipSlice = m_baseMipLevel};
  }

  DX12API& dx12API = g_dx12API();
  DX12DescriptorHeap& heap = dx12API.getDepthTargetHeap();
  m_depthTargetIndex = heap.allocate();
  dx12API.getDevice()->CreateDepthStencilView(resource, &desc,
                                              heap.getCpuHandle(m_depthTargetIndex));
}

} // namespace chEngineSDK
