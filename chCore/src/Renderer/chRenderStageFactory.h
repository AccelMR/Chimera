/************************************************************************/
/**
 * @file chRenderStageFactory.h
 * @author AccelMR
 * @date 2025/07/21
 * @brief Factory for creating render stages
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chLogger.h"
#include "chRenderStage.h"
#include "chModule.h"

namespace chEngineSDK {

using RenderStageCreatorFunc = Function<SPtr<IRenderStage>()>;  // Changed to SPtr

CH_LOG_DECLARE_STATIC(RenderStageFactoryLog, All);

/**
 * @brief Factory for creating render stages by type
 */
class CH_CORE_EXPORT RenderStageFactory : public Module<RenderStageFactory> {
public:
  /**
   * @brief Register a stage type for creation
   * @tparam T Stage type (must inherit from IRenderStage)
   */
  template<typename T>
  void registerStageType() {
    static_assert(std::is_base_of_v<IRenderStage, T>, "T must inherit from IRenderStage");

    const UUID typeId = TypeTraits<T>::getTypeId();

    m_creators[typeId] = []() -> SPtr<IRenderStage> {
      return chMakeShared<T>();  // Use chMakeShared
    };

    m_typeNames[typeId] = String(TypeTraits<T>::getTypeName());

    CH_LOG_INFO(RenderStageFactoryLog, "Registered stage type: {0} with ID: {1}",
                TypeTraits<T>::getTypeName(), typeId.toString());
  }

  /**
   * @brief Create a stage by type ID
   * @param typeId UUID of the stage type
   * @return Shared pointer to created stage, or nullptr if type not found
   */
  SPtr<IRenderStage> createStage(const UUID& typeId) const {  // Changed to SPtr
    auto it = m_creators.find(typeId);
    if (it == m_creators.end()) {
      CH_LOG_ERROR(RenderStageFactoryLog, "Unknown stage type ID: {0}", typeId.toString());
      return nullptr;
    }
    return it->second();
  }

  /**
   * @brief Create a stage by template type
   * @tparam T Stage type
   * @return Shared pointer to created stage
   */
  template<typename T>
  SPtr<IRenderStage> createStage() const {  // Changed to SPtr
    return createStage(TypeTraits<T>::getTypeId());
  }

  /**
   * @brief Get all registered stage type IDs
   * @return Vector of registered type UUIDs
   */
  Vector<UUID> getRegisteredTypes() const {
    Vector<UUID> types;
    types.reserve(m_creators.size());

    for (const auto& [typeId, _] : m_creators) {
      types.push_back(typeId);
    }

    return types;
  }

  /**
   * @brief Get human-readable name for a stage type
   * @param typeId UUID of the stage type
   * @return Type name or "Unknown"
   */
  String getTypeName(const UUID& typeId) const {
    auto it = m_typeNames.find(typeId);
    return (it != m_typeNames.end()) ? it->second : "Unknown";
  }

private:
  Map<UUID, RenderStageCreatorFunc> m_creators;
  Map<UUID, String> m_typeNames;
};

} // namespace chEngineSDK
