/************************************************************************/
/**
 * @file chVertexBuffer.h
 * @author AccelMR
 * @date 2022/10/28
 *
 *   Vertex buffer interface that handles basic logic to create a GPU vertex buffer.
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
using VertexBufferData = void*;
class CH_CORE_EXPORT VertexBuffer
{
 public:
 /*
  *   Default constructor
  */
  VertexBuffer() = default;

 /*
  *   Default destructor
  */
  virtual ~VertexBuffer() = default;

  void
  update(const chGPUDesc::VertexBufferDesc& des, const VertexBufferData data);

 protected:
 
 virtual void
 _internalUpdate(const chGPUDesc::VertexBufferDesc& des, 
                 const VertexBufferData data) = 0;
 
};

}

