setParameter/************************************************************************/
/**
 * @file chStandardMaterialInstance.h
 * @author AccelMR
 * @date 2025/08/05
 * @brief Standard PBR Material Instance implementation
 */
/************************************************************************/
#pragma once

#include "chIMaterialInstance.h"
#include "chPrerequisitesCore.h"

#include "chUUID.h"

namespace chEngineSDK {

class CH_CORE_EXPORT StandardMaterialInstance : public IMaterialInstance
{
 public:
  StandardMaterialInstance(const String& name, SPtr<IMaterial> baseMaterial);
  ~StandardMaterialInstance() override;

  // IMaterialInstance interface
  UUID
  getInstanceId() const override {
    return m_instanceId;
  }
  String
  getName() const override {
    return m_name;
  }
  SPtr<IMaterial>
  getBaseMaterial() const override {
    return m_baseMaterial;
  }

  bool
  hasParameterOverride(const String& paramName) const override {
    return m_parameterOverrides.find(paramName) != m_parameterOverrides.end();
  }

  MaterialParameter
  getParameter(const String& paramName) const override;
  void
  setParameter(const String& paramName, const MaterialParameter& value) override;
  void
  removeParameterOverride(const String& paramName) override;

  Vector<String>
  getAllParameterNames() const override;
  Vector<String>
  getOverrideParameterNames() const override;

  // Descriptor management
  SPtr<IDescriptorSet>
  getDescriptorSet() const override {
    return m_descriptorSet;
  }
  void
  updateDescriptorSet() override;
  bool
  isDescriptorSetDirty() const override {
    return m_descriptorSetDirty;
  }

  // Validation
  bool
  isValid() const override {
    return m_baseMaterial && m_baseMaterial->isValid();
  }

  bool
  isCompatibleWith(const VertexLayout& layout) const override {
    return m_baseMaterial ? m_baseMaterial->isCompatibleWith(layout) : false;
  }

  // Convenience methods for common PBR parameters
  void
  setAlbedo(const LinearColor& color);
  void
  setMetallic(float metallic);
  void
  setRoughness(float roughness);
  void
  setAlbedoTexture(SPtr<ITextureView> texture);

 private:
  void
  markDescriptorSetDirty() {
    m_descriptorSetDirty = true;
  }
  void
  createDescriptorSet();

 private:
  UUID m_instanceId;
  String m_name;
  SPtr<IMaterial> m_baseMaterial;

  // Parameter overrides (only stores differences from base material)
  UnorderedMap<String, MaterialParameter> m_parameterOverrides;

  // Per-instance GPU resources
  SPtr<IDescriptorSet> m_descriptorSet;
  bool m_descriptorSetDirty = true;
};

} // namespace chEngineSDK
