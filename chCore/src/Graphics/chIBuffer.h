/************************************************************************/
/**
 * @file chIBuffer.h
 * @author AccelMR
 * @date 2025/04/08
 * @brief
 * Interface for the buffer. This is the base class for all buffers.
 * It is used to create buffers and allocate them.
 * It is also used to reset buffers and free them.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chGraphicsTypes.h"

namespace chEngineSDK {
class IBuffer {
 public:
  virtual ~IBuffer() = default;

  /**
   * A buffer the CPU can write (CpuToGpu, CpuOnly) is written at once. A GpuOnly buffer is
   * copied at the start of the next frame submit, before anything that frame records.
   */
  virtual void
  update(const void* data, SIZE_T size, uint32 offset = 0) = 0;

  NODISCARD virtual SIZE_T
  getSize() const = 0;

  /**
   * Index of this buffer in the bindless heap (ResourceDescriptorHeap in HLSL) for uniform
   * and storage buffers; GraphicsLimits::INVALID_BINDLESS_INDEX for the others.
   */
  NODISCARD virtual uint32
  getBindlessIndex() const = 0;
};

} // namespace chEngineSDK