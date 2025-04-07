/************************************************************************/
/**
 * @file chIGraphicsAPI.h
 * @author AccelMR
 * @date 2025/04/07
 * @brief
 *  Interface for the graphics API. This is the base class for all graphics
 */
 /************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"
#include "chModule.h"

namespace chEngineSDK {

struct GraphicsAPIInfo {
  WeakPtr<DisplaySurface> weakDisplaySurface;
  uint32 width = 1920;
  uint32 height = 1080;
  bool enableValidationLayer = true;
};

class IGraphicsAPI : public Module<IGraphicsAPI> {
  public:
  IGraphicsAPI() = default;
  virtual ~IGraphicsAPI() = default;
  
  virtual void
  initialize(const GraphicsAPIInfo& graphicsAPIInfo) = 0;

  virtual String
  getAdapterName() const = 0;
};

} // namespace chEngineSDK {