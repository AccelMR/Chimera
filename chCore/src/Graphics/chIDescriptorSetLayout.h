/************************************************************************/
/**
 * @file chDescriptorSetLayout.h
 * @author AccelMR
 * @date 2025/04/17
 * @brief
 *  Descriptor set layout class. This class is used to define the descriptor set layout
 *  for the graphics pipeline. It is used to describe the format of the
 *  descriptor data and how it is laid out in memory.
 * 
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class IDescriptorSetLayout {
 public:
  virtual ~IDescriptorSetLayout() = default;
};
} // namespace chEngineSDK