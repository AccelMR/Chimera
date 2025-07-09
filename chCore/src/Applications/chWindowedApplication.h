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
#include "chLinearColor.h"

namespace chEngineSDK {

/**
 * @brief Render components structure containing all rendering resources
 */
struct ApplicationRenderContext {
  SPtr<ISwapChain> swapChain;
  SPtr<ITexture> depthTexture;
  SPtr<ITextureView> depthTextureView;
  SPtr<ICommandPool> commandPool;
  Vector<SPtr<ICommandBuffer>> commandBuffers;
  SPtr<ICommandQueue> graphicsQueue;

  Vector<SPtr<ISemaphore>> imageAvailableSemaphores;  // One per SwapChain image
  Vector<SPtr<ISemaphore>> renderFinishedSemaphores;  // One per SwapChain image
  Vector<SPtr<IFence>> inFlightFences;                // One per frame in flight

  // Frame tracking
  uint32 currentFrame = 0;
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

  void
  initialize() override;

 protected:

  NODISCARD FORCEINLINE virtual LinearColor
  getBackgroundColor() const { return LinearColor::Black; }

  NODISCARD FORCEINLINE virtual const ApplicationRenderContext&
  getRenderComponents() const { return m_renderComponents; }

  NODISCARD virtual SPtr<DisplayEventHandle>
  getEventHandler() const { return m_eventhandler; }

  NODISCARD FORCEINLINE virtual SPtr<DisplaySurface>
  getDisplaySurface() const { return m_display; }

  virtual void
  initializeModules() override;

  virtual void
  destroyModules() override;

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
  onRender(const float deltaTime, const SPtr<ICommandBuffer>&) { CH_PAMRAMETER_UNUSED(deltaTime); }

 private:
  void
  render(const float deltaTime);

  void
  resize(uint32 width, uint32 height);

  void
  createSyncObjects();

  void
  bindEvents();

 private:
  bool m_running = true;
  SPtr<DisplayEventHandle> m_eventhandler;
  SPtr<DisplaySurface> m_display;

  ApplicationRenderContext m_renderComponents;
};

} // namespace chEngineSDK
