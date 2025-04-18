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

#ifdef CH_CROSS_WINDOW
#include "CrossWindow/CrossWindow.h"
#include "chXCBGlobals.h"

using namespace chEngineSDK::XCBGlobals;
namespace chEngineSDK {

#if USING(CH_PLATFORM_LINUX)


bool 
initializeXCB(int argc, const char** argv) {
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

  if (connection) {
    setXCBConnection(connection);
  }

  if (screen) {
    setXCBScreen(screen);
  }

  xwin::init(argc, argv, connection, screen);

  return true;
}
#endif //USING(CH_PLATFORM_LINUX)

#if USING(CH_PLATFORM_WIN32)

static HINSTANCE g_hInstance = nullptr;
static HINSTANCE g_hPrevInstance = nullptr;
static LPSTR g_lpCmdLine = nullptr;
static int g_nCmdShow = 0;

bool 
initializeWin32(int argc, const char** argv) {
  g_hInstance = GetModuleHandle(NULL);
  g_hPrevInstance = nullptr;
  g_lpCmdLine = GetCommandLineA();
  g_nCmdShow = SW_SHOWDEFAULT;
  
  // Inicializar CrossWindow con estos parámetros
  xwin::init(g_hInstance, g_hPrevInstance, g_lpCmdLine, g_nCmdShow);
  
  return true;
}

void 
cleanupWin32() {
  // Windows specific cleanup code
}
#endif //USING(CH_PLATFORM_WIN32)

bool 
initializePlatformSpecific(int argc, const char** argv) {
#if USING(CH_PLATFORM_LINUX)
  const bool bIsXCBinitialized = initializeXCB(argc, argv);
  const bool bIsXCBKeySymbolinitialized =  initXCBKeySymbols();
  return bIsXCBinitialized && bIsXCBKeySymbolinitialized;
#elif USING(CH_PLATFORM_WIN32)
  return initializeWin32(argc, argv);
#endif //USING(CH_PLATFORM_WIN32)
}

/*
*/
void 
cleanupPlatformSpecific() {
#if USING(CH_PLATFORM_LINUX)
  cleanupXCB();
#elif USING(CH_PLATFORM_WIN32)
  cleanupWin32();
#endif //USING(CH_PLATFORM_WIN32)
}

/*
*/
NODISCARD bool
DisplaySurface::init(ScreenDescriptor desc, SPtr<DisplayEventHandle> eventHandler) {

  CommandParser& commandParser = CommandParser::getInstance();
  int argc = static_cast<int>(commandParser.getArgc());
  const char** argv = const_cast<const char**>(commandParser.getArgv());
  if (!initializePlatformSpecific(argc, argv)) {
    //CH_EXCEPT(InternalErrorException, "DisplaySurface.init() - XCB initialization failed.");
    return false;
  }

  ::xwin::WindowDesc windowDesc;
  windowDesc.name = desc.name;
  windowDesc.title = desc.title;
  windowDesc.width = desc.width;
  windowDesc.height = desc.height;
  windowDesc.visible = true;

  m_displayHandle = new ::xwin::Window();
  ::xwin::EventQueue* eventQueue = static_cast<::xwin::EventQueue*>(eventHandler->getPlatformPtr());

  if (!m_displayHandle->create(windowDesc, *eventQueue)) {
    //CH_EXCEPT(InternalErrorException, "DisplaySurface.init() - Window creation failed.");
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

  cleanupPlatformSpecific();
}

/*
*/
uint32
DisplaySurface::getPlatformHandlerInt() {
  #if USING(CH_PLATFORM_LINUX)
  // For this I had to modify the CrossWindow library to expose mXcbWindowId
  // if this throws an error, you need to modify the CrossWindow library at XCBWindow.h
  return m_displayHandle->mXcbWindowId;
  #elif USING(CH_PLATFORM_WIN32)
  // If somehting is needed for windows it can go here
  // but for now we are not using it.
  // Windows implementation oif the xwin library does expose a lot of things
  // so we can use it directly.
  return INVALID_INDEX;
  #endif //USING(CH_PLATFORM_WIN32)
}

} // namespace chEngineSDK
#endif //CH_CROSS_WINDOW

