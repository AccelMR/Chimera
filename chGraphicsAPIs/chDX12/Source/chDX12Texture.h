/************************************************************************/
/**
 * @file chDX12Texture.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of ITexture.
 */
/************************************************************************/
#pragma once

#include "chDX12Prerequisites.h"

#include "chITexture.h"

namespace chEngineSDK {

/**
 * Direct3D 12 texture whose memory comes from D3D12MA, released through the deletion
 * queue. Swap chain buffers are wrapped too: the swap chain makes them and must take every
 * reference back before it resizes them.
 */
class DX12Texture : public ITexture
{
 public:
  DX12Texture(D3D12MA::Allocator* allocator, const TextureCreateInfo& createInfo);

  /**
   * Wraps a swap chain buffer.
   */
  DX12Texture(ComPtr<ID3D12Resource> resource, Format format, uint32 width, uint32 height);

  ~DX12Texture() override;

  DX12Texture(const DX12Texture&) = delete;
  DX12Texture&
  operator=(const DX12Texture&) = delete;

  NODISCARD TextureType
  getType() const override
  {
    return m_type;
  }

  NODISCARD Format
  getFormat() const override
  {
    return m_format;
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

  NODISCARD uint32
  getDepth() const override
  {
    return m_depth;
  }

  NODISCARD uint32
  getMipLevels() const override
  {
    return m_mipLevels;
  }

  NODISCARD uint32
  getArrayLayers() const override
  {
    return m_arrayLayers;
  }

  NODISCARD uint32
  getBindlessIndex() const override;

  NODISCARD SPtr<ITextureView>
  createView(const TextureViewCreateInfo& createInfo = {}) override;

  void
  uploadData(const void* data, SIZE_T size) override;

  NODISCARD FORCEINLINE ID3D12Resource*
  getHandle() const
  {
    return m_resource.Get();
  }

  NODISCARD FORCEINLINE TextureUsageFlags
  getUsage() const
  {
    return m_usage;
  }

 private:
  ComPtr<ID3D12Resource> m_resource;
  D3D12MA::Allocation* m_allocation = nullptr;
  uint32 m_width = 0;
  uint32 m_height = 0;
  uint32 m_depth = 1;
  uint32 m_mipLevels = 1;
  uint32 m_arrayLayers = 1;
  Format m_format = Format::Unknown;
  TextureType m_type = TextureType::Texture2D;
  TextureUsageFlags m_usage;
  bool m_ownsTexture = true;
  // The whole texture as shaders read it; it holds the bindless index of the texture.
  SPtr<ITextureView> m_defaultView;
};

} // namespace chEngineSDK
