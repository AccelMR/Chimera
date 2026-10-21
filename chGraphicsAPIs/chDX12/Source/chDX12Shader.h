/************************************************************************/
/**
 * @file chDX12Shader.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of IShader.
 */
/************************************************************************/
#pragma once

#include "chDX12Prerequisites.h"

#include "chIShader.h"
#include "chUUID.h"

namespace chEngineSDK {

/**
 * Compiled DXIL of one entry point. Direct3D 12 has no shader objects; pipelines read the
 * bytecode when they are built.
 */
class DX12Shader : public IShader
{
 public:
  explicit DX12Shader(const ShaderCreateInfo& createInfo);

  NODISCARD UUID
  getShaderId() const override
  {
    return m_shaderId;
  }

  NODISCARD FORCEINLINE D3D12_SHADER_BYTECODE
  getBytecode() const
  {
    return {.pShaderBytecode = m_bytecode.data(), .BytecodeLength = m_bytecode.size()};
  }

 private:
  Vector<uint8> m_bytecode;
  UUID m_shaderId;
};

} // namespace chEngineSDK
