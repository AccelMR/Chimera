/************************************************************************/
/**
 * @file chObject.h
 * @author AccelMR
 * @date 2025/04/26
 * @brief
 * Object class representing a generic object in the engine.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
/**
 * @brief Object class representing a generic object in the engine.
 */
class CH_CORE_EXPORT Object
{
 public:
  /**
   * Constructor
   * 
   * @param name Name of the object
   */
  Object(const String& name) : m_name(name) {}

  /**
   * Destructor
   */
  virtual ~Object() = default;

  /**
   * Get the name of the object
   * 
   * @return Name of the object
   */
  NODISCARD const String& 
  getName() const { return m_name; }

 protected:
  String m_name; ///< Name of the object
};

} // namespace chEngineSDK