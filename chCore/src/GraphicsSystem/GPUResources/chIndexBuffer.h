/************************************************************************/
/**
 * @file chIndexBuffer.h
 * @author AccelMR
 * @date 2022/11/23
 *    Index buffer interface that holds a high level reference.
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
using IndexBufferData = void*;
class CH_CORE_EXPORT IndexBuffer {
 public:
 /*
  *   Default constructor
  */
  IndexBuffer() = default;

 /*
  *   Default destructor
  */
  virtual ~IndexBuffer() = default;

  void
  update(const chGPUDesc::IndexBufferDesc& des, const IndexBufferData data);

  void
  init(const chGPUDesc::IndexBufferDesc& des);

 protected:
 
 virtual void
 _internalUpdate(const chGPUDesc::IndexBufferDesc& des, 
                 const IndexBufferData data) = 0;

  virtual void
  _internalInit(const chGPUDesc::IndexBufferDesc& des) = 0;
 
};
}

