/************************************************************************/
/**
 * @file chDX12Shader.h
 * @author AccelMR
 * @date 2022/10/31
 *    DirectX 12 implementation of a shader. 
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
#include <chShader.h>


namespace chEngineSDK {
  using namespace chGPUDesc;
class DX12Shader final: public Shader
{
 public:
 /*
  *   Default constructor
  */
  DX12Shader();

 /*
  *   Default destructor
  */
  ~DX12Shader();

 protected:
  friend class DX12GPUResourceModule;
  friend class DX12GPUPipelineState;

  void
  _init(const ShaderDesc& desc);

  ID3DBlob* m_vertexShader;
  ID3DBlob* m_pixelShader;
  ID3DBlob* m_computeShader;
  ID3DBlob* m_meshShader;

 
};
}

