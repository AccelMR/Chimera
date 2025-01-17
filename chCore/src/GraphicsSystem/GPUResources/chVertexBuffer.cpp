/************************************************************************/
/**
 * @file chVertexBuffer.cpp
 * @author AccelMR
 * @date 2022/11/13
 *   Vertex buffer interface that handles basic logic to create a GPU vertex buffer.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chVertexBuffer.h"

namespace chEngineSDK{
/*
*/
void
VertexBuffer::update(const chGPUDesc::VertexBufferDesc& des, 
                     const VertexBufferData data) {
  _internalUpdate(des, data);
}

}


