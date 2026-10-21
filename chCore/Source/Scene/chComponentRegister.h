/************************************************************************/
/**
 * @file chComponentRegister.h
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  Makes components from the type id saved in a file.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chComponent.h"
#include "chTypeTraits.h"
#include "chUUID.h"

namespace chEngineSDK {

using ComponentCreator = UniquePtr<Component> (*)();

/**
 * Exists so a saved GameObject can make its components again from the type ids in the file.
 * The components of chCore are registered on first use; a module with its own components
 * registers them when it starts.
 */
class CH_CORE_EXPORT ComponentRegister
{
 public:
  /**
   * T needs DECLARE_TYPE_TRAITS and a default constructor.
   */
  template<typename T>
  static void
  registerType()
  {
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component.");
    static_assert(TypeTraits<T>::DECLARED, "T needs DECLARE_TYPE_TRAITS.");
    add(TypeTraits<T>::getTypeId(), TypeTraits<T>::getTypeName(),
        []() { return UniquePtr<Component>(new T()); });
  }

  /**
   * Null for a type that is not registered.
   */
  NODISCARD static UniquePtr<Component>
  create(const UUID& typeId);

  /**
   * "Unknown" for a type that is not registered.
   */
  NODISCARD static const ANSICHAR*
  getTypeName(const UUID& typeId);

 private:
  static void
  add(const UUID& typeId, const ANSICHAR* typeName, ComponentCreator creator);
};

} // namespace chEngineSDK
