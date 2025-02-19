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

// Forward declaration
namespace chGPUDesc {
  struct BindingGroup;
} // namespace chGPUDesc

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

  const Vector<chGPUDesc::BindingGroup>&
  getBindingGroups() const {
    return m_bindingBufferGroup;
  }

 protected:
  Vector<chGPUDesc::BindingGroup> m_bindingBufferGroup;
};
}

