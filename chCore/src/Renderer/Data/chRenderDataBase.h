/************************************************************************/
/**
 * @file chRenderDataBase.h
 * @author AccelMR
 * @date 2025/07/21
 * @brief Base classes for render stage data system
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chUUID.h"

namespace chEngineSDK {

/**
 * @brief Base class for all render stage data
 * Provides type identification through UUID system
 */
struct CH_CORE_EXPORT RenderStageData {
  virtual ~RenderStageData() = default;

  /**
   * @brief Get the type ID for this data type
   * @return UUID identifying the concrete type
   */
  virtual UUID
  getTypeId() const = 0;

  /**
   * @brief Check if this data is valid/usable
   * @return true if data is valid, false otherwise
   */
  virtual bool
  isValid() const { return true; }
};

} // namespace chEngineSDK
