/************************************************************************/
/**
 * @file chSampler.h
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
#include "chPrerequisitesGraphics.h"

namespace chEngineSDK {
namespace chGPUDesc{
struct SamplerDesc;
}

/* 
*/
class Sampler {
 public:
  /*
   *   Default constructor
   */
  Sampler() = default;

  /*
   *   Default destructor
   */
  virtual ~Sampler() = default;

 /*
  *   Initializes the sampler.
  */
  void
  init(const chGPUDesc::SamplerDesc& desc);

 protected:
  /**
   *   Initializes the sampler.
   */
  virtual void
  _internalInit(const chGPUDesc::SamplerDesc& desc) = 0;
};
} // namespace chEngineSDK
