/************************************************************************/
/**
 * @file chStringID.cpp
 * @author AccelMR
 * @date 2022/09/28
 *    String to be used as name. Which means it can't be repeated and
 *  will return a unique id.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chStringID.h"

#include "chStringUtils.h"

namespace chEngineSDK{

#if USING(CH_DEBUG_MODE)
Set<SIZE_T> g_hashedNames;
#endif //#if USING(CH_DEBUG_MODE)
/*
*/
StringID::StringID(const String& name) {
  setId(name);
}

/*
*/
void
StringID::setId(const String& newName) {
  m_name = newName;
  String tohash = StringUtils::trim(newName);
  tohash = StringUtils::toLower(tohash);
  m_id = std::hash<std::string>{}(tohash);
#if USING(CH_DEBUG_MODE)
  auto search = g_hashedNames.find( m_id);
  if (search != g_hashedNames.end()) {
    //TODO: fix this
    //LOG_ERROR(StringUtils::format("Name {0} has been used before as StringID, same ID created.", newName));
  }
  else {
    g_hashedNames.insert(m_id);
  }
#endif //#if USING(CH_DEBUG_MODE)
}

}


