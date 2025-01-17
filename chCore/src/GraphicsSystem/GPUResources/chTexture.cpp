/***********************************************************************/
/**
 * @file chTexture.h
 * @author AccelMR
 * @date 2022/09/19
 *  GPU Texture handler that holds inner an actual GPU Interface object
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chTexture.h"

namespace chEngineSDK{
/*
*/
void
Texture::init(const chGPUDesc::TextureDesc& desc) {
  _internalInit(desc);
}

}


