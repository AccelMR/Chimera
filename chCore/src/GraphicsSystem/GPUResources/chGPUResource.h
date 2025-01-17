/************************************************************************/
/**
 * @file chGPUResource.h
 * @author AccelMR
 * @date 2022/09/14
 *   Basic Resource handler that will interface any type of GPU resource
 * that the RHI can provide.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesCore.h"

namespace chEngineSDK{
class CH_CORE_EXPORT GPUResource
{
 public:
 /*
  *   Default constructor
  */
  FORCEINLINE GPUResource(): m_index(0){}

  FORCEINLINE bool isValid() const { return m_index != 0; }

 protected:
  friend class GPUResourceModule;

  FORCEINLINE GPUResource(uint32 _index)
    : m_index(_index)
  {}

  /*
   *   Default destructor
   */
   FORCEINLINE virtual ~GPUResource() = default;

 protected:
  uint32 m_index;
};
}
