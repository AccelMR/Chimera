/************************************************************************/
/**
 * @file chScreenEvent.h
 * @author AccelMR
 * @date 2022/09/12
 *   Screen event wrapper. Handles all event from OS. 
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesCore.h"

#include "chScreenEventData.h"

namespace chEngineSDK{
/*
 * Description: 
 *     Platform event types used along chimera.
 */
enum class PLATFORM_EVENT_TYPE {
  kNONE = 0,
  kMOUSE_MOVE,
  kKEY_DOWN,
  kKEY_UP,
  kCLOSE,
  kRESIZE,

  kCOUNT
};


/*
 * Description: 
 *     Screen event.
 *
 */
class CH_CORE_EXPORT ScreenEvent
{
 public:
 /*
  *   Default constructor
  */
  ScreenEvent(PLATFORM_EVENT_TYPE _type): m_type(_type) {}

  ScreenEvent(PLATFORM_EVENT_TYPE _type, MouseMoveData mouseData)
    : m_type(_type) { m_data.mouseMoveData = mouseData; }

  ScreenEvent(PLATFORM_EVENT_TYPE _type, KeyBoardData keyBoardData)
    : m_type(_type) { m_data.keyBoardData = keyBoardData; }

  ScreenEvent(PLATFORM_EVENT_TYPE _type, ResizeData resizeData)
    : m_type(_type) { m_data.resizeData = resizeData; }
 /*
  *  Default destructor.
  */
  ~ScreenEvent() = default;

  FORCEINLINE PLATFORM_EVENT_TYPE
  getType() const{ return m_type; }

  FORCEINLINE ScreenEventData
  getData() const{ return m_data; }

  FORCEINLINE String
  getTypeAsString() const;

 private:
  PLATFORM_EVENT_TYPE m_type;
  ScreenEventData m_data;
};

/*
*/
String 
ScreenEvent::getTypeAsString() const {
  switch (m_type)
  {
  case PLATFORM_EVENT_TYPE::kMOUSE_MOVE:
    return "Mouse Move";
    break;
  case PLATFORM_EVENT_TYPE::kKEY_DOWN:
    return "Key Down";
    break;
  case PLATFORM_EVENT_TYPE::kKEY_UP:
    return "Key Up";
    break;
  case PLATFORM_EVENT_TYPE::kCLOSE:
    return "Close";
    break;
  case PLATFORM_EVENT_TYPE::kRESIZE:
    return "Resize";
    break;
  case PLATFORM_EVENT_TYPE::kNONE:
  default:
    return "None";
    break;
  }
}

}