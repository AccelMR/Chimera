/************************************************************************/
/**
 * @file chDisplaySurfaceCrossWin.cpp
 * @author AccelMR
 * @date 2025/04/06
 *  Implementation of DisplaySurface for cross platform windowing.
 *  This file uses a external library to handle window creation and events.
 */
/************************************************************************/
#include "chDisplaySurface.h"
#include "chCommandParser.h"
#include "chException.h"

#include "CrossWindow/CrossWindow.h"

#ifdef CH_CROSS_WINDOW

namespace chEngineSDK {
static xcb_connection_t * g_connection = nullptr;

#if USING(CH_PLATFORM_LINUX)
bool initializeXCB(int argc, const char** argv) {
  int screenNum = 0;
  xcb_connection_t* connection = xcb_connect(nullptr, &screenNum);
  if (xcb_connection_has_error(connection) > 0) {
    return false;
  }
  const xcb_setup_t* setup = xcb_get_setup(connection);
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
  for (int i = 0; i < screenNum; ++i) {
    xcb_screen_next(&iter);
  }
  xcb_screen_t* screen = iter.data;
  g_connection = connection;

  xwin::init(argc, argv, g_connection, screen);

  return true;
}

void cleanupXCB() {
  if (g_connection) {
    xcb_disconnect(g_connection);
    g_connection = nullptr;
  }
}


#endif //USING(CH_PLATFORM_LINUX)


/*
*/
NODISCARD bool
DisplaySurface::init(SCREEN_DESC desc, SPtr<DisplayEventHandle> eventHandler) {

#if USING(CH_PLATFORM_LINUX)
  CommandParser& commandParser = CommandParser::getInstance();
  int argc = static_cast<int>(commandParser.getArgc());
  const char** argv = const_cast<const char**>(commandParser.getArgv());
  if (!initializeXCB(argc, argv)) {
    CH_EXCEPT(InternalErrorException, "DisplaySurface.init() - XCB initialization failed.");
    return false;
  }
#endif //USING(CH_PLATFORM_LINUX)

  ::xwin::WindowDesc windowDesc;
  windowDesc.name = desc.name;
  windowDesc.title = desc.title;
  windowDesc.width = desc.width;
  windowDesc.height = desc.height;
  windowDesc.visible = true;

  m_displayHandle = new ::xwin::Window();
  ::xwin::EventQueue* eventQueue = static_cast<::xwin::EventQueue*>(eventHandler->getPlatformPtr());
  try {
    if (!m_displayHandle->create(windowDesc, *eventQueue)) {
      CH_EXCEPT(InternalErrorException, "DisplaySurface.init() - Window creation failed.");
      return false;
    }
  } 
  catch (...) {
    return false;
  }

  m_width = desc.width;
  m_height = desc.height;

  return true;
}

/*
*/
void
DisplaySurface::close() {
  if (m_displayHandle) {
    m_displayHandle->close();

    delete m_displayHandle;
    m_displayHandle = nullptr;
  }

#if USING(CH_PLATFORM_LINUX)
  cleanupXCB();
#endif //USING(CH_PLATFORM_LINUX)
}

} // namespace chEngineSDK
#endif //CH_CROSS_WINDOW

