/************************************************************************/
/**
 * @file chTexture.h
 * @author AccelMR
 * @date 2022/09/19
 *  GPU Texture handler that holds inner an actual GPU Interface object
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesCore.h"

#include <chVector3I.h>

#include "chPrerequisitesGraphics.h"

#include "chGPUResource.h"
#include "chFormats.h"


namespace chEngineSDK{


class CH_CORE_EXPORT Texture: public GPUResource
{
 public:
  /*
  *   Default constructor
  */
  Texture() = default;

  /*
  *   Default destructor
  */
  virtual ~Texture() = default;

  FORCEINLINE int32
  getWidth() const { return m_width; }

  FORCEINLINE int32
  getHeight() const { return m_height; }

 protected:
  friend class GPUResourceModule;

  void
  init(const chGPUDesc::TextureDesc& desc);

 protected:

  virtual void
  _internalInit(const chGPUDesc::TextureDesc& desc) = 0;

 protected:
  int32 m_width = 0;
  int32 m_height = 0;
};
}


