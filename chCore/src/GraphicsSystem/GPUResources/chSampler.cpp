/************************************************************************/
/**
 * @file chSampler.cpp
 * @author AccelMR
 * @date 2025/01/26
 *   Sampler descriptor for GPU resources.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chSampler.h"

namespace chEngineSDK {

/*
*/
void
Sampler::init(const chGPUDesc::SamplerDesc& desc) {
  _internalInit(desc);
}

} // namespace chEngineSDK