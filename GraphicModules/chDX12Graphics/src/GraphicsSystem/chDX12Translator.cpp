/************************************************************************/
/**
 * @file chDX12Translator.cpp
 * @author AccelMR
 * @date 2022/11/01
 *    Here goes all the hard coded data so the access is O(1) to translate between
 *  Chimera and DX
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chDX12Translator.h"

#include "d3dx12.h"
#include <wrl/client.h>
#include "chDX12Map.h"

namespace chEngineSDK{ 
using Microsoft::WRL::ComPtr;

/*
*/
D3D12_INPUT_ELEMENT_DESC
DX12Translator::get(const VertexBufferBindingDesc& chDesc) {
  D3D12_INPUT_ELEMENT_DESC inputElemDesc {
    g_semanticIndex.at(static_cast<uint32>(chDesc.vertexSemantic)),
    0, //TODO: Make dynamic
    get(chDesc.format),
    chDesc.slot,
    chDesc.byteStride,
    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, //TODO: Make dynamic
    0//TODO: Make dynamic
  };
  return inputElemDesc;
}

/*
*/
ID3D12RootSignature*
DX12Translator::get(const BindingGroup& bindingGroups, ID3D12Device* device) {
// Create a root signature.
  D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
  featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
  if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, 
                                         &featureData, 
                                         sizeof(featureData))))
  {
      featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
  }
  
  // Allow input layout and deny unnecessary access to certain pipeline stages.
  D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
  
  // A single 32-bit constant root parameter that is used by the vertex shader.
  Vector<CD3DX12_ROOT_PARAMETER1> rootParameters;
  SIZE_T sizeParam = bindingGroups.textures.size() + bindingGroups.buffers.size();
  rootParameters.resize(sizeParam);
  
  uint32 i = 0;
  for (const auto& buffer: bindingGroups.buffers) {
    D3D12_SHADER_VISIBILITY shaderVisibility = DX12Translator::get(buffer.stages);
    rootParameters[i].InitAsConstantBufferView(buffer.slot);
    i++;
  }
  
  uint32 j = 0;
  for (const auto& texture: bindingGroups.textures) {
    D3D12_SHADER_VISIBILITY shaderVisibility = DX12Translator::get(texture.stages);
    rootParameters[i].InitAsShaderResourceView(bindingGroups.slot,
                                               0,
                                               D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
                                               shaderVisibility);
    j++;
  }

  CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
  if (rootParameters.empty()) {
  // Create the versioned root signature description
    rootSignatureDescription.Init_1_0(0, nullptr, 0, nullptr, rootSignatureFlags);
  }
  else {
    rootSignatureDescription.Init_1_1(rootParameters.size(),
                                      &rootParameters[0], 
                                      0, 
                                      nullptr, 
                                      rootSignatureFlags);
  }

  // Serialize the root signature.
  ComPtr<ID3DBlob> rootSignatureBlob;
  ComPtr<ID3DBlob> errorBlob;
  ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
                                                      featureData.HighestVersion, 
                                                      &rootSignatureBlob, 
                                                      &errorBlob));
  
  ID3D12RootSignature* ret;
  // Create the root signature.
  ThrowIfFailed(device->CreateRootSignature(0, 
                                            rootSignatureBlob->GetBufferPointer(),
                                            rootSignatureBlob->GetBufferSize(), 
                                            IID_PPV_ARGS(&ret)));
  
  return ret;
}

/*
*/
D3D12_SHADER_VISIBILITY
DX12Translator::get(ShaderStageFlag stages) {
  D3D12_SHADER_VISIBILITY ret = D3D12_SHADER_VISIBILITY_ALL;

  if (stages.isSetAny(chGPUDesc::SHADER_STAGE::kVERTEX)) {
    ret = D3D12_SHADER_VISIBILITY_VERTEX;
  }

  if (stages.isSetAny(chGPUDesc::SHADER_STAGE::kPIXEL)) {
    ret = D3D12_SHADER_VISIBILITY_PIXEL;
  }

  return ret;
}

