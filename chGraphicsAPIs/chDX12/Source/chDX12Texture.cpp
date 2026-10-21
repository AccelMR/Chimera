/************************************************************************/
/**
 * @file chDX12Texture.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of ITexture.
 */
/************************************************************************/
#include "chDX12Texture.h"

#include <D3D12MemAlloc.h>

#include "chDX12API.h"
#include "chDX12TextureView.h"

namespace chEngineSDK {
namespace {
NODISCARD TextureViewType
toDefaultViewType(TextureType type, uint32 arrayLayers)
{
  switch (type) {
  case TextureType::Texture1D:
    return arrayLayers > 1 ? TextureViewType::View1DArray : TextureViewType::View1D;
  case TextureType::Texture3D:
    return TextureViewType::View3D;
  case TextureType::TextureCube:
    return arrayLayers > 6 ? TextureViewType::ViewCubeArray : TextureViewType::ViewCube;
  case TextureType::Texture2D:
  default:
    return arrayLayers > 1 ? TextureViewType::View2DArray : TextureViewType::View2D;
  }
}

NODISCARD D3D12_RESOURCE_DIMENSION
toResourceDimension(TextureType type)
{
  switch (type) {
  case TextureType::Texture1D:
    return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
  case TextureType::Texture3D:
    return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
  case TextureType::Texture2D:
  case TextureType::TextureCube:
  default:
    // A cube is a 2D array of six faces per cube.
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  }
}

NODISCARD D3D12_RESOURCE_FLAGS
toResourceFlags(TextureUsageFlags usage)
{
  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
  if (usage.isSet(TextureUsage::ColorAttachment)) {
    flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  }
  if (usage.isSet(TextureUsage::DepthStencil)) {
    flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    // Lets the driver compress the depth further when shaders never read it.
    if (!usage.isSet(TextureUsage::Sampled)) {
      flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }
  }
  if (usage.isSet(TextureUsage::Storage)) {
    flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  }
  return flags;
}
} // namespace

/*
 */
DX12Texture::DX12Texture(D3D12MA::Allocator* allocator, const TextureCreateInfo& createInfo)
  : m_width(createInfo.width),
    m_height(createInfo.height),
    m_depth(createInfo.depth),
    m_mipLevels(createInfo.mipLevels),
    m_arrayLayers(createInfo.arrayLayers),
    m_format(createInfo.format),
    m_type(createInfo.type),
    m_usage(createInfo.usage),
    m_ownsTexture(true)
{
  const bool isDepth = FormatUtils::isDepth(m_format);
  const bool isSampled = m_usage.isSet(TextureUsage::Sampled);
  const DXGI_FORMAT resourceFormat = isDepth && isSampled ? getTypelessDepthFormat(m_format)
                                                          : chFormatToDxgiFormat(m_format);
  const bool is3D = m_type == TextureType::Texture3D;

  const D3D12_RESOURCE_DESC1 desc{
      .Dimension = toResourceDimension(m_type),
      .Alignment = 0,
      .Width = m_width,
      .Height = m_height,
      .DepthOrArraySize = static_cast<UINT16>(is3D ? m_depth : m_arrayLayers),
      .MipLevels = static_cast<UINT16>(m_mipLevels),
      .Format = resourceFormat,
      .SampleDesc = {.Count = static_cast<UINT>(createInfo.samples), .Quality = 0},
      .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
      .Flags = toResourceFlags(m_usage),
      .SamplerFeedbackMipRegion = {}};

  D3D12MA::ALLOCATION_DESC allocationDesc{};
  allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
  // Render targets are big and recreated on resize, so each gets memory of its own.
  if (m_usage.isSet(TextureUsage::ColorAttachment) ||
      m_usage.isSet(TextureUsage::DepthStencil)) {
    allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
  }

  // Every first use of a texture is a barrier from Undefined.
  DX12_CHECK(allocator->CreateResource3(&allocationDesc, &desc, D3D12_BARRIER_LAYOUT_UNDEFINED,
                                        nullptr, 0, nullptr, &m_allocation, IID_ID3D12Resource,
                                        outPtr(m_resource)));

  if (createInfo.initialData && createInfo.initialDataSize > 0) {
    uploadData(createInfo.initialData, createInfo.initialDataSize);
  }

  if (isSampled) {
    m_defaultView = createView({.format = Format::Unknown,
                                .viewType = toDefaultViewType(m_type, m_arrayLayers)});
  }
}

/*
 */
DX12Texture::DX12Texture(ComPtr<ID3D12Resource> resource,
                         Format format,
                         uint32 width,
                         uint32 height)
  : m_resource(std::move(resource)),
    m_width(width),
    m_height(height),
    m_format(format),
    m_usage(TextureUsage::ColorAttachment),
    m_ownsTexture(false)
{}

/*
 */
DX12Texture::~DX12Texture()
{
  // A swap chain buffer is released at once: the swap chain waits for the GPU before it
  // drops its buffers, and resizing them needs every reference gone.
  if (!m_ownsTexture) {
    return;
  }

  m_defaultView.reset();
  DX12DeletionQueue& deletionQueue = g_dx12API().getDeletionQueue();
  deletionQueue.enqueue(m_resource);
  deletionQueue.enqueueObject(m_allocation);
}

/*
 */
uint32
DX12Texture::getBindlessIndex() const
{
  return m_defaultView ? m_defaultView->getBindlessIndex()
                       : GraphicsLimits::INVALID_BINDLESS_INDEX;
}

/*
 */
SPtr<ITextureView>
DX12Texture::createView(const TextureViewCreateInfo& createInfo)
{
  return chMakeShared<DX12TextureView>(*this, createInfo);
}

/*
 */
void
DX12Texture::uploadData(const void* data, SIZE_T size)
{
  g_dx12API().getUploader().uploadTexture(*this, data, size);
}

} // namespace chEngineSDK
