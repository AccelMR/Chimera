/************************************************************************/
/**
 * @file chMultiStageRenderer.cpp
 * @author AccelMR
 * @date 2025/07/21
 * @brief Multi-stage renderer implementation
 */
/************************************************************************/

#include "chMultiStageRenderer.h"
#include "chLogger.h"

namespace chEngineSDK {

#if USING(CH_DEBUG_MODE)
#define CH_MULTI_STAGE_RENDERER_LOG_LEVEL All
#else
#define CH_MULTI_STAGE_RENDERER_LOG_LEVEL Info
#endif

CH_LOG_DECLARE_STATIC( MultiStageRendererLog, CH_MULTI_STAGE_RENDERER_LOG_LEVEL);

/*
 */
MultiStageRenderer::MultiStageRenderer() {
  CH_LOG_INFO( MultiStageRendererLog, "MultiStageRenderer created");
}

/*
 */
MultiStageRenderer::~MultiStageRenderer() {
  CH_LOG_INFO( MultiStageRendererLog, "MultiStageRenderer destroyed");
  cleanup();
}

/*
 */
void
MultiStageRenderer::initialize(uint32 width, uint32 height) {
  CH_LOG_INFO( MultiStageRendererLog, "Initializing MultiStageRenderer with dimensions: {0}x{1}",
              width, height);

  m_renderWidth = width;
  m_renderHeight = height;

  // Initialize all stages
  for (auto& stageInstance : m_stages) {
    if (stageInstance.stage) {
      stageInstance.stage->initialize(width, height);
    }
  }

  CH_LOG_INFO( MultiStageRendererLog, "MultiStageRenderer initialized with {0} stages",
              m_stages.size());
}

/*
 */
RendererOutput
MultiStageRenderer::onRender(float deltaTime) {
  // Early return if no stages
  if (m_stages.empty()) {
    CH_LOG_WARNING( MultiStageRendererLog, "No render stages configured");
    return {};
  }

  executeRenderPipeline(deltaTime);
  return m_finalOutput;
}

/*
 */
void
MultiStageRenderer::resize(uint32 width, uint32 height) {
  CH_LOG_INFO( MultiStageRendererLog, "Resizing MultiStageRenderer to {0}x{1}", width, height);

  m_renderWidth = width;
  m_renderHeight = height;

  // Resize all stages
  for (auto& stageInstance : m_stages) {
    if (stageInstance.stage) {
      stageInstance.stage->resize(width, height);
    }
  }

  CH_LOG_INFO( MultiStageRendererLog, "MultiStageRenderer resized successfully");
}

/*
 */
void
MultiStageRenderer::cleanup() {
  CH_LOG_INFO( MultiStageRendererLog, "Cleaning up MultiStageRenderer");

  // Cleanup all stages
  for (auto& stageInstance : m_stages) {
    if (stageInstance.stage) {
      stageInstance.stage->cleanup();
    }
  }

  m_stages.clear();
  m_stageMap.clear();
  m_sceneData.clear();

  CH_LOG_INFO( MultiStageRendererLog, "MultiStageRenderer cleanup completed");
}

/*
 */
void
MultiStageRenderer::setClearColors(const Vector<LinearColor>& clearColors) {
  m_clearColors = clearColors;
}

/*
 */
UUID
MultiStageRenderer::addStage(SPtr<IRenderStage> stage, const UUID& instanceId) {
  if (!stage) {
    CH_LOG_ERROR( MultiStageRendererLog, "Cannot add null stage");
    return UUID::null();
  }

  // Generate instance ID if not provided
  UUID finalInstanceId = instanceId.isNull() ? UUID::createRandom() : instanceId;

  // Check for duplicate instance ID
  if (m_stageMap.find(finalInstanceId) != m_stageMap.end()) {
    CH_LOG_ERROR( MultiStageRendererLog, "Stage with instance ID {0} already exists",
                 finalInstanceId.toString());
    return UUID::null();
  }

  // Add stage
  StageInstance stageInstance{
    .stage = stage,
    .instanceId = finalInstanceId,
    .typeId = stage->getTypeId(),
    .enabled = true
  };

  SIZE_T index = m_stages.size();
  m_stages.push_back(stageInstance);
  m_stageMap[finalInstanceId] = index;

  // Initialize stage if renderer is already initialized
  if (m_renderWidth > 0 && m_renderHeight > 0) {
    stage->initialize(m_renderWidth, m_renderHeight);
  }

  CH_LOG_INFO( MultiStageRendererLog, "Added stage '{0}' with instance ID: {1}",
              stage->getName(), finalInstanceId.toString());

  return finalInstanceId;
}

/*
 */
void
MultiStageRenderer::removeStage(const UUID& instanceId) {
  auto it = m_stageMap.find(instanceId);
  if (it == m_stageMap.end()) {
    CH_LOG_WARNING( MultiStageRendererLog, "Stage with instance ID {0} not found",
                   instanceId.toString());
    return;
  }

  SIZE_T index = it->second;

  // Cleanup stage
  if (m_stages[index].stage) {
    m_stages[index].stage->cleanup();
  }

  // Remove from vector and update map
  m_stages.erase(m_stages.begin() + index);
  m_stageMap.erase(it);

  // Update indices in map
  for (auto& [id, idx] : m_stageMap) {
    if (idx > index) {
      idx--;
    }
  }

  CH_LOG_INFO( MultiStageRendererLog, "Removed stage with instance ID: {0}",
              instanceId.toString());
}

/*
 */
void
MultiStageRenderer::enableStage(const UUID& instanceId, bool enabled) {
  auto it = m_stageMap.find(instanceId);
  if (it == m_stageMap.end()) {
    CH_LOG_WARNING( MultiStageRendererLog, "Stage with instance ID {0} not found",
                   instanceId.toString());
    return;
  }

  m_stages[it->second].enabled = enabled;
  if (m_stages[it->second].stage) {
    m_stages[it->second].stage->setEnabled(enabled);
  }

  CH_LOG_DEBUG( MultiStageRendererLog, "Stage {0} {1}",
               instanceId.toString(), enabled ? "enabled" : "disabled");
}

/*
 */
IRenderStage*
MultiStageRenderer::getStage(const UUID& instanceId) const {
  auto it = m_stageMap.find(instanceId);
  if (it == m_stageMap.end()) {
    return nullptr;
  }

  return m_stages[it->second].stage.get();
}

/*
 */
void
MultiStageRenderer::setSceneData(const RenderStageIO& sceneData) {
  m_sceneData = sceneData;
}

/*
 */
Vector<UUID>
MultiStageRenderer::getStageIds() const {
  Vector<UUID> ids;
  ids.reserve(m_stages.size());

  for (const auto& stageInstance : m_stages) {
    ids.push_back(stageInstance.instanceId);
  }

  return ids;
}

/*
 */
void
MultiStageRenderer::executeRenderPipeline(float deltaTime) {
  RenderStageIO currentInputs = m_sceneData; // Start with scene data
  RenderStageIO currentOutputs;

  for (const auto& stageInstance : m_stages) {
    // Early continue for disabled stages
    if (!stageInstance.enabled || !stageInstance.stage || !stageInstance.stage->isEnabled()) {
      continue;
    }

    // Clear outputs for this stage
    currentOutputs.clear();

    // Execute stage
    bool success = stageInstance.stage->execute(currentInputs, currentOutputs, deltaTime);

    // Early return on stage failure
    if (!success) {
      CH_LOG_ERROR( MultiStageRendererLog, "Stage '{0}' execution failed",
                   stageInstance.stage->getName());
      return;
    }

    // Chain outputs to next stage inputs
    currentInputs = currentOutputs;
  }

  // Update final output (simplified for now)
  // In a real implementation, you'd extract the final color texture
  m_finalOutput.width = m_renderWidth;
  m_finalOutput.height = m_renderHeight;
  m_finalOutput.isValid = true;

  // TODO: Extract final color target from last stage outputs
  // For now, we'll leave colorTarget and depthTarget as nullptr
}

} // namespace chEngineSDK