/*
*/
D3D12_RASTERIZER_DESC
DX12Translator::get(const RasterizerStateDesc& rastDesc) {
  D3D12_RASTERIZER_DESC ret = {};
  ret.FillMode = DX12Translator::get(rastDesc.fillMode);
  ret.CullMode = DX12Translator::get(rastDesc.cullMode);
  ret.FrontCounterClockwise = rastDesc.frontCounterClockWise;
  ret.DepthBias = rastDesc.depthBias;
  ret.DepthBiasClamp = rastDesc.depthBiasClamp;
  ret.SlopeScaledDepthBias = rastDesc.slopeScaledDepthBias;
  ret.DepthClipEnable = rastDesc.depthClipEnable;
  ret.MultisampleEnable = rastDesc.multiSampleEnable;
  ret.AntialiasedLineEnable = rastDesc.antialiasedLineEnable;
  ret.ForcedSampleCount = rastDesc.forcedSampleCount;
  return ret;
}

/*
*/
D3D12_FILL_MODE
DX12Translator::get(const FILL_MODE& fillMode) {
  switch (fillMode)
  {
  default:
  case FILL_MODE::kWIREFRAME:
    return D3D12_FILL_MODE::D3D12_FILL_MODE_WIREFRAME;
    break;
  case FILL_MODE::kSOLID:
    return D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
    break;
  }
}

/*
*/
D3D12_CULL_MODE
DX12Translator::get(const CULL_MODE& cullMode) {
  switch (cullMode)
  {
  default:
    return D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
    break;
  case CULL_MODE::kNONE:
    return D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
    break;
  case CULL_MODE::kFRONT:
    return D3D12_CULL_MODE::D3D12_CULL_MODE_FRONT;
    break;
  case CULL_MODE::kBACK:
    return D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
    break;
  }
}

/*
*/
D3D12_BLEND_DESC
DX12Translator::get(const BlendStateDesc& blendState) {
  D3D12_BLEND_DESC blendDesc = {};
  blendDesc.AlphaToCoverageEnable = blendState.alphaToCoverageEnable;
  blendDesc.IndependentBlendEnable = blendState.independentBlendEnable;
  uint8 i = 0;
  for (auto rtBlendDesc: blendState.renderTargetBlendDesc) {
    blendDesc.RenderTarget[i].SrcBlend = DX12Translator::get(rtBlendDesc.srcBlend);
    blendDesc.RenderTarget[i].DestBlend = DX12Translator::get(rtBlendDesc.destBlend);
    blendDesc.RenderTarget[i].BlendOp = DX12Translator::get(rtBlendDesc.blendOP);
    blendDesc.RenderTarget[i].BlendOpAlpha = DX12Translator::get(rtBlendDesc.blenOpAlpha);
    blendDesc.RenderTarget[i].SrcBlendAlpha = DX12Translator::get(rtBlendDesc.srcsBlendAlpha);
    blendDesc.RenderTarget[i].DestBlendAlpha = DX12Translator::get(rtBlendDesc.destBlendAlpha);
    blendDesc.RenderTarget[i].LogicOp = DX12Translator::get(rtBlendDesc.logicOP);
    blendDesc.RenderTarget[i].RenderTargetWriteMask = rtBlendDesc.renderTargetWritemask;
    i++;
  }
  return blendDesc;
}

