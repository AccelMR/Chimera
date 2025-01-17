/************************************************************************/
/**
 * @file chGPUBuffer.cpp
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
#include "chGPUBuffer.h"

namespace chEngineSDK{

/*
*/
void
GPUBuffer::init(const SIZE_T& size) {
  _init(size);
}

/*
*/
void
GPUBuffer::update(const SIZE_T& size, const GPUBufferData data) {
  _internalUpdate(size, data);
}

}


