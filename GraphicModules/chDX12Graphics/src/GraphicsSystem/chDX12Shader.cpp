/************************************************************************/
/**
 * @file chDX12Shader.cpp
 * @author AccelMR
 * @date 2022/10/31
 *    DirectX 12 implementation of a shader.
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chDX12Shader.h"

#include <chStringUtils.h>
#include <chDebug.h>


namespace chEngineSDK {
  using namespace chGPUDesc;
bool 
compileShaderDX(const Vector<uint8>& dataStream, 
                SIZE_T size, 
                const String& name, 
                const String& entryFunc, 
                const String& shaderModel,
                uint32 flags,
                ID3DBlob** shaderBlob) {
  ID3DBlob* outError = nullptr;

  HRESULT hr = D3DCompile(&dataStream[0],
                          size,
                          name.c_str(),
                          nullptr,
                          nullptr,
                          entryFunc.c_str(),
                          shaderModel.c_str(),
                          flags,
                          0,
                          shaderBlob,
                          &outError );
  if (FAILED(hr)) {
    String error(static_cast<ANSICHAR*>(outError->GetBufferPointer()));
    CH_LOG_ERROR(StringUtils::format("Can't compile {0}. {1}", name, error));
    return false;
  }

  return true;
}
/*
*/
DX12Shader::DX12Shader()
  : m_vertexShader(nullptr), 
    m_pixelShader(nullptr),
    m_computeShader(nullptr),
    m_meshShader(nullptr)
{}

/*
*/
DX12Shader::~DX12Shader() {
  SafeRelease(m_vertexShader);
  SafeRelease(m_pixelShader);
  SafeRelease(m_meshShader);
  SafeRelease(m_computeShader);
}

/*
*/
void
DX12Shader::_init(const ShaderDesc& desc) {
#if USING(CH_DEBUG_MODE)
  // Enable better shader debugging with the graphics debugging tools.
  UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else //#if USING(CH_DEBUG_MODE)
  UINT compileFlags = 0;
#endif //#else #if USING(CH_DEBUG_MODE)
  m_name = desc.name;

  if (!desc.VS.byteCode.empty()) {
    const bool VSCompiled = compileShaderDX(desc.VS.byteCode, 
                                            desc.VS.byteCode.size(), 
                                            desc.name.getName().c_str(),
                                            desc.VS.entryFunc,
                                            "vs_5_0",
                                            compileFlags,
                                            &m_vertexShader);

    if (!VSCompiled) {
      CH_EXCEPT(InternalErrorException, "Shader not loaded");
    }
  }

  if (!desc.PS.byteCode.empty()) {
    const bool PSCompiled = compileShaderDX(desc.PS.byteCode,
                                            desc.PS.byteCode.size(),
                                            desc.name.getName().c_str(),
                                            desc.PS.entryFunc,
                                            "ps_5_0",
                                            compileFlags,
                                            &m_pixelShader);

    if (!PSCompiled) {
      CH_EXCEPT(InternalErrorException,"Shader not loaded");
    }
  }

  if (!desc.CS.byteCode.empty()) {
  const bool CSCompiled = compileShaderDX(desc.CS.byteCode,
                                          desc.CS.byteCode.size(),
                                          desc.name.getName().c_str(),
                                          desc.CS.entryFunc,
                                          "cs_5_0",
                                          compileFlags,
                                          &m_computeShader);
    if (!CSCompiled) {

    }
  }

  if (!desc.MS.byteCode.empty()) {
  const bool MSCompiled = compileShaderDX(desc.MS.byteCode,
                                          desc.MS.byteCode.size(),
                                          desc.name.getName().c_str(),
                                          desc.MS.entryFunc,
                                          "ms_5_0",
                                          compileFlags,
                                          &m_meshShader);
    if (!MSCompiled) {

    }
  }

}

}


