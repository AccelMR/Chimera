/************************************************************************/
/**
 * @file chDX12Shader.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief
 *  Direct3D 12 implementation of IShader.
 */
/************************************************************************/
#include "chDX12Shader.h"

namespace chEngineSDK {

/*
 */
DX12Shader::DX12Shader(const ShaderCreateInfo& createInfo)
  : m_bytecode(createInfo.sourceCode)
{
  if (m_bytecode.empty()) {
    CH_EXCEPT(DX12ErrorException,
              StringUtils::format("Shader {0} has no bytecode.", createInfo.filePath));
  }

  m_shaderId = UUID::createFromName(StringUtils::format(
      "{0}{1}{2}", createInfo.entryPoint, static_cast<uint32>(createInfo.stage),
      createInfo.filePath));
}

} // namespace chEngineSDK
