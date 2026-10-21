/************************************************************************/
/**
 * @file chComponentRegister.cpp
 * @author AccelMR
 * @date 2026/10/14
 * @brief
 *  Makes components from the type id saved in a file.
 */
/************************************************************************/
#include "chCameraComponent.h"
#include "chComponentRegister.h"

#include "chLightComponent.h"
#include "chLogger.h"
#include "chModelComponent.h"
#include "chSceneManager.h"
#include "chShapeComponent.h"
#include "chSkyLightComponent.h"

namespace chEngineSDK {
namespace {
struct RegisteredType
{
  const ANSICHAR* name = nullptr;
  ComponentCreator creator = nullptr;
};

/*
 * Built on first use, so components of other modules can register at any time.
 */
UnorderedMap<UUID, RegisteredType>&
getTypes()
{
  static UnorderedMap<UUID, RegisteredType> types = [] {
    UnorderedMap<UUID, RegisteredType> builtIn;
    const auto addBuiltIn = [&builtIn]<typename T>() {
      builtIn[TypeTraits<T>::getTypeId()] = {
          .name = TypeTraits<T>::getTypeName(),
          .creator = []() { return UniquePtr<Component>(new T()); }};
    };
    addBuiltIn.operator()<ModelComponent>();
    addBuiltIn.operator()<DirectionalLightComponent>();
    addBuiltIn.operator()<PointLightComponent>();
    addBuiltIn.operator()<SpotLightComponent>();
    addBuiltIn.operator()<SkyLightComponent>();
    addBuiltIn.operator()<ShapeComponent>();
    addBuiltIn.operator()<CameraComponent>();
    return builtIn;
  }();
  return types;
}
} // namespace

/*
 */
UniquePtr<Component>
ComponentRegister::create(const UUID& typeId)
{
  const UnorderedMap<UUID, RegisteredType>& types = getTypes();
  const auto it = types.find(typeId);
  return it != types.end() ? it->second.creator() : nullptr;
}

/*
 */
const ANSICHAR*
ComponentRegister::getTypeName(const UUID& typeId)
{
  const UnorderedMap<UUID, RegisteredType>& types = getTypes();
  const auto it = types.find(typeId);
  return it != types.end() ? it->second.name : "Unknown";
}

/*
 */
void
ComponentRegister::add(const UUID& typeId, const ANSICHAR* typeName, ComponentCreator creator)
{
  RegisteredType& type = getTypes()[typeId];
  if (type.creator) {
    CH_LOG_WARNING(SceneManagerLog,
                   "Component type {0} was registered twice; the last one wins.", typeName);
  }
  type = {.name = typeName, .creator = creator};
}

} // namespace chEngineSDK
