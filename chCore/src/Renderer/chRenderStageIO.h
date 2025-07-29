/************************************************************************/
/**
 * @file chRenderStageIO.h
 * @author AccelMR
 * @date 2025/07/21
 * @brief Type-safe input/output system for render stages
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chTypeTraits.h"
#include "Data/chRenderDataBase.h"

namespace chEngineSDK {

/**
 * @brief Type-safe container for render stage inputs and outputs
 */
class CH_CORE_EXPORT RenderStageIO {
public:
  /**
   * @brief Set output data with compile-time type safety
   * @tparam T The data type (must inherit from RenderStageData)
   * @param data Shared pointer to the data
   */
  template<typename T>
  void setOutput(SPtr<T> data) {
    static_assert(std::is_base_of_v<RenderStageData, T>,
                  "T must inherit from RenderStageData");

    // Early return for null data
    if (!data) {
      return;
    }

    m_data[TypeTraits<T>::getTypeId()] = data;
  }

  /**
   * @brief Get input data with compile-time type safety
   * @tparam T The data type (must inherit from RenderStageData)
   * @return Shared pointer to the data, or nullptr if not found
   */
  template<typename T>
  SPtr<T> getInput() const {
    static_assert(std::is_base_of_v<RenderStageData, T>,
                  "T must inherit from RenderStageData");

    auto it = m_data.find(TypeTraits<T>::getTypeId());

    // Early return for missing data
    if (it == m_data.end()) {
      return nullptr;
    }

    return std::static_pointer_cast<T>(it->second);
  }

  /**
   * @brief Check if input data exists and is valid
   * @tparam T The data type to check
   * @return true if data exists and is valid
   */
  template<typename T>
  bool hasValidInput() const {
    auto input = getInput<T>();
    return input && input->isValid();
  }

  /**
   * @brief Clear all data
   */
  void clear() {
    m_data.clear();
  }

  /**
   * @brief Get all available type IDs
   * @return Vector of UUIDs for all stored data
   */
  Vector<UUID> getAvailableTypes() const {
    Vector<UUID> types;
    types.reserve(m_data.size());

    for (const auto& [typeId, _] : m_data) {
      types.push_back(typeId);
    }

    return types;
  }

  /**
   * @brief Get count of stored data items
   * @return Number of data items
   */
  SIZE_T getDataCount() const {
    return m_data.size();
  }

private:
  Map<UUID, SPtr<RenderStageData>> m_data;
};

} // namespace chEngineSDK
