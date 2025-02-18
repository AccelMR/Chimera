/************************************************************************/
/**
 * @file chRenderPass.h
 * @author AccelMR
 * @date 2025/02/02
 *  RenderPass interface.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesCore.h"

namespace chEngineSDK {

  // Forward declarations
namespace chGPUDesc {
  struct RenderPassDesc;
  struct SubpassDesc;
} // namespace chGPUDesc

class CH_CORE_EXPORT RenderPass {
 public:
  /*
   *   Default constructor
   */
  RenderPass() = default;

  /*
   *   Default destructor
   */
  virtual ~RenderPass() = default;

  void 
  init(const chGPUDesc::RenderPassDesc& desc);

  uint32
  getSubPassCount() const;

  chGPUDesc::SubpassDesc
  getSubpassDesc(uint32 index) const;

  bool
  isValid() const;

 protected:
  virtual void
  _internalInit(const chGPUDesc::RenderPassDesc& desc) = 0;

  virtual uint32
  _internalGetSubPassCount() const = 0;

  virtual bool
  _internalIsValid() const = 0;

  virtual chGPUDesc::SubpassDesc
  _internalGetSubpassDesc(uint32 index) const = 0;

};
}
