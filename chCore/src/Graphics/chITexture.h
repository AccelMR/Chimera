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
class CH_CORE_EXPORT ITexture {
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

  NODISCARD virtual SPtr<ITextureView> 
  createView(const TextureViewCreateInfo& createInfo = {}) = 0;

  virtual void 
  uploadData(const void* data, size_t size) = 0;
};
} // namespace chEngineSDK