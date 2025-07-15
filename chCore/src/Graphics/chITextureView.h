/************************************************************************/
/**
 * @file chITextureView.h
 * @author AccelMR
 * @date 2025/04/08
 * @brief
 * Interface for the texture view. This is the base class for all texture
 * views.
 * It is used to create texture views and allocate them.
 * It is also used to reset texture views and free them.
 */
 /************************************************************************/
 #pragma once

#include "chPrerequisitesCore.h"
#include "chGraphicsTypes.h"

namespace chEngineSDK {
class CH_CORE_EXPORT ITextureView {
 public:
  virtual ~ITextureView() = default;

  //NODISCARD virtual SPtr<ITexture>
  //getTexture() const = 0;

  NODISCARD virtual Format
  getFormat() const = 0;

  NODISCARD virtual TextureViewType
  getViewType() const = 0;

  NODISCARD virtual uint32
  getBaseMipLevel() const = 0;

  NODISCARD virtual uint32
  getMipLevelCount() const = 0;

  NODISCARD virtual uint32
  getBaseArrayLayer() const = 0;

  NODISCARD virtual uint32
  getArrayLayerCount() const = 0;

  NODISCARD virtual void*
  getRaw() const = 0;
};
} // namespace chEngineSDK
