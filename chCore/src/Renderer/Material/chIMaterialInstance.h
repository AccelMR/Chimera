/************************************************************************/
/**
 * @file chIMaterialInstance.h
 * @author AccelMR
 * @date 2025/08/05
 * @brief Material Instance interface for per-object material variations
 */
/************************************************************************/
#pragma once

#include "chIMaterial.h"
#include "chPrerequisitesCore.h"

namespace chEngineSDK {

/**
 * @brief Interface for material instances - allows per-object parameter overrides
 * Similar to Unreal's Material Instance Dynamic system
 */
class CH_CORE_EXPORT IMaterialInstance
{
 public:
  virtual ~IMaterialInstance() = default;

  // Instance identification
  virtual UUID
  getInstanceId() const = 0;
  virtual String
  getName() const = 0;

  // Parent material reference (shared pipeline/shaders)
  virtual SPtr<IMaterial>
  getBaseMaterial() const = 0;

  // Parameter override system (early return pattern)
  virtual bool
  hasParameterOverride(const String& paramName) const = 0;
  virtual MaterialParameter
  getParameter(const String& paramName) const = 0;
  virtual void
  setParameter(const String& paramName, const MaterialParameter& value) = 0;
  virtual void
  removeParameterOverride(const String& paramName) = 0;

  // Get all parameter names (base + overrides)
  virtual Vector<String>
  getAllParameterNames() const = 0;
  virtual Vector<String>
  getOverrideParameterNames() const = 0;

  // Descriptor management per-instance (GPU resources)
  virtual SPtr<IDescriptorSet>
  getDescriptorSet() const = 0;
  virtual void
  updateDescriptorSet() = 0;
  virtual bool
  isDescriptorSetDirty() const = 0;

  // Validation
  virtual bool
  isValid() const = 0;
  virtual bool
  isCompatibleWith(const VertexLayout& layout) const = 0;
};
} // namespace chEngineSDK
