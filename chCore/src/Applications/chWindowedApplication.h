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

#include "chIRenderer.h"

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
  NODISCARD FORCEINLINE virtual LinearColor
  getBackgroundColor() const {
    return LinearColor::Black;
  }

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
   * Records the scene into the frame command list, before the swap chain image is drawn.
   */
  virtual RendererOutput
  onRender(ICommandList& commandList, float deltaTime) = 0;

  /**
   * Draws into the swap chain image; rendering to it has already begun.
   */
  virtual void
  onPresent(const RendererOutput& rendererOutput,
            ICommandList& commandList,
            uint32 swapChainWidth,
            uint32 swapChainHeight) = 0;

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
