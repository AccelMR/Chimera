/************************************************************************/
/**
 * @file chDisplayEventHandleSDL.h
 * @author AccelMR
 * @date 2025/04/06
 * @brief SDL specific implementation of DisplaySurface event handle.
 * @details This file uses external library to handle events.
 */
/************************************************************************/
#include "chDisplayEventHandle.h"


#ifdef CH_CROSS_WINDOW
#include "CrossWindow/CrossWindow.h"

FORCEINLINE chEngineSDK::chKeyBoard::Key
translateSDLKey(::xwin::Key key) {
  // This works because the enum values are the same in Chimera and CrossWindow.
  return static_cast<chEngineSDK::chKeyBoard::Key>(key);
}

namespace chEngineSDK{

/*
*/
DisplayEventHandle::DisplayEventHandle() {
  m_platformPtr = new ::xwin::EventQueue();
}

/*
*/
DisplayEventHandle::~DisplayEventHandle() {
  if (m_platformPtr) {
    delete m_platformPtr;
    m_platformPtr = nullptr;
  }
}

/*
*/
PlatformPtr
DisplayEventHandle::getPlatformPtr() {
  return m_platformPtr;
}

/*
*/
void
DisplayEventHandle::update() {
  if (m_platformPtr) {
    m_platformPtr->update();
  }

  while (!m_platformPtr->empty()) {
    const ::xwin::Event event = m_platformPtr->front();
    
    switch (event.type) {

      case ::xwin::EventType::MouseInput:
      break;

      case ::xwin::EventType::Keyboard:
      break;

      case ::xwin::EventType::Close:
        addEvent(PLATFORM_EVENT_TYPE::kCLOSE);
      break;

      default:
      break;
    }

    m_platformPtr->pop();
  }

}

} // namespace chEngineSDK
#endif //CH_CROSS_WINDOW