/************************************************************************/
/**
 * @file chMultiStageRenderer.h
 * @author AccelMR
 * @date 2025/07/21
 * @brief Multi-stage renderer that executes render stages in sequence
 */
/************************************************************************/
#pragma once

#include "chIRenderer.h"

#include "chLinearColor.h"
#include "chRenderStage.h"
#include "chRenderStageFactory.h"

namespace chEngineSDK {

/**
 * @brief Multi-stage renderer that can execute multiple render stages
 */
class CH_CORE_EXPORT MultiStageRenderer : public IRenderer
{
 public:
  MultiStageRenderer();
  ~MultiStageRenderer() override;

  // IRenderer interface
  void
  initialize(uint32 width, uint32 height) override;
  RendererOutput
  onRender(float deltaTime) override;
  void
  resize(uint32 width, uint32 height) override;
  void
  cleanup() override;

  uint32
  getWidth() const override {
    return m_renderWidth;
  }
  uint32
  getHeight() const override {
    return m_renderHeight;
  }
  void
  setClearColors(const Vector<LinearColor>& clearColors) override;

  // Stage management
  /**
   * @brief Add a render stage to the pipeline
   * @param stage Shared pointer to the stage
   * @param instanceId Custom instance ID, or null for auto-generated
   * @return Instance ID of the added stage
   */
  UUID
  addStage(SPtr<IRenderStage> stage, const UUID& instanceId = UUID::null());

  /**
   * @brief Remove a stage from the pipeline
   * @param instanceId Instance ID of the stage to remove
   */
  void
  removeStage(const UUID& instanceId);

  /**
   * @brief Enable/disable a stage
   * @param instanceId Instance ID of the stage
   * @param enabled true to enable, false to disable
   */
  void
  enableStage(const UUID& instanceId, bool enabled = true);

  /**
   * @brief Get a stage by instance ID
   * @param instanceId Instance ID of the stage
   * @return Pointer to stage, or nullptr if not found
   */
  IRenderStage*
  getStage(const UUID& instanceId) const;

  /**
   * @brief Get a stage with type casting
   * @tparam T Stage type
   * @param instanceId Instance ID of the stage
   * @return Typed pointer to stage, or nullptr if not found or wrong type
   */
  template <typename T>
  T*
  getStageAs(const UUID& instanceId) const {
    static_assert(std::is_base_of_v<IRenderStage, T>, "T must inherit from IRenderStage");
    auto* stage = getStage(instanceId);
    return dynamic_cast<T*>(stage);
  }

  /**
   * @brief Set initial scene data for the pipeline
   * @param sceneData Initial data to pass to first stage
   */
  void
  setSceneData(const RenderStageIO& sceneData);

  /**
   * @brief Get all stage instance IDs
   * @return Vector of instance UUIDs
   */
  Vector<UUID>
  getStageIds() const;

 private:
  void
  executeRenderPipeline(float deltaTime);

  struct StageInstance {
    SPtr<IRenderStage> stage;
    UUID instanceId;
    UUID typeId;
    bool enabled = true;
  };

  Vector<StageInstance> m_stages;
  Map<UUID, SIZE_T> m_stageMap; // instanceId -> index in m_stages

  RenderStageIO m_sceneData; // Initial data for pipeline

  uint32 m_renderWidth = 0;
  uint32 m_renderHeight = 0;
  Vector<LinearColor> m_clearColors;

  // Final output (from last enabled stage)
  RendererOutput m_finalOutput;
};

} // namespace chEngineSDK
