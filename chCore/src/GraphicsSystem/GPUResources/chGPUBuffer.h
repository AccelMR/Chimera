/************************************************************************/
/**
 * @file chGPUBuffer.h
 * @author AccelMR
 * @date 2022/10/18
 * GPU Buffer abstraction that do not necessarily hold memory but describes the buffer
 *  in GPU and how it'll be read this buffer.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesCore.h"

#include "chPrerequisitesGraphics.h"

namespace chEngineSDK{
using GPUBufferData = void*;
class CH_CORE_EXPORT GPUBuffer {
 public:
 /*
  *   Default constructor
  */
  GPUBuffer() = default;

 /*
  *   Default destructor
  */
  virtual ~GPUBuffer() = default;

  void
  init(const SIZE_T& size);

  void 
  update(const SIZE_T& size, const GPUBufferData data);

 protected:
  
  virtual void
  _init(const SIZE_T& size) = 0;

   
 virtual void
 _internalUpdate(const SIZE_T& size,
                 const GPUBufferData data) = 0; 
};
}

