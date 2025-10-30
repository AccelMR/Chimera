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

#include "chUUID.h"

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
  Object(const String& name) :
  m_name(name){}

  /*
   * Constructor with UUID
   *
   * @param name Name of the object
   * @param id UUID of the object
  */
  Object(const String& name, UUID id) :
    m_name(name),
    m_uuid(id) {}

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

  /**
   * Get the UUID of the object
   *
   * @return UUID of the object
   */
  NODISCARD const UUID&
  getUUID() const { return m_uuid; }

 protected:
  String m_name; ///< Name of the object
  UUID m_uuid = UUID::null(); ///< Unique identifier for the object
};

} // namespace chEngineSDK
