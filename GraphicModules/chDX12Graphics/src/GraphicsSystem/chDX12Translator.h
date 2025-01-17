/************************************************************************/
/**
 * @file chDX12Translator.h
 * @author AccelMR
 * @date 2022/11/01
 *    Here goes all the hard coded data so the access is O(1) to translate between
 *  Chimera and DX 
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesDX12.h"

#include <chResourceDescriptors.h>

namespace chEngineSDK{
 using namespace chGPUDesc;

class DX12Translator
{
 public:
  
  static D3D12_INPUT_ELEMENT_DESC
  get(const VertexBufferBindingDesc& chDesc);

  static ID3D12RootSignature*
  get(const BindingGroup& bindingGroups, ID3D12Device* device);

  static D3D12_SHADER_VISIBILITY
  get(ShaderStageFlag stages);

  static D3D12_RASTERIZER_DESC
  get(const RasterizerStateDesc& rastDesc);

  static D3D12_FILL_MODE
  get(const FILL_MODE& fillMode);
  
  static D3D12_CULL_MODE
  get(const CULL_MODE& cullMOde);

  static D3D12_BLEND_DESC
  get(const BlendStateDesc& blendState);
  
  static D3D12_BLEND
  get(const BLEND& blend);
    
  static D3D12_BLEND_OP
  get(const BLEND_OP& blendOp);

  static D3D12_LOGIC_OP
  get(const LOGIC_OP& logicOp);

  static D3D12_PRIMITIVE_TOPOLOGY_TYPE
  get(const PRIMITIVE_TOPOLOGY_TYPE& topology);

  //TODO
  static D3D12_PRIMITIVE_TOPOLOGY
  get1(const PRIMITIVE_TOPOLOGY_TYPE& topology);

  static DXGI_FORMAT
  get(const FORMAT& format);

  static D3D12_COMPARISON_FUNC
  get(const COMPARISON_FUNC& format);

  static D3D12_DEPTH_STENCILOP_DESC
  get(const DepthStencilOpDesc& depthStencilDesc);

  static D3D12_STENCIL_OP
  get(const STENCIL_OP& stencilOp);

  static D3D12_RESOURCE_STATES
  get(const ResourceStates& resState);

  static DXGI_SWAP_EFFECT
  get(const SWAPCHAIN_EFFECT& effect);
 
};
}

