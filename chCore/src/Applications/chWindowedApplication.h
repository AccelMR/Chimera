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

class CH_CORE_EXPORT WindowedApplication : public BaseApplication
{
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
  NODISCARD FORCEINLINE const SPtr<ISwapChain>&
  getSwapChain() const
  {
    return m_swapChain;
  }

  NODISCARD virtual SPtr<DisplayEventHandle>
  getEventHandler() const {
    return m_eventhandler;
  }

  NODISCARD FORCEINLINE virtual SPtr<DisplaySurface>
  getDisplaySurface() const {
    return m_display;
  }

  virtual void
  initializeModules() override;

  virtual void
  destroyModules() override;

  virtual void
  initializeDisplay(const ScreenDescriptor& desc);

  virtual void
  initializeGraphics();

  virtual void
  initializeRenderComponents();

  virtual void
  destroyDisplay();

  virtual void
  destroyRenderer();

  /**
   * Draws the frame into the current image of the swap chain, which is already in the
   * RenderTarget state and must be left in it. Begins and ends its own rendering.
   */
  virtual void
  onRender(ICommandList& commandList, const ISwapChain& swapChain, float deltaTime) = 0;

  /**
   * Runs after the frame was submitted and presented, for work that submits on its own.
   */
  virtual void
  onPostPresent() {}

 private:
  void
  render(const float deltaTime);

  void
  resize(uint32 width, uint32 height);

  void
  bindEvents();

 private:
  bool m_running = true;
  SPtr<DisplayEventHandle> m_eventhandler;
  SPtr<DisplaySurface> m_display;

  SPtr<ISwapChain> m_swapChain;

  HEvent m_resizeEvent; ///< Event for handling display resize
  HEvent m_closeEvent;  ///< Event for handling application close
};

} // namespace chEngineSDK
