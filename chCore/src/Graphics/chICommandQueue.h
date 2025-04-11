/************************************************************************/
/**
 * @file chICommandQueue.h
 * @author AccelMR
 * @date 2025/04/08
 * @brief
 *  Interface for the command queue. This is the base class for all command
 *  queues. It is used to submit command buffers to the graphics API.
 *  It is also used to synchronize the CPU and GPU.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chGraphicsTypes.h"

namespace chEngineSDK {

class CH_CORE_EXPORT ICommandQueue {
 public:
  ICommandQueue() = default;
  virtual ~ICommandQueue() = default;

  NODISCARD virtual 
  QueueType getQueueType() const = 0;

  virtual void submit(const Vector<SPtr<ICommandBuffer>>& commandBuffers, 
                      const SPtr<IFence>& fence) = 0;

  virtual void waitIdle() = 0;
};

} // namespace chEngineSDK