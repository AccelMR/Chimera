/************************************************************************/
/**
 * @file chStandardMaterial.h
 * @author AccelMR
 * @date 2025/08/02
 * @brief
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chIMaterial.h"

namespace chEngineSDK {
class StandardMaterial : public IMaterial {
 public:
  StandardMaterial(const String& name,
                   const SPtr<IShader>& vertexShader,
                   const SPtr<IShader>& fragmentShader);

  ~StandardMaterial() override;

  // IMaterial interface
  FORCEINLINE UUID
  getMaterialId() const override { return m_materialId; }

  FORCEINLINE String
  getName() const override {
    return m_name;
  }

  FORCEINLINE SPtr<IShader>
  getVertexShader() const override { return m_vertexShader; }

  FORCEINLINE SPtr<IShader>
  getFragmentShader() const override { return m_fragmentShader; }

  SPtr<IShader>
  getShader(ShaderStage stage) const override {
    switch (stage) {
      case ShaderStage::Vertex:
        return m_vertexShader;
      case ShaderStage::Fragment:
        return m_fragmentShader;
      default:
        return nullptr;
    }
  }

  FORCEINLINE bool
  isStageSupported(ShaderStage stage) const override {
    return stage == ShaderStage::Vertex || stage == ShaderStage::Fragment;
  }

  FORCEINLINE MaterialParameter
  getParameter(const String& name) const override {
    auto it = m_customParameters.find(name);
    if (it != m_customParameters.end()) {
      return it->second;
    }
    return MaterialParameter(); // Return default if not found
  }

  SPtr<IPipeline>
  createPipeline(const PipelineCreateInfo& baseInfo) const override;

  FORCEINLINE bool
  isValid() const override {
    return m_vertexShader && m_fragmentShader && !m_name.empty();
  }

  FORCEINLINE bool
  isCompatibleWith(const VertexLayout& layout) const override {
    // Check if the vertex layout matches the expected layout for this material
    return layout == VertexNormalTexCoord::getLayout();
  }

 private:
  UUID m_materialId;
  String m_name;

  // Shader references
  SPtr<IShader> m_vertexShader;
  SPtr<IShader> m_fragmentShader;

  // PBR Parameters
  LinearColor m_albedo = {1.0f, 1.0f, 1.0f, 1.0f};
  float m_metallic = 0.0f;
  float m_roughness = 0.5f;
  float m_normalStrength = 1.0f;

  // Texture maps
  SPtr<ITextureView> m_albedoTexture;
  SPtr<ITextureView> m_normalTexture;
  SPtr<ITextureView> m_metallicTexture;
  SPtr<ITextureView> m_roughnessTexture;

  Map<MaterialParameter::Type, MaterialParameter> m_customParameters;
};

} // namespace chEngineSDK
