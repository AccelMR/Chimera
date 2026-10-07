/************************************************************************/
/**
 * @file chDX12Pipeline.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of IPipeline.
 */
/************************************************************************/
#pragma once

#include "chDX12Prerequisites.h"

#include "chIPipeline.h"

namespace chEngineSDK {

/**
 * Graphics pipeline state on the root signature every pipeline shares. Direct3D 12 sets
 * the topology and the vertex strides with the vertex buffers, not in the pipeline, so it
 * keeps them for the command list.
 */
class DX12Pipeline : public IPipeline
{
 public:
  static constexpr uint32 MAX_VERTEX_BINDINGS = 8;

  DX12Pipeline(ID3D12Device* device,
               ID3D12RootSignature* rootSignature,
               const GraphicsPipelineDesc& desc);
  ~DX12Pipeline() override;

  DX12Pipeline(const DX12Pipeline&) = delete;
  DX12Pipeline&
  operator=(const DX12Pipeline&) = delete;

  NODISCARD FORCEINLINE ID3D12PipelineState*
  getHandle() const
  {
    return m_pipelineState.Get();
  }

  NODISCARD FORCEINLINE D3D_PRIMITIVE_TOPOLOGY
  getTopology() const
  {
    return m_topology;
  }

  NODISCARD FORCEINLINE uint32
  getStride(uint32 binding) const
  {
    return m_strides[binding];
  }

 private:
  ComPtr<ID3D12PipelineState> m_pipelineState;
  D3D_PRIMITIVE_TOPOLOGY m_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  Array<uint32, MAX_VERTEX_BINDINGS> m_strides{};
};

} // namespace chEngineSDK
