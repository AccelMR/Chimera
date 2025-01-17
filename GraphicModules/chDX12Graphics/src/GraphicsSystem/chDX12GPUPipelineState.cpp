/************************************************************************/
/**
 * @file chDX12GPUPipelineState.cpp
 * @author AccelMR
 * @date 2022/09/29
 *
 *    DirectX 12 specific pipeline state implementation.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chDX12GPUPipelineState.h"

#include "chGPUResourceDescriptors.h

#include "chDX12Translator.h"
#include "chDX12Shader.h"

namespace chEngineSDK {
/*
*/
DX12GPUPipelineState::~DX12GPUPipelineState() {
  SafeRelease(m_pipeline);
}

/*
*/
void
DX12GPUPipelineState::_init(const chGPUDesc::PipelineStateDesc& desc, ID3D12Device* device) {

  Vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
  inputElementDescs.resize(desc.vertexBufferBindingsDesc.size());

  for (uint32 i = 0; i < 
       std::min(inputElementDescs.size(), desc.vertexBufferBindingsDesc.size()); 
       ++i) {
    inputElementDescs[i] = DX12Translator::get(desc.vertexBufferBindingsDesc[i]);
  }

  //Not sure if we must force this or not here anyways. 
  CH_ASSERT(desc.VS);
  CH_ASSERT(desc.PS);

  auto dx12VSshader = std::reinterpret_pointer_cast<DX12Shader>(desc.VS);
  auto dx12PSshader = std::reinterpret_pointer_cast<DX12Shader>(desc.PS);

  if (desc.MS) {//TODO:
  }

  if (desc.CS) {//TODO:
  }

  // Describe and create the graphics pipeline state object (PSO).
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.InputLayout = {&inputElementDescs[0], static_cast<UINT>(inputElementDescs.size())};
  m_rs = DX12Translator::get(desc.bindingGroups, device);
  psoDesc.pRootSignature = m_rs;
  psoDesc.VS = D3D12_SHADER_BYTECODE({dx12VSshader->m_vertexShader->GetBufferPointer(), 
                                      dx12VSshader->m_vertexShader->GetBufferSize()
                                     });
  psoDesc.PS = D3D12_SHADER_BYTECODE({dx12PSshader->m_pixelShader->GetBufferPointer(),
                                      dx12PSshader->m_pixelShader->GetBufferSize()
                                     });
  //TODO: add other shaders

  psoDesc.RasterizerState = DX12Translator::get(desc.rasterizerStateDesc);
  psoDesc.BlendState = DX12Translator::get(desc.blendState);
  psoDesc.DepthStencilState.DepthEnable = desc.depthStencilStateDesc.depthEnable;
  psoDesc.DepthStencilState.StencilEnable = desc.depthStencilStateDesc.stencilEnable;
  if (desc.depthStencilStateDesc.DepthFunc != chGPUDesc::COMPARISON_FUNC::kNONE) {
    psoDesc.DepthStencilState.DepthFunc = DX12Translator::get(desc.depthStencilStateDesc.DepthFunc);
  }
  psoDesc.DepthStencilState.StencilEnable = desc.depthStencilStateDesc.stencilEnable;
  psoDesc.DepthStencilState.StencilReadMask = desc.depthStencilStateDesc.stencilReadMask;
  psoDesc.DepthStencilState.StencilWriteMask = desc.depthStencilStateDesc.stencilWriteMask;
  psoDesc.DepthStencilState.FrontFace = DX12Translator::get(desc.depthStencilStateDesc.FrontFace);
  psoDesc.DepthStencilState.BackFace = DX12Translator::get(desc.depthStencilStateDesc.BackFace);

  psoDesc.SampleMask = desc.sampleMask;
  psoDesc.PrimitiveTopologyType = DX12Translator::get(desc.topology);
  psoDesc.NumRenderTargets = desc.numRenderTextures;
  for (uint32 i = 0; i < desc.numRenderTextures; i++) {
    psoDesc.RTVFormats[i] = DX12Translator::get(desc.renderTextureFormats[i]); 
  }
  psoDesc.SampleDesc.Count = 1;
  psoDesc.SampleMask = UINT_MAX;
  ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, 
                                                    __uuidof(m_pipeline),
                                                    reinterpret_cast<void**>(&m_pipeline)));
}

}


