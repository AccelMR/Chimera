/************************************************************************/
/**
 * @file chGPUPipelineState.h
 * @author AccelMR
 * @date 2022/09/29
 * 
 *    Pipeline state interface to be specific implemented for each drive API.
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
class GPUPipelineState
{
 public:
 /*
  *   Default constructor
  */
  GPUPipelineState() = default;

 /*
  *   Default destructor
  */
  virtual ~GPUPipelineState() = default;
};
}

