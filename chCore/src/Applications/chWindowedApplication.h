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

  /**
   * Loads the graphics API plugin without initializing it, so the main window can be made
   * with the flags it asks for.
   */
  virtual void
  loadGraphicsAPI();

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
   * Draws the frame into the current image of the swap chain. The image starts with
   * undefined contents (state Undefined) and must be left in the Present state; importing
   * it into a render graph with those two states does both. It only draws what update()
   * decided (a RenderFrame), so it can move to a render thread later.
   */
  virtual void
  onRender(ICommandList& commandList, const ISwapChain& swapChain) = 0;

  /**
   * Runs after the frame was submitted and the main swap chain presented; other swap chains
   * acquired during the frame are presented here.
   */
  virtual void
  onPostPresent() {}

  /**
   * Called when the main window is asked to close. Returning false keeps the application
   * running, for instance to ask about unsaved work; call quit() later to close.
   */
  NODISCARD virtual bool
  onCloseRequested() { return true; }

  /**
   * Id the desktop knows the program by when Application.Id is empty, so the editor and a
   * game get different window rules without a config file.
   */
  NODISCARD virtual StringView
  getDefaultApplicationId() const { return "chimera-engine"; }

  /**
   * Ends the main loop after the current frame.
   */
  void
  quit() { m_running = false; }

  /**
   * Writes the main window's size and maximized state into /Saved/Config/<configFile>.ini,
   * so the next start opens it the same way. Call it before destroyModules closes the window.
   */
  void
  saveWindowPlacement(StringView configFile) const;

 private:
  void
  render();

  void
  resize(uint32 width, uint32 height);

  void
  requestResize();

  void
  applyPendingResize();

  void
  onSwapChainOutOfDate();

  void
  bindEvents();

 private:
  bool m_running = true;
  bool m_bResizePending = false;
  bool m_bSwapChainOutOfDate = false;
  int64 m_lastResizeRequestNs = 0;
  SPtr<DisplayEventHandle> m_eventhandler;
  SPtr<DisplaySurface> m_display;

  SPtr<ISwapChain> m_swapChain;

  HEvent m_resizeEvent; ///< Event for handling display resize
  HEvent m_closeEvent;  ///< Event for handling application close
};

} // namespace chEngineSDK
