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

#if USING(CH_PLATFORM_LINUX)
#include "chXCBGlobals.h"
#endif //USING(CH_PLATFORM_LINUX)

namespace chCrossWindowHelpers {

FORCEINLINE chEngineSDK::chKeyBoard::Key
translateKey(::xwin::Key key) {
  // This works because the enum values are the same in Chimera and CrossWindow.
  return static_cast<chEngineSDK::chKeyBoard::Key>(key);
}

FORCEINLINE chEngineSDK::KEYBOARD_STATE
translateKeyState(::xwin::ButtonState state) {
  switch (state) {
    case ::xwin::ButtonState::Pressed:
      return chEngineSDK::KEYBOARD_STATE::PRESSED;
    case ::xwin::ButtonState::Released:
      return chEngineSDK::KEYBOARD_STATE::RELEASED;
    default:
      return chEngineSDK::KEYBOARD_STATE::NONE;
  }
}

FORCEINLINE uint16_t
translateKeyModifier(::xwin::ModifierState modifier) {
  uint16_t result = 0;
  if (modifier.ctrl) {
    result |= static_cast<uint16_t>(chEngineSDK::KEY_MODIFIER::LCTRL);
  }
  if (modifier.alt) {
    result |= static_cast<uint16_t>(chEngineSDK::KEY_MODIFIER::LALT);
  }
  if (modifier.shift) {
    result |= static_cast<uint16_t>(chEngineSDK::KEY_MODIFIER::LSHIFT);
  }
  if (modifier.meta) {
    result |= static_cast<uint16_t>(chEngineSDK::KEY_MODIFIER::LMETA);
  }
  return result;
}

#if USING(CH_PLATFORM_LINUX)
FORCEINLINE chEngineSDK::chKeyBoard::Key
translateXCBKey(xcb_keycode_t keycode, xcb_key_symbols_t* keysyms) {
    xcb_keysym_t keysym = xcb_key_symbols_get_keysym(keysyms, keycode, 0);
    
    switch (keysym) {
        case XK_Escape: return chEngineSDK::chKeyBoard::Key::Escape;
        case XK_a: case XK_A: return chEngineSDK::chKeyBoard::Key::A;
        case XK_b: case XK_B: return chEngineSDK::chKeyBoard::Key::B;
        case XK_c: case XK_C: return chEngineSDK::chKeyBoard::Key::C;
        case XK_d: case XK_D: return chEngineSDK::chKeyBoard::Key::D;
        case XK_w: case XK_W: return chEngineSDK::chKeyBoard::Key::W;

        
        default: return chEngineSDK::chKeyBoard::Key::KeysMax;
    }
}
#endif // USING(CH_PLATFORM_LINUX)


} // namespace chCrossWindowHelpers
using namespace chCrossWindowHelpers;