/*
*/
D3D12_BLEND
DX12Translator::get(const BLEND& blend) {
  switch (blend) {
  default:
  case BLEND::kBLEND_ZERO:
    return D3D12_BLEND::D3D12_BLEND_ZERO;
    break;
  case BLEND::kBLEND_ONE:
    return D3D12_BLEND::D3D12_BLEND_ONE;
    break;
  case BLEND::kBLEND_SRC_COLOR:
    return D3D12_BLEND::D3D12_BLEND_SRC_COLOR;
    break;
  case BLEND::kBLEND_INV_SRC_COLOR:
    return D3D12_BLEND::D3D12_BLEND_INV_SRC_COLOR;
    break;
  case BLEND::kBLEND_SRC_ALPHA:
    return D3D12_BLEND::D3D12_BLEND_SRC_ALPHA;
    break;
  case BLEND::kBLEND_INV_SRC_ALPHA:
    return D3D12_BLEND::D3D12_BLEND_INV_SRC_ALPHA;
    break;
  case BLEND::kBLEND_DEST_ALPHA:
    return D3D12_BLEND::D3D12_BLEND_DEST_ALPHA;
    break;
  case BLEND::kBLEND_INV_DEST_ALPHA:
    return D3D12_BLEND::D3D12_BLEND_INV_DEST_ALPHA;
    break;
  case BLEND::kBLEND_DEST_COLOR:
    return D3D12_BLEND::D3D12_BLEND_DEST_COLOR;
    break;
  case BLEND::kBLEND_INV_DEST_COLOR:
    return D3D12_BLEND::D3D12_BLEND_INV_DEST_COLOR;
    break;
  case BLEND::kBLEND_SRC_ALPHA_SAT:
    return D3D12_BLEND::D3D12_BLEND_SRC_ALPHA_SAT;
    break;
  case BLEND::kBLEND_BLEND_FACTOR:
    return D3D12_BLEND::D3D12_BLEND_BLEND_FACTOR;
    break;
  case BLEND::kBLEND_INV_BLEND_FACTOR:
    return D3D12_BLEND::D3D12_BLEND_INV_BLEND_FACTOR;;
    break;
  case BLEND::kBLEND_SRC1_COLOR:
    return D3D12_BLEND::D3D12_BLEND_SRC1_COLOR;
    break;
  case BLEND::kBLEND_INV_SRC1_COLOR:
    return D3D12_BLEND::D3D12_BLEND_INV_SRC1_COLOR;
    break;
  case BLEND::kBLEND_SRC1_ALPHA:
    return D3D12_BLEND::D3D12_BLEND_SRC1_ALPHA;
    break;
  case BLEND::kBLEND_INV_SRC1_ALPHA:
    return D3D12_BLEND::D3D12_BLEND_INV_SRC1_ALPHA;
    break;
  }
}

/*
*/
D3D12_BLEND_OP
DX12Translator::get(const BLEND_OP& blendOp) {
  switch (blendOp) {
  default:
  case BLEND_OP::kBLEND_OP_ADD:
    return D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
    break;
  case BLEND_OP::kBLEND_OP_SUBTRACT:
    return D3D12_BLEND_OP::D3D12_BLEND_OP_SUBTRACT;
    break;
  case BLEND_OP::kBLEND_OP_REV_SUBTRACT:
    return D3D12_BLEND_OP::D3D12_BLEND_OP_REV_SUBTRACT;
    break;
  case BLEND_OP::kBLEND_OP_MIN:
    return D3D12_BLEND_OP::D3D12_BLEND_OP_MIN;
    break;
  case BLEND_OP::kBLEND_OP_MAX:
    return D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
    break;
  }
}

