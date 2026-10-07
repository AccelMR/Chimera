/************************************************************************/
/**
 * @file chDX12Sampler.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of ISampler.
 */
/************************************************************************/
#include "chDX12Sampler.h"

#include "chDX12API.h"
#include "chMath.h"

namespace chEngineSDK {
namespace {
NODISCARD D3D12_FILTER_TYPE
toFilterType(SamplerFilter filter)
{
  return filter == SamplerFilter::Linear ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT;
}

NODISCARD D3D12_TEXTURE_ADDRESS_MODE
toAddressMode(SamplerAddressMode addressMode)
{
  switch (addressMode) {
  case SamplerAddressMode::MirroredRepeat:
    return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
  case SamplerAddressMode::ClampToEdge:
    return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  case SamplerAddressMode::ClampToBorder:
    return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  case SamplerAddressMode::MirrorClampToEdge:
    return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
  case SamplerAddressMode::Repeat:
  default:
    return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  }
}

NODISCARD D3D12_FILTER
toFilter(const SamplerCreateInfo& createInfo)
{
  if (createInfo.anisotropyEnable) {
    return createInfo.compareEnable ? D3D12_FILTER_COMPARISON_ANISOTROPIC
                                    : D3D12_FILTER_ANISOTROPIC;
  }

  const D3D12_FILTER_TYPE mipFilter = createInfo.mipmapMode == SamplerMipmapMode::Linear
                                          ? D3D12_FILTER_TYPE_LINEAR
                                          : D3D12_FILTER_TYPE_POINT;
  const D3D12_FILTER_REDUCTION_TYPE reduction = createInfo.compareEnable
                                                    ? D3D12_FILTER_REDUCTION_TYPE_COMPARISON
                                                    : D3D12_FILTER_REDUCTION_TYPE_STANDARD;
  const D3D12_FILTER_TYPE minFilter = toFilterType(createInfo.minFilter);
  const D3D12_FILTER_TYPE magFilter = toFilterType(createInfo.magFilter);
  return static_cast<D3D12_FILTER>(
      D3D12_ENCODE_BASIC_FILTER(minFilter, magFilter, mipFilter, reduction));
}
} // namespace

/*
 */
DX12Sampler::DX12Sampler(const SamplerCreateInfo& createInfo)
{
  const LinearColor& border = createInfo.borderColor;
  const D3D12_SAMPLER_DESC desc{
      .Filter = toFilter(createInfo),
      .AddressU = toAddressMode(createInfo.addressModeU),
      .AddressV = toAddressMode(createInfo.addressModeV),
      .AddressW = toAddressMode(createInfo.addressModeW),
      .MipLODBias = createInfo.mipLodBias,
      .MaxAnisotropy = static_cast<UINT>(Math::clamp(createInfo.maxAnisotropy, 1.0f, 16.0f)),
      .ComparisonFunc = createInfo.compareEnable ? chCompareOpToD3D12(createInfo.compareOp)
                                                 : D3D12_COMPARISON_FUNC_NEVER,
      .BorderColor = {border.r, border.g, border.b, border.a},
      .MinLOD = createInfo.minLod,
      .MaxLOD = createInfo.maxLod};

  DX12API& dx12API = g_dx12API();
  DX12DescriptorHeap& heap = dx12API.getSamplerHeap();
  m_bindlessIndex = heap.allocate();
  dx12API.getDevice()->CreateSampler(&desc, heap.getCpuHandle(m_bindlessIndex));
}

/*
 */
DX12Sampler::~DX12Sampler()
{
  DX12API& dx12API = g_dx12API();
  dx12API.getDeletionQueue().enqueueDescriptor(dx12API.getSamplerHeap(), m_bindlessIndex);
}

} // namespace chEngineSDK