namespace chEngineSDK{

/*
*/
DisplayEventHandle::DisplayEventHandle() {
#if USING(CH_PLATFORM_WIN32)
  m_platformPtr = new ::xwin::EventQueue();
#elif USING(CH_PLATFORM_LINUX)
 
#endif //USING(CH_PLATFORM_LINUX)
}

/*
*/
DisplayEventHandle::~DisplayEventHandle() {

#if USING(CH_PLATFORM_WIN32)
  if (m_platformPtr) {
    delete m_platformPtr;
    m_platformPtr = nullptr;
  }
#elif USING(CH_PLATFORM_LINUX)
  cleanupXCBKeySymbols();
#endif //USING(CH_PLATFORM_LINUX)

}

/*
*/
PlatformPtr
DisplayEventHandle::getPlatformPtr() {
  //TODO: this is awful and should be changed to a better solution.
  #if USING(CH_PLATFORM_LINUX)
    initXCBKeySymbols();
  #endif //USING(CH_PLATFORM_LINUX)

  return m_platformPtr;
}

/*
*/
void
DisplayEventHandle::update() {
#if USING(CH_PLATFORM_WIN32)
  if (!m_platformPtr) {
    return;
  }

  m_platformPtr->update();

  while (!m_platformPtr->empty()) {
    const ::xwin::Event event = m_platformPtr->front();
    if (event.type == ::xwin::EventType::MouseMove || 
        event.type == ::xwin::EventType::Focus) {
      m_platformPtr->pop();
      continue;
    }
    
    switch (event.type) {

      case ::xwin::EventType::MouseInput:
      break;

      case ::xwin::EventType::MouseMove:
        addEvent(PLATFORM_EVENT_TYPE::kMOUSE_MOVE, MouseMoveData(
                 event.data.mouseMove.x, 
                 event.data.mouseMove.y,
                 event.data.mouseMove.screenx,
                 event.data.mouseMove.screeny,
                 event.data.mouseMove.deltax,
                 event.data.mouseMove.deltay));
      break;

      case ::xwin::EventType::Keyboard:
        addEvent(PLATFORM_EVENT_TYPE::kKEYBOARD, KeyBoardData(
                translateKey(event.data.keyboard. key),
                translateKeyState(event.data.keyboard.state),
                translateKeyModifier(event.data.keyboard.modifiers)));
      break;

      case ::xwin::EventType::Close:
        addEvent(PLATFORM_EVENT_TYPE::kCLOSE);
      break;

      default:
      break;
    }

    m_platformPtr->pop();
  }
#elif USING(CH_PLATFORM_LINUX)
  // Procesar solo eventos XCB directamente
  xcb_connection_t* connection = getXCBConnection();
  if (connection) {
    xcb_key_symbols_t* keySymbols = getXCBKeySymbols();
    if (keySymbols) {
      xcb_generic_event_t* event;
      while ((event = xcb_poll_for_event(connection))) {
        uint8_t event_type = event->response_type & ~0x80;
        
        switch (event_type) {
          case XCB_KEY_PRESS: {
            xcb_key_press_event_t* keyEvent = reinterpret_cast<xcb_key_press_event_t*>(event);
            KeyBoardData keyData;
            keyData.key = translateXCBKey(keyEvent->detail, keySymbols);
            keyData.state = KEYBOARD_STATE::DOWN;
            
            // Procesar modificadores
            uint16_t modifiers = 0;
            if (keyEvent->state & XCB_MOD_MASK_SHIFT) 
              modifiers |= static_cast<uint16_t>(KEY_MODIFIER::LSHIFT);
            if (keyEvent->state & XCB_MOD_MASK_CONTROL) 
              modifiers |= static_cast<uint16_t>(KEY_MODIFIER::LCTRL);
            if (keyEvent->state & XCB_MOD_MASK_1) 
              modifiers |= static_cast<uint16_t>(KEY_MODIFIER::LALT);
            
            keyData.modifiers = modifiers;
            addEvent(PLATFORM_EVENT_TYPE::kKEYBOARD, keyData);
            break;
          }
          
          case XCB_KEY_RELEASE: {
            xcb_key_release_event_t* keyEvent = 
              reinterpret_cast<xcb_key_release_event_t*>(event);
            KeyBoardData keyData;
            keyData.key = translateXCBKey(keyEvent->detail, keySymbols);
            keyData.state = KEYBOARD_STATE::UP;
            
            // Procesar modificadores
            uint16_t modifiers = 0;
            if (keyEvent->state & XCB_MOD_MASK_SHIFT) 
              modifiers |= static_cast<uint16_t>(KEY_MODIFIER::LSHIFT);
            if (keyEvent->state & XCB_MOD_MASK_CONTROL) 
              modifiers |= static_cast<uint16_t>(KEY_MODIFIER::LCTRL);
            if (keyEvent->state & XCB_MOD_MASK_1) 
              modifiers |= static_cast<uint16_t>(KEY_MODIFIER::LALT);
            
            keyData.modifiers = modifiers;
            addEvent(PLATFORM_EVENT_TYPE::kKEYBOARD, keyData);
            break;
          }
          
          case XCB_CLIENT_MESSAGE: {
            xcb_client_message_event_t* clientMsg = reinterpret_cast<xcb_client_message_event_t*>(event);
            // Verificar si es un evento de cierre de ventana
            addEvent(PLATFORM_EVENT_TYPE::kCLOSE);
            break;
          }
          
          case XCB_MOTION_NOTIFY: {
            xcb_motion_notify_event_t* motionEvent = reinterpret_cast<xcb_motion_notify_event_t*>(event);
            // No tenemos delta en XCB, así que usamos 0
            addEvent(PLATFORM_EVENT_TYPE::kMOUSE_MOVE, MouseMoveData(
                     static_cast<uint32>(motionEvent->event_x),
                     static_cast<uint32>(motionEvent->event_y),
                     static_cast<uint32>(motionEvent->root_x),
                     static_cast<uint32>(motionEvent->root_y),
                     0, 0));
            break;
          }
          
          case XCB_CONFIGURE_NOTIFY: {
            xcb_configure_notify_event_t* configEvent = reinterpret_cast<xcb_configure_notify_event_t*>(event);
            addEvent(PLATFORM_EVENT_TYPE::kRESIZE, ResizeData{
                     static_cast<uint32>(configEvent->width),
                     static_cast<uint32>(configEvent->height)});
            break;
          }
        }
        
        free(event);
      }
    }
  }

#endif //USING(CH_PLATFORM_LINUX)

}

} // namespace chEngineSDK
#endif //CH_CROSS_WINDOW