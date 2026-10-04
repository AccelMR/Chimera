/************************************************************************/
/**
 * @file chITexture.h
 * @author AccelMR
 * @date 2025/04/08
 * @brief
 * Interface for the texture. This is the base class for all textures.
 * It is used to create textures and allocate them.
 * It is also used to reset textures and free them.
 */
 /************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chGraphicsTypes.h"

namespace chEngineSDK {
class ITexture {
 public:
  ITexture() = default;
  virtual ~ITexture() = default;

  NODISCARD virtual TextureType 
  getType() const = 0;

  NODISCARD virtual Format 
  getFormat() const = 0;

  NODISCARD virtual uint32 
  getWidth() const = 0;

  NODISCARD virtual uint32 
  getHeight() const = 0;

  NODISCARD virtual uint32 
  getDepth() const = 0;

  NODISCARD virtual uint32 
  getMipLevels() const = 0;

  NODISCARD virtual uint32 
  getArrayLayers() const = 0;

  /**
   * Index of a view of the whole texture in the bindless heap, for reading it in shaders;
   * GraphicsLimits::INVALID_BINDLESS_INDEX when it was not created with Sampled usage.
   */
  NODISCARD virtual uint32
  getBindlessIndex() const = 0;

  NODISCARD virtual SPtr<ITextureView>
  createView(const TextureViewCreateInfo& createInfo = {}) = 0;

  /**
   * Copies pixels into the texture and leaves it in the ShaderRead state. The data holds the
   * mip levels one after the other, each with all its layers; mips past the end of the data
   * keep undefined contents. The copy runs at the start of the next frame submit, before
   * anything that frame records, so the CPU never waits for it.
   */
  virtual void
  uploadData(const void* data, SIZE_T size) = 0;
};
} // namespace chEngineSDK