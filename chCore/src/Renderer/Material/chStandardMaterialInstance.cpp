/************************************************************************/
/**
 * @file chStandardMaterialInstance.cpp
 * @author AccelMR
 * @date 2025/08/05
 * @brief Standard PBR Material Instance implementation
 */
/************************************************************************/

#include "chStandardMaterialInstance.h"

#include "chIGraphicsAPI.h"
#include "chLogger.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(StandardMaterialInstanceLog, All);

/*
 */
StandardMaterialInstance::StandardMaterialInstance(const String& name,
                                                   SPtr<IMaterial> baseMaterial)
 : m_instanceId(UUID::createRandom()), m_name(name), m_baseMaterial(std::move(baseMaterial)) {
  if (!m_baseMaterial) {
    CH_LOG_ERROR(StandardMaterialInstanceLog,
                 "Cannot create material instance '{0}' with null base material", name);
    return;
  }

  CH_LOG_INFO(StandardMaterialInstanceLog, "Created material instance '{0}' from base '{1}'",
              name, m_baseMaterial->getName());

  // Defer descriptor set creation until first use
  m_descriptorSetDirty = true;
}

/*
 */
StandardMaterialInstance::~StandardMaterialInstance() {
  CH_LOG_DEBUG(StandardMaterialInstanceLog, "Destroying material instance '{0}'", m_name);
}

/*
 */
MaterialParameter
StandardMaterialInstance::getParameter(const String& paramName) const {
  // Early return: check override first (most common case)
  auto overrideIt = m_parameterOverrides.find(paramName);
  if (overrideIt != m_parameterOverrides.end()) {
    return overrideIt->second;
  }

  // Fallback to base material parameter
  if (!m_baseMaterial) {
    CH_LOG_WARNING(StandardMaterialInstanceLog, "No base material for parameter '{0}'",
                   paramName);
    return MaterialParameter(); // Return invalid parameter
  }

  return m_baseMaterial->getParameter(paramName);
}

/*
 */
void
StandardMaterialInstance::setParameter(const String& paramName,
                                       const MaterialParameter& value) {
  if (paramName.empty()) {
    CH_LOG_WARNING(StandardMaterialInstanceLog, "Cannot set parameter with empty name");
    return;
  }

  m_parameterOverrides[paramName] = value;
  markDescriptorSetDirty();

  CH_LOG_DEBUG(StandardMaterialInstanceLog, "Set parameter '{0}' on instance '{1}'", paramName,
               m_name);
}

/*
 */
void
StandardMaterialInstance::removeParameterOverride(const String& paramName) {
  auto it = m_parameterOverrides.find(paramName);
  if (it != m_parameterOverrides.end()) {
    m_parameterOverrides.erase(it);
    markDescriptorSetDirty();
    CH_LOG_DEBUG(StandardMaterialInstanceLog,
                 "Removed parameter override '{0}' from instance '{1}'", paramName, m_name);
  }
}

/*
 */
Vector<String>
StandardMaterialInstance::getAllParameterNames() const {
  Vector<String> allNames;

  // Start with base material parameters
  if (m_baseMaterial) {
    allNames = m_baseMaterial->getParameterNames();
  }

  // Add any override-only parameters (shouldn't happen in normal usage)
  for (const auto& [name, param] : m_parameterOverrides) {
    if (std::find(allNames.begin(), allNames.end(), name) == allNames.end()) {
      allNames.push_back(name);
    }
  }

  return allNames;
}

/*
 */
Vector<String>
StandardMaterialInstance::getOverrideParameterNames() const {
  Vector<String> overrideNames;
  overrideNames.reserve(m_parameterOverrides.size());

  for (const auto& [name, param] : m_parameterOverrides) {
    overrideNames.push_back(name);
  }

  return overrideNames;
}

/*
 */
void
StandardMaterialInstance::updateDescriptorSet() {
  if (!m_descriptorSetDirty) {
    return; // Early return if not dirty
  }

  if (!m_baseMaterial) {
    CH_LOG_ERROR(StandardMaterialInstanceLog,
                 "Cannot update descriptor set without base material");
    return;
  }

  // Create descriptor set if needed
  if (!m_descriptorSet) {
    createDescriptorSet();
  }

  // TODO: Update descriptor set with current parameter values
  // This will be implemented when we have the descriptor set update system ready

  m_descriptorSetDirty = false;
  CH_LOG_DEBUG(StandardMaterialInstanceLog, "Updated descriptor set for instance '{0}'",
               m_name);
}

/*
 */
void
StandardMaterialInstance::createDescriptorSet() {
  // TODO: Create descriptor set based on base material's layout
  // For now, this is a placeholder - will implement when descriptor system is ready
  CH_LOG_DEBUG(StandardMaterialInstanceLog, "Creating descriptor set for instance '{0}'",
               m_name);
}

// Convenience methods for common PBR parameters
/*
 */
void
StandardMaterialInstance::setAlbedo(const LinearColor& color) {
  setParameter("u_albedo", MaterialParameter(color));
}

/*
 */
void
StandardMaterialInstance::setMetallic(float metallic) {
  setParameter("u_metallic", MaterialParameter(metallic));
}

/*
 */
void
StandardMaterialInstance::setRoughness(float roughness) {
  setParameter("u_roughness", MaterialParameter(roughness));
}

/*
 */
void
StandardMaterialInstance::setAlbedoTexture(SPtr<ITextureView> texture) {
  setParameter("u_albedoTexture", MaterialParameter(std::move(texture)));
}

} // namespace chEngineSDK