/*
*/
D3D12_LOGIC_OP
DX12Translator::get(const LOGIC_OP& logicOp) {
  switch (logicOp)  {
  default:
  case LOGIC_OP::kLOGIC_OP_CLEAR:
    return D3D12_LOGIC_OP::D3D12_LOGIC_OP_CLEAR;
    break;
  case LOGIC_OP::kLOGIC_OP_SET:
    return D3D12_LOGIC_OP::D3D12_LOGIC_OP_SET;
    break;
  case LOGIC_OP::kLOGIC_OP_COPY:
    return D3D12_LOGIC_OP::D3D12_LOGIC_OP_COPY;
    break;
  case LOGIC_OP::kLOGIC_OP_COPY_INVERTED:
    return D3D12_LOGIC_OP::D3D12_LOGIC_OP_COPY_INVERTED;
    break;
  case LOGIC_OP::kLOGIC_OP_NOOP:
    return D3D12_LOGIC_OP::D3D12_LOGIC_OP_NOOP;
    break;
  case LOGIC_OP::kLOGIC_OP_INVERT:
    return D3D12_LOGIC_OP::D3D12_LOGIC_OP_INVERT;
    break;
  case LOGIC_OP::kLOGIC_OP_AND:
    return D3D12_LOGIC_OP::D3D12_LOGIC_OP_AND;
    break;
  case LOGIC_OP::kLOGIC_OP_NAND:
    return D3D12_LOGIC_OP::D3D12_LOGIC_OP_NAND;
    break;
  case LOGIC_OP::kLOGIC_OP_OR:
    return D3D12_LOGIC_OP::D3D12_LOGIC_OP_OR;
    break;
  case LOGIC_OP::kLOGIC_OP_NOR:
    return D3D12_LOGIC_OP::D3D12_LOGIC_OP_NOR;
    break;
  case LOGIC_OP::kLOGIC_OP_XOR:
    return D3D12_LOGIC_OP::D3D12_LOGIC_OP_XOR;
    break;
  case LOGIC_OP::kLOGIC_OP_EQUIV:
    return D3D12_LOGIC_OP::D3D12_LOGIC_OP_EQUIV;
    break;
  case LOGIC_OP::kLOGIC_OP_AND_REVERSE:
    return D3D12_LOGIC_OP::D3D12_LOGIC_OP_AND_REVERSE;
    break;
  case LOGIC_OP::kLOGIC_OP_AND_INVERTED:
    return D3D12_LOGIC_OP::D3D12_LOGIC_OP_AND_INVERTED;
    break;
  case LOGIC_OP::kLOGIC_OP_OR_REVERSE:
    return D3D12_LOGIC_OP::D3D12_LOGIC_OP_OR_REVERSE;
    break;
  case LOGIC_OP::kLOGIC_OP_OR_INVERTED:
    return D3D12_LOGIC_OP::D3D12_LOGIC_OP_OR_INVERTED;
    break;
  }
}

/*
*/
D3D12_PRIMITIVE_TOPOLOGY_TYPE
DX12Translator::get(const PRIMITIVE_TOPOLOGY_TYPE& topology) {
  switch (topology) {
  default:
  case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_UNDEFINED:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    break;
  case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_POINT:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    break;
  case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_LINE:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    break;
  case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    break;
  case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_PATCH:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    break;
  }
}

/*
*/
DXGI_FORMAT
DX12Translator::get(const FORMAT& format) {
  //TODO: hehe dont
  return static_cast<DXGI_FORMAT>(format);
}

/*
*/
D3D12_COMPARISON_FUNC
DX12Translator::get(const COMPARISON_FUNC& comparasion) {
  switch (comparasion) {
  default:
  case COMPARISON_FUNC::kNEVER:
    return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_NEVER;
    break;
  case COMPARISON_FUNC::kLESS:
    return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
    break;
  case COMPARISON_FUNC::kEQUAL:
    return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_EQUAL;
    break;
  case COMPARISON_FUNC::kLESS_EQUAL:
    return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS_EQUAL;
    break;
  case COMPARISON_FUNC::kGREATER:
    return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER;
    break;
  case COMPARISON_FUNC::kNOT_EQUAL:
    return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_NOT_EQUAL;
    break;
  case COMPARISON_FUNC::kGREATER_EQUAL:
    return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    break;
  case COMPARISON_FUNC::kALWAYS:
    return D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
    break;
  }
}

/*
*/
D3D12_DEPTH_STENCILOP_DESC
DX12Translator::get(const DepthStencilOpDesc& stencilOp) {
  D3D12_DEPTH_STENCILOP_DESC ret = {};
  if(stencilOp.stencilDepthFailOp != STENCIL_OP::kNONE){
    ret.StencilDepthFailOp = DX12Translator::get(stencilOp.stencilDepthFailOp);
  }

  if(stencilOp.stencilFailOp != STENCIL_OP::kNONE){
    ret.StencilFailOp = DX12Translator::get(stencilOp.stencilFailOp);
  }

  if(stencilOp.stencilPassOp != STENCIL_OP::kNONE) {
    ret.StencilPassOp = DX12Translator::get(stencilOp.stencilPassOp);
  }

  if(stencilOp.stencilFunc != COMPARISON_FUNC::kNONE) {
    ret.StencilFunc = DX12Translator::get(stencilOp.stencilFunc);
  }

  return  ret;
}

