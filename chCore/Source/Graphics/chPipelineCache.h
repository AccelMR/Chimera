/************************************************************************/
/**
 * @file chPipelineCache.h
 * @author AccelMR
 * @date 2026/10/03
 * @brief
 * Reuses graphics pipelines with the same description.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chGraphicsTypes.h"

namespace chEngineSDK {

/**
 * Hands out one pipeline per distinct description, so a renderer can ask for the pipeline
 * it needs when it needs it instead of keeping its own. Building a pipeline compiles
 * shaders in the driver and takes milliseconds; a lookup is a hash and a map search.
 */
class CH_CORE_EXPORT PipelineCache
{
 public:
  /**
   * The reference stays valid until clear(), so it can be kept or bound every frame.
   */
  NODISCARD const SPtr<IPipeline>&
  getOrCreate(const GraphicsPipelineDesc& desc);

  /**
   * Releases every pipeline; call it before the graphics API shuts down.
   */
  void
  clear();

  NODISCARD FORCEINLINE SIZE_T
  getSize() const
  {
    return m_pipelines.size();
  }

 private:
  struct Entry
  {
    SPtr<IPipeline> pipeline;
    // Keeps the shaders alive, so a new shader cannot take the address of a freed one and
    // get its pipeline (the key uses shader identity).
    SPtr<IShader> vertexShader;
    SPtr<IShader> fragmentShader;
  };

  UnorderedMap<uint64, Entry> m_pipelines;
};

} // namespace chEngineSDK
