/************************************************************************/
/**
 * @file chPipelineCache.cpp
 * @author AccelMR
 * @date 2026/10/03
 * @brief
 * Reuses graphics pipelines with the same description.
 */
/************************************************************************/
#include "chPipelineCache.h"

#include "chIGraphicsAPI.h"
#include "chIPipeline.h"

namespace chEngineSDK {

/*
 */
const SPtr<IPipeline>&
PipelineCache::getOrCreate(const GraphicsPipelineDesc& desc)
{
  const uint64 key = desc.getHash();
  auto it = m_pipelines.find(key);
  if (it != m_pipelines.end()) {
    return it->second.pipeline;
  }

  Entry entry{.pipeline = IGraphicsAPI::instance().createGraphicsPipeline(desc),
              .vertexShader = desc.vertexShader,
              .fragmentShader = desc.fragmentShader};
  return m_pipelines.emplace(key, std::move(entry)).first->second.pipeline;
}

/*
 */
void
PipelineCache::clear()
{
  m_pipelines.clear();
}

} // namespace chEngineSDK