/*
*/
D3D12_STENCIL_OP
DX12Translator::get(const STENCIL_OP& stencilOp) {
  switch (stencilOp) {
  default:
  case STENCIL_OP::kKEEP:
    return D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
    break;
  case STENCIL_OP::kZERO:
    return D3D12_STENCIL_OP::D3D12_STENCIL_OP_ZERO;
    break;
  case STENCIL_OP::kREPLACE:
    return D3D12_STENCIL_OP::D3D12_STENCIL_OP_REPLACE;
    break;
  case STENCIL_OP::kINCR_SAT:
    return D3D12_STENCIL_OP::D3D12_STENCIL_OP_INCR_SAT;
    break;
  case STENCIL_OP::kDECR_SAT:
    return D3D12_STENCIL_OP::D3D12_STENCIL_OP_DECR_SAT;
    break;
  case STENCIL_OP::kINVERT:
    return D3D12_STENCIL_OP::D3D12_STENCIL_OP_INVERT;
    break;
  case STENCIL_OP::kINCR:
    return D3D12_STENCIL_OP::D3D12_STENCIL_OP_INCR;
    break;
  case STENCIL_OP::kDECR:
    return D3D12_STENCIL_OP::D3D12_STENCIL_OP_DECR;
    break;
  }
}

/*
*/
D3D12_RESOURCE_STATES
DX12Translator::get(const ResourceStates& resState) {
  switch (resState) {
  default:
  case ResourceStates::kRENDER_TARGET:
    return  D3D12_RESOURCE_STATE_RENDER_TARGET;
    break;
  case ResourceStates::kPRESENT:
    return D3D12_RESOURCE_STATE_PRESENT;
    break;
//   case ResourceStates::kPREDICATION:
//     break;
//   case ResourceStates::kVIDEO_DECODE_READ:
//     break;
//   case ResourceStates::kVIDEO_DECODE_WRITE:
//     break;
//   case ResourceStates::kVIDEO_PROCESS_READ:
//     break;
//   case ResourceStates::kVIDEO_PROCESS_WRITE:
//     break;
//   case ResourceStates::kVIDEO_ENCODE_READ:
//     break;
//   case ResourceStates::kVIDEO_ENCODE_WRITE:
//     break;
//   default:
//     CH_ASSERT(false);
  break;
  }
}

/*
*/
DXGI_SWAP_EFFECT
DX12Translator::get(const SWAPCHAIN_EFFECT& effect) {
  switch (effect) {
  default:
  case SWAPCHAIN_EFFECT::DISCARD:
    return DXGI_SWAP_EFFECT_DISCARD;
    break;
  case SWAPCHAIN_EFFECT::SEQUENTIAL:
  return DXGI_SWAP_EFFECT_SEQUENTIAL;
    break;
  case SWAPCHAIN_EFFECT::FLIP_SEQUENTIAL:
  return DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    break;
  case SWAPCHAIN_EFFECT::FLIP_DISCARD:
   return DXGI_SWAP_EFFECT_FLIP_DISCARD;
    break;
  }
}

/*
*/
D3D12_PRIMITIVE_TOPOLOGY
DX12Translator::get1(const PRIMITIVE_TOPOLOGY_TYPE& topology) {
  switch (topology) {
  default:
  case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_UNDEFINED:
    return D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    break;
  case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_POINT:
    return D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    break;
  case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_LINE:
    return D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    break;
  case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:
    return D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    break;
  case PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_PATCH:
    return D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
    break;
  }
}

}


