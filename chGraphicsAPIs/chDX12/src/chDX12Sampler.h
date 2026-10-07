/************************************************************************/
/**
 * @file chDX12Sampler.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of ISampler.
 */
/************************************************************************/
#pragma once

#include "chDX12Prerequisites.h"

#include "chISampler.h"

namespace chEngineSDK {

/**
 * Direct3D 12 samplers are only descriptors, so a sampler is its slot in the bindless
 * sampler heap.
 */
class DX12Sampler : public ISampler
{
 public:
  explicit DX12Sampler(const SamplerCreateInfo& createInfo);
  ~DX12Sampler() override;

  DX12Sampler(const DX12Sampler&) = delete;
  DX12Sampler&
  operator=(const DX12Sampler&) = delete;

  NODISCARD uint32
  getBindlessIndex() const override
  {
    return m_bindlessIndex;
  }

 private:
  uint32 m_bindlessIndex = GraphicsLimits::INVALID_BINDLESS_INDEX;
};

} // namespace chEngineSDK
