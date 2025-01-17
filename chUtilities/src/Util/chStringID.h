/************************************************************************/
/**
 * @file chStringID.h
 * @author AccelMR
 * @date 2022/09/28
 *    String to be used as name. Which means it can't be repeated and 
 *  will return a unique id.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesUtilities.h"


namespace chEngineSDK{
class CH_UTILITY_EXPORT StringID
{
 public:
 /*
  *   Default constructor
  */
  StringID(const String& name);

  StringID(const char* name) : StringID(String(name)) {}

 /*
  *   Default destructor
  */
  ~StringID() = default;

  FORCEINLINE SIZE_T
  getId() const;

  FORCEINLINE String
  getName() const;

  void
  setId(const String& newName);

  FORCEINLINE bool
  operator==(const StringID& strId) const;

 private:
  String m_name;
  SIZE_T m_id;
};

FORCEINLINE SIZE_T
StringID::getId() const {
  return m_id;
}

FORCEINLINE bool
StringID::operator==(const StringID& strId) const{
  return m_id == strId.getId();
}

FORCEINLINE String
StringID::getName() const {
  return m_name;
}

}

