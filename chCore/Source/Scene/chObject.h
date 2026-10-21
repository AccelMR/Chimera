/************************************************************************/
/**
 * @file chObject.h
 * @author AccelMR
 * @date 2025/04/26
 * @brief
 * Object class, the name and UUID shared by scene objects.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chUUID.h"

namespace chEngineSDK {
/**
 * Exists to give GameObject and Scene one name and UUID. It is never used through an
 * Object pointer, so it has no virtual destructor; a class becomes an Object only when a
 * system needs a common root (weak references by UUID, reflection).
 */
class CH_CORE_EXPORT Object
{
 public:
  explicit Object(const String& name, UUID id)
   : m_name(name),
     m_uuid(id)
  {}

  NODISCARD FORCEINLINE const String&
  getName() const
  {
    return m_name;
  }

  void
  setName(const String& name)
  {
    m_name = name;
  }

  NODISCARD FORCEINLINE const UUID&
  getUUID() const
  {
    return m_uuid;
  }

 protected:
  ~Object() = default;

  String m_name;
  UUID m_uuid;
};

} // namespace chEngineSDK
