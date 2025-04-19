/************************************************************************/
/**
 * @file chDisplayEvent.h
 * @author AccelMR
 * @date 2022/09/12
 *   DisplaySurface event wrapper. Handles all event from OS. 
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesCore.h"

#include "chDisplayEventData.h"

namespace chEngineSDK{
/*
 * Description: 
 *     Platform event types used along chimera.
 */
enum class PlatformEventType {
  NonePlatformEvent = 0,
  MouseMove,
  MouseButton,
  MouseWheel,
  Keyboard,
  Close,
  Resize,

  Count
};


/*
 * Description: 
 *     DisplaySurface event.
 *
 */
class CH_CORE_EXPORT DisplayEvent
{
 public:
 /*
  *   Default constructor
  */
  DisplayEvent(PlatformEventType _type)
    : m_type(_type), m_data(std::monostate{}) {}

  DisplayEvent(PlatformEventType _type, MouseMoveData mouseData)
    : m_type(_type), m_data(std::move(mouseData)) { }

  DisplayEvent(PlatformEventType _type, KeyBoardData keyBoardData)
    : m_type(_type), m_data(std::move(keyBoardData)) { }

  DisplayEvent(PlatformEventType _type, ResizeData resizeData)
    : m_type(_type), m_data(std::move(resizeData)) { } 

  DisplayEvent(PlatformEventType _type, MouseButtonData mouseButtonData)
    : m_type(_type), m_data(std::move(mouseButtonData)) { }

  DisplayEvent(PlatformEventType _type, MouseWheelData mouseWheelData)
    : m_type(_type), m_data(std::move(mouseWheelData)) { }

 /*
  *  Default destructor.
  */
  ~DisplayEvent() = default;

  FORCEINLINE PlatformEventType
  getType() const{ return m_type; }

  FORCEINLINE const DisplayEventData&
  getData() const{ return m_data; }

  FORCEINLINE String
  getTypeAsString() const;

 private:
  PlatformEventType m_type;
  DisplayEventData m_data;
};

/*
*/
String 
DisplayEvent::getTypeAsString() const {
  switch (m_type)
  {
  case PlatformEventType::MouseMove:
    return "Mouse Move";
    break;
  case PlatformEventType::Keyboard:
    return "Key Board";
    break;
  case PlatformEventType::Close:
    return "Close";
    break;
  case PlatformEventType::Resize:
    return "Resize";
    break;
  case PlatformEventType::MouseButton:
    return "Mouse Button";
    break;
  case PlatformEventType::NonePlatformEvent:
  default:
    return "None";
    break;
  }
}

}