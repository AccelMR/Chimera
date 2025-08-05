/************************************************************************/
/**
 * @file chMaterialManager.h
 * @author AccelMR
 * @date 2025/08/02
 * @brief
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chModule.h"

namespace chEngineSDK {
class IMaterial;
class IShader;
class UUID;

class CH_CORE_EXPORT MaterialManager : Module<MaterialManager>
{
 public:
  SPtr<IMaterial>
  createCustomMaterial(const String& name);

  // Material retrieval
  SPtr<IMaterial>
  getMaterial(const UUID& materialId) const;

  SPtr<IMaterial>
  getMaterialByName(const String& name) const;

  // Shader management integration
  void
  reloadShadersForMaterial(const UUID& materialId);

 private:

 private:
  Map<UUID, SPtr<IMaterial>> m_materials;
  Map<String, UUID> m_materialNameMap;
  Map<UUID, SPtr<IShader>> m_shaderCache;
};
} // namespace chEngineSDK
