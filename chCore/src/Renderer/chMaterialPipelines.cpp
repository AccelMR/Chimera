/************************************************************************/
/**
 * @file chMaterialPipelines.cpp
 * @author AccelMR
 * @date 2026/10/14
 * @brief The pipelines one pass needs for every kind of material, and the draws that use
 *  them.
 */
/************************************************************************/

#include "chMaterialPipelines.h"

#include "chIPipeline.h"
#include "chPipelineCache.h"

namespace chEngineSDK {

/*
 */
void
MaterialPipelines::build(PipelineCache& cache,
                         GraphicsPipelineDesc desc,
                         const SPtr<IShader>& maskedFragmentShader)
{
  const SPtr<IShader> opaqueFragmentShader = desc.fragmentShader;
  const CullMode cullMode = desc.raster.cullMode;
  for (uint32 variant = 0; variant < VARIANT_COUNT; ++variant) {
    desc.fragmentShader =
        (variant & MASKED_BIT) != 0 ? maskedFragmentShader : opaqueFragmentShader;
    desc.raster.cullMode = (variant & TWO_SIDED_BIT) != 0 ? CullMode::None : cullMode;
    m_pipelines[variant] = cache.getOrCreate(desc).get();
  }
}

} // namespace chEngineSDK
