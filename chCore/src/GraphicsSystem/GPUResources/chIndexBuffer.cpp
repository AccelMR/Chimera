/************************************************************************/
/**
 * @file chIndexBuffer.cpp
 * @author AccelMR
 * @date 2022/11/23
 *   
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chIndexBuffer.h"

namespace chEngineSDK{
/*
*/
void
IndexBuffer::update(const chGPUDesc::IndexBufferDesc& des, 
                    const IndexBufferData data) {
  _internalUpdate(des, data);
}

/*
*/
void
IndexBuffer::init( const chGPUDesc::IndexBufferDesc &des ) {
  _internalInit(des);
}

}


