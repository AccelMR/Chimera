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
enum class PLATFORM_EVENT_TYPE {
  kNONE = 0,
  kMOUSE_MOVE,
  kKEYBOARD,
  kCLOSE,
  kRESIZE,

  kCOUNT
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
  DisplayEvent(PLATFORM_EVENT_TYPE _type)
    : m_type(_type), m_data(std::monostate{}) {}

  DisplayEvent(PLATFORM_EVENT_TYPE _type, MouseMoveData mouseData)
    : m_type(_type), m_data(std::move(mouseData)) { }

  DisplayEvent(PLATFORM_EVENT_TYPE _type, KeyBoardData keyBoardData)
    : m_type(_type), m_data(std::move(keyBoardData)) { }

  DisplayEvent(PLATFORM_EVENT_TYPE _type, ResizeData resizeData)
    : m_type(_type), m_data(std::move(resizeData)) { } 
 /*
  *  Default destructor.
  */
  ~DisplayEvent() = default;

  FORCEINLINE PLATFORM_EVENT_TYPE
  getType() const{ return m_type; }

  FORCEINLINE const DisplayEventData&
  getData() const{ return m_data; }

  FORCEINLINE String
  getTypeAsString() const;

 private:
  PLATFORM_EVENT_TYPE m_type;
  DisplayEventData m_data;
};

/*
*/
String 
DisplayEvent::getTypeAsString() const {
  switch (m_type)
  {
  case PLATFORM_EVENT_TYPE::kMOUSE_MOVE:
    return "Mouse Move";
    break;
  case PLATFORM_EVENT_TYPE::kKEYBOARD:
    return "Key Board";
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