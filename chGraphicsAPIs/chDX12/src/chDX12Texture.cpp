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

#include "chDX12API.h"
#include "chDX12TextureView.h"

namespace chEngineSDK {

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
  if (m_ownsTexture) {
    g_dx12API().getDeletionQueue().enqueue(m_resource);
  }
}

/*
 */
uint32
DX12Texture::getBindlessIndex() const
{
  return GraphicsLimits::INVALID_BINDLESS_INDEX;
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
  CH_PARAMETER_UNUSED(data);
  CH_PARAMETER_UNUSED(size);
  CH_EXCEPT(DX12ErrorException, "Texture uploads are not implemented yet.");
}

} // namespace chEngineSDK
