/************************************************************************/
/**
 * @file chWindowedApplication.h
 * @author AccelMR
 * @date 2025/07/05
 * Windowed application base class with DisplaySurface and graphics
 */
/************************************************************************/
#pragma once

#include "chBaseApplication.h"
#include "chDisplaySurface.h"

namespace chEngineSDK {

struct RenderComponents {
};

class CH_CORE_EXPORT WindowedApplication : public BaseApplication {
 public:
  /**
   * @brief Default constructor.
   */
  WindowedApplication();

  /**
   * @brief Default destructor.
   */
  virtual ~WindowedApplication();

  virtual void
  run() override;

 protected:

  virtual void
  initializeModules() override;

  virtual void
  destroyModules() override;

  void
  onPostInitialize() override;

  virtual void
  onPostDestoyModules() override;

  virtual void
  initializeDisplay(const ScreenDescriptor& desc);

  virtual void
  initializeGraphics();

  virtual void
  initializeRenderComponents();

  virtual void
  destroyGraphics();

  virtual void
  destroyDisplay();

  virtual void
  destroyRenderer();

  virtual void
  onRender(const float deltaTime) { CH_PAMRAMETER_UNUSED(deltaTime); }

 private:
  void
  render(const float deltaTime);

 private:
  bool m_running = true;
  SPtr<DisplayEventHandle> m_eventhandler;
  SPtr<DisplaySurface> m_display;

  RenderComponents m_renderComponents;
};

} // namespace chEngineSDK
