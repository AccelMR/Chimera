/************************************************************************/
/**
 * @file chRenderStage.h
 * @author AccelMR
 * @date 2025/07/21
 * @brief Base interface for render stages
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chRenderStageIO.h"

namespace chEngineSDK {

/**
 * @brief Base interface for all render stages
 */
class CH_CORE_EXPORT IRenderStage
{
 public:
  virtual ~IRenderStage() = default;

  /**
   * @brief Get the unique type ID for this stage type
   * @return UUID identifying the stage type
   */
  virtual UUID
  getTypeId() const = 0;

  /**
   * @brief Get human-readable name for this stage
   * @return Stage name for debugging/logging
   */
  virtual String
  getName() const = 0;

  /**
   * @brief Initialize the stage with render target dimensions
   * @param width Render target width
   * @param height Render target height
   */
  virtual void
  initialize(uint32 width, uint32 height) = 0;

  /**
   * @brief Execute the render stage
   * @param inputs Type-safe input data from previous stages
   * @param outputs Type-safe output data for next stages
   * @param deltaTime Time since last frame
   * @return true if execution succeeded, false otherwise
   */
  virtual bool
  execute(SPtr<ICommandBuffer> commandBuffer, const RenderStageIO& inputs, RenderStageIO& outputs, float deltaTime) = 0;

  /**
   * @brief Resize render targets
   * @param width New width
   * @param height New height
   */
  virtual void
  resize(uint32 width, uint32 height) = 0;

  /**
   * @brief Cleanup stage resources
   */
  virtual void
  cleanup() = 0;

  /**
   * @brief Check if stage is enabled
   * @return true if enabled
   */
  virtual bool
  isEnabled() const {
    return m_enabled;
  }

  /**
   * @brief Enable/disable the stage
   * @param enabled true to enable, false to disable
   */
  virtual void
  setEnabled(bool enabled) {
    m_enabled = enabled;
  }

  /**
   * @brief Get the data types this stage requires as input
   * @return Vector of UUIDs for required input types
   */
  virtual Vector<UUID>
  getRequiredInputTypes() const = 0;

  /**
   * @brief Get the data types this stage provides as output
   * @return Vector of UUIDs for provided output types
   */
  virtual Vector<UUID>
  getProvidedOutputTypes() const = 0;

 protected:
  bool m_enabled = true;
  uint32 m_width = 0;
  uint32 m_height = 0;
};

} // namespace chEngineSDK
