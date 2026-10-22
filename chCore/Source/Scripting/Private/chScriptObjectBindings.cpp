/************************************************************************/
/**
 * @file chScriptObjectBindings.cpp
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  The scene objects scripts reach through handles.
 */
/************************************************************************/
#include "chScriptBindings.h"

#include "chGameObject.h"
#include "chMath.h"
#include "chMatrix4.h"
#include "chScene.h"
#include "chScriptAsset.h"
#include "chScriptComponent.h"
#include "chScriptManager.h"
#include "chScriptMathTypes.h"
#include "chScriptObjectTypes.h"

namespace chEngineSDK {
namespace {
/*
 * The object is null once it died; scripts that kept the handle get an error.
 */
struct GameObjectHandle
{
  GameObject* object;
};

/*
 * Points at the object pointer inside the GameObject handle, which its user value keeps
 * alive, so both handles die with the object at once.
 */
struct TransformHandle
{
  GameObject* const* object;
};

/*
 * An object that left its scene but is still held elsewhere (undo, the editor selection) is
 * gone for scripts too.
 */
GameObject&
checkAlive(lua_State* state, GameObject* gameObject)
{
  if (!gameObject) {
    luaL_error(state, "the GameObject was destroyed");
  }
  if (!gameObject->getScene()) {
    luaL_error(state, "GameObject '%s' was destroyed", gameObject->getName().c_str());
  }
  return *gameObject;
}

NODISCARD FORCEINLINE bool
isAlive(const GameObject* gameObject)
{
  return gameObject && gameObject->getScene();
}

/*
 * A Vector3 at index, or three numbers starting there (no vector to make in hot code).
 */
Vector3
readVector3(lua_State* state, const int32 index)
{
  if (lua_type(state, index) == LUA_TNUMBER) {
    return Vector3(ScriptTraits<float>::check(state, index),
                   ScriptTraits<float>::check(state, index + 1),
                   ScriptTraits<float>::check(state, index + 2));
  }
  return ScriptTraits<Vector3>::check(state, index);
}

/*
 * Writes into the value at outIndex and returns it when one was passed, else pushes a new
 * one, so a getter called every frame can reuse one vector.
 */
template<typename T>
int32
pushResult(lua_State* state, const T& value, const int32 outIndex)
{
  if (!lua_isnoneornil(state, outIndex)) {
    ScriptTraits<T>::check(state, outIndex) = value;
    lua_pushvalue(state, outIndex);
    return 1;
  }
  ScriptTraits<T>::push(state, value);
  return 1;
}

/*
 * World values are read now instead of from the last Scene::updateTransforms, so they are
 * right after a move in the same frame. The cached world matrix is used while nothing up
 * the chain changed.
 */
NODISCARD bool
isChainDirty(const GameObject& gameObject)
{
  for (const GameObject* current = &gameObject; current; current = current->getParent()) {
    if (current->getTransform().isDirty()) {
      return true;
    }
  }
  return false;
}

Matrix4
getWorldMatrixNow(const GameObject& gameObject)
{
  if (!isChainDirty(gameObject)) {
    return gameObject.getTransform().getWorldMatrix();
  }
  Matrix4 world = gameObject.getTransform().getLocalMatrix();
  for (const GameObject* parent = gameObject.getParent(); parent; parent = parent->getParent()) {
    world = world * parent->getTransform().getLocalMatrix();
  }
  return world;
}

Vector3
getWorldPositionNow(const GameObject& gameObject)
{
  const Matrix4 world = getWorldMatrixNow(gameObject);
  return Vector3(world[3][0], world[3][1], world[3][2]);
}

/*
 * Quaternions compose right to left (a * b turns by b first), so the parent goes left.
 */
Quaternion
getWorldRotationNow(const GameObject& gameObject)
{
  Quaternion rotation = gameObject.getTransform().getLocalRotation();
  for (const GameObject* parent = gameObject.getParent(); parent; parent = parent->getParent()) {
    rotation = parent->getTransform().getLocalRotation() * rotation;
  }
  return rotation;
}

void
setWorldPosition(GameObject& gameObject, const Vector3& position)
{
  const GameObject* parent = gameObject.getParent();
  if (!parent) {
    gameObject.getTransform().setLocalPosition(position);
    return;
  }
  const Vector4 local =
      getWorldMatrixNow(*parent).getInverseAffine().transformPosition(position);
  gameObject.getTransform().setLocalPosition(Vector3(local.x, local.y, local.z));
}

void
setWorldRotation(GameObject& gameObject, const Quaternion& rotation)
{
  const GameObject* parent = gameObject.getParent();
  if (!parent) {
    gameObject.getTransform().setLocalRotation(rotation);
    return;
  }
  gameObject.getTransform().setLocalRotation(getWorldRotationNow(*parent).getInverse() *
                                             rotation);
}

GameObject&
checkTransformOwner(lua_State* state)
{
  return ScriptTraits<Transform>::checkOwner(state, 1);
}

int32
getLocalPosition(lua_State* state)
{
  return pushResult(state, checkTransformOwner(state).getTransform().getLocalPosition(), 2);
}

int32
setLocalPosition(lua_State* state)
{
  checkTransformOwner(state).getTransform().setLocalPosition(readVector3(state, 2));
  return 0;
}

int32
getWorldPosition(lua_State* state)
{
  return pushResult(state, getWorldPositionNow(checkTransformOwner(state)), 2);
}

int32
setWorldPositionScript(lua_State* state)
{
  setWorldPosition(checkTransformOwner(state), readVector3(state, 2));
  return 0;
}

int32
getLocalRotation(lua_State* state)
{
  return pushResult(state, checkTransformOwner(state).getTransform().getLocalRotation(), 2);
}

int32
setLocalRotation(lua_State* state)
{
  GameObject& gameObject = checkTransformOwner(state);
  gameObject.getTransform().setLocalRotation(ScriptTraits<Quaternion>::check(state, 2));
  return 0;
}

int32
getWorldRotation(lua_State* state)
{
  return pushResult(state, getWorldRotationNow(checkTransformOwner(state)), 2);
}

int32
setWorldRotationScript(lua_State* state)
{
  GameObject& gameObject = checkTransformOwner(state);
  setWorldRotation(gameObject, ScriptTraits<Quaternion>::check(state, 2));
  return 0;
}

int32
getLocalScale(lua_State* state)
{
  return pushResult(state, checkTransformOwner(state).getTransform().getLocalScale(), 2);
}

int32
setLocalScale(lua_State* state)
{
  checkTransformOwner(state).getTransform().setLocalScale(readVector3(state, 2));
  return 0;
}

/*
 * Moves along the world axes.
 */
int32
translateWorld(lua_State* state)
{
  GameObject& gameObject = checkTransformOwner(state);
  const Vector3 offset = readVector3(state, 2);
  if (!gameObject.getParent()) {
    Transform& transform = gameObject.getTransform();
    transform.setLocalPosition(transform.getLocalPosition() + offset);
    return 0;
  }
  setWorldPosition(gameObject, getWorldPositionNow(gameObject) + offset);
  return 0;
}

/*
 * Moves along the object's own axes (x forward, y right, z up), scale ignored.
 */
int32
translateLocal(lua_State* state)
{
  GameObject& gameObject = checkTransformOwner(state);
  const Vector3 offset = getWorldRotationNow(gameObject).rotateVector(readVector3(state, 2));
  setWorldPosition(gameObject, getWorldPositionNow(gameObject) + offset);
  return 0;
}

Quaternion
readEuler(lua_State* state, const int32 index)
{
  return Quaternion(Rotator(ScriptTraits<float>::check(state, index),
                            ScriptTraits<float>::check(state, index + 1),
                            ScriptTraits<float>::check(state, index + 2)));
}

/*
 * Turns around the object's own axes by pitch, yaw and roll in degrees.
 */
int32
rotateLocal(lua_State* state)
{
  Transform& transform = checkTransformOwner(state).getTransform();
  transform.setLocalRotation(transform.getLocalRotation() * readEuler(state, 2));
  return 0;
}

/*
 * Turns around the world axes by pitch, yaw and roll in degrees.
 */
int32
rotateWorld(lua_State* state)
{
  GameObject& gameObject = checkTransformOwner(state);
  setWorldRotation(gameObject, readEuler(state, 2) * getWorldRotationNow(gameObject));
  return 0;
}

/*
 * Turns the forward axis toward a world point, with world Z up and no roll.
 */
int32
lookAt(lua_State* state)
{
  GameObject& gameObject = checkTransformOwner(state);
  const Vector3 direction = readVector3(state, 2) - getWorldPositionNow(gameObject);
  const float flatLength = Math::sqrt(direction.x * direction.x + direction.y * direction.y);
  if (flatLength < Math::SMALL_NUMBER && Math::abs(direction.z) < Math::SMALL_NUMBER) {
    return 0;
  }
  const Degree yaw(Math::atan2(direction.y, direction.x));
  const Degree pitch(Math::atan2(direction.z, flatLength));
  setWorldRotation(gameObject, Quaternion(Rotator(pitch, yaw, Degree(0.0f))));
  return 0;
}

int32
getForward(lua_State* state)
{
  const Quaternion rotation = getWorldRotationNow(checkTransformOwner(state));
  return pushResult(state, rotation.rotateVector(Vector3::FORWARD), 2);
}

int32
getRight(lua_State* state)
{
  const Quaternion rotation = getWorldRotationNow(checkTransformOwner(state));
  return pushResult(state, rotation.rotateVector(Vector3::RIGHT), 2);
}

int32
getUp(lua_State* state)
{
  const Quaternion rotation = getWorldRotationNow(checkTransformOwner(state));
  return pushResult(state, rotation.rotateVector(Vector3::UP), 2);
}

int32
getGameObjectOfTransform(lua_State* state)
{
  ScriptTraits<GameObject>::push(state, checkTransformOwner(state));
  return 1;
}

const String&
getName(const GameObject& gameObject)
{
  return gameObject.getName();
}

void
setName(GameObject& gameObject, const StringView name)
{
  gameObject.setName(String(name));
}

uint32
getChildCount(const GameObject& gameObject)
{
  return static_cast<uint32>(gameObject.getChildren().size());
}

/*
 * Does not raise for a destroyed object: this is how scripts ask.
 */
int32
isValid(lua_State* state)
{
  const GameObjectHandle* handle = static_cast<const GameObjectHandle*>(
      luaL_checkudata(state, 1, ScriptHandleType<GameObject>::NAME));
  lua_pushboolean(state, isAlive(handle->object) ? 1 : 0);
  return 1;
}

int32
getTransform(lua_State* state)
{
  ScriptTraits<Transform>::pushOf(state, ScriptTraits<GameObject>::check(state, 1));
  return 1;
}

int32
getParent(lua_State* state)
{
  GameObject* parent = ScriptTraits<GameObject>::check(state, 1).getParent();
  if (parent) {
    ScriptTraits<GameObject>::push(state, *parent);
  }
  else {
    lua_pushnil(state);
  }
  return 1;
}

/*
 * Children are numbered from 1, as Lua does; nil past the end.
 */
int32
getChild(lua_State* state)
{
  const GameObject& gameObject = ScriptTraits<GameObject>::check(state, 1);
  const lua_Integer index = luaL_checkinteger(state, 2);
  const Vector<SPtr<GameObject>>& children = gameObject.getChildren();
  if (index < 1 || index > static_cast<lua_Integer>(children.size())) {
    lua_pushnil(state);
    return 1;
  }
  ScriptTraits<GameObject>::push(state, *children[static_cast<SIZE_T>(index - 1)]);
  return 1;
}

GameObject*
findIn(const Vector<SPtr<GameObject>>& objects, const StringView name, const bool bRecursive)
{
  for (const SPtr<GameObject>& object : objects) {
    if (object->getName() == name) {
      return object.get();
    }
  }
  if (bRecursive) {
    for (const SPtr<GameObject>& object : objects) {
      if (GameObject* found = findIn(object->getChildren(), name, true)) {
        return found;
      }
    }
  }
  return nullptr;
}

int32
pushFound(lua_State* state, GameObject* found)
{
  if (found) {
    ScriptTraits<GameObject>::push(state, *found);
  }
  else {
    lua_pushnil(state);
  }
  return 1;
}

int32
findChild(lua_State* state)
{
  const GameObject& gameObject = ScriptTraits<GameObject>::check(state, 1);
  return pushFound(state, findIn(gameObject.getChildren(), ScriptTraits<StringView>::check(state, 2),
                                 false));
}

/*
 * Children first, then their children, and so on.
 */
int32
findDescendant(lua_State* state)
{
  const GameObject& gameObject = ScriptTraits<GameObject>::check(state, 1);
  return pushFound(state, findIn(gameObject.getChildren(), ScriptTraits<StringView>::check(state, 2),
                                 true));
}

/*
 * Scene.find(name): the first object with that name in the scene of the running script.
 * Walks the whole tree, so look objects up once (in onStart), not every frame.
 */
int32
findInScene(lua_State* state)
{
  const StringView name = ScriptTraits<StringView>::check(state, 1);
  const Scene* scene = ScriptManager::instance().getCurrentScene();
  if (!scene) {
    return luaL_error(state, "Scene.find needs a running script's scene");
  }
  return pushFound(state, findIn(scene->getRootGameObjects(), name, true));
}

/*
 * The first enabled script component of the object whose script has that name.
 */
ScriptComponent*
findScript(const GameObject& gameObject, const StringView name)
{
  for (const UniquePtr<Component>& component : gameObject.getComponents()) {
    if (component->getTypeId() != TypeTraits<ScriptComponent>::getTypeId()) {
      continue;
    }
    ScriptComponent& scriptComponent = static_cast<ScriptComponent&>(*component);
    const SPtr<ScriptAsset>& script = scriptComponent.getScript();
    if (script && StringView(script->getName()) == name) {
      return &scriptComponent;
    }
  }
  return nullptr;
}

/*
 * gameObject:getScript(name) gives that script's self, starting it first when its update
 * has not run yet; nil when the object has no running script with that name.
 */
int32
getScript(lua_State* state)
{
  const GameObject& gameObject = ScriptTraits<GameObject>::check(state, 1);
  ScriptComponent* scriptComponent =
      findScript(gameObject, ScriptTraits<StringView>::check(state, 2));
  if (!scriptComponent || !scriptComponent->start()) {
    lua_pushnil(state);
    return 1;
  }
  lua_rawgeti(state, LUA_REGISTRYINDEX, scriptComponent->getInstance());
  return 1;
}

/*
 * Turning a script off runs its onDestroy; turning it on starts it again with a new self.
 */
int32
setScriptEnabled(lua_State* state)
{
  const GameObject& gameObject = ScriptTraits<GameObject>::check(state, 1);
  ScriptComponent* scriptComponent =
      findScript(gameObject, ScriptTraits<StringView>::check(state, 2));
  if (!scriptComponent) {
    return luaL_error(state, "GameObject '%s' has no script '%s'", gameObject.getName().c_str(),
                      lua_tostring(state, 2));
  }
  scriptComponent->setEnabled(lua_toboolean(state, 3) != 0);
  return 0;
}

int32
isScriptEnabled(lua_State* state)
{
  const GameObject& gameObject = ScriptTraits<GameObject>::check(state, 1);
  const ScriptComponent* scriptComponent =
      findScript(gameObject, ScriptTraits<StringView>::check(state, 2));
  lua_pushboolean(state, scriptComponent && scriptComponent->isEnabled() ? 1 : 0);
  return 1;
}

/*
 * Never raises, so printing a destroyed object is safe.
 */
int32
gameObjectToString(lua_State* state)
{
  const GameObjectHandle* handle = static_cast<const GameObjectHandle*>(
      luaL_checkudata(state, 1, ScriptHandleType<GameObject>::NAME));
  if (isAlive(handle->object)) {
    lua_pushfstring(state, "GameObject(%s)", handle->object->getName().c_str());
  }
  else {
    lua_pushliteral(state, "GameObject(destroyed)");
  }
  return 1;
}

int32
transformToString(lua_State* state)
{
  const TransformHandle* handle = static_cast<const TransformHandle*>(
      luaL_checkudata(state, 1, ScriptHandleType<Transform>::NAME));
  if (isAlive(*handle->object)) {
    lua_pushfstring(state, "Transform(%s)", (*handle->object)->getName().c_str());
  }
  else {
    lua_pushliteral(state, "Transform(destroyed)");
  }
  return 1;
}

void
setToString(lua_State* state, const ANSICHAR* typeName, const lua_CFunction function)
{
  luaL_getmetatable(state, typeName);
  lua_pushcfunction(state, function);
  lua_setfield(state, -2, "__tostring");
  lua_pop(state, 1);
}
} // namespace

/*
 */
void
ScriptTraits<GameObject>::push(lua_State* state, GameObject& gameObject)
{
  if (gameObject.m_scriptHandle != ScriptClassInfo::NO_REFERENCE) {
    lua_rawgeti(state, LUA_REGISTRYINDEX, gameObject.m_scriptHandle);
    return;
  }
  // One user value: the Transform handle, made on first use.
  GameObjectHandle* handle =
      static_cast<GameObjectHandle*>(lua_newuserdatauv(state, sizeof(GameObjectHandle), 1));
  handle->object = &gameObject;
  luaL_setmetatable(state, ScriptHandleType<GameObject>::NAME);
  lua_pushvalue(state, -1);
  gameObject.m_scriptHandle = luaL_ref(state, LUA_REGISTRYINDEX);
}

/*
 */
GameObject&
ScriptTraits<GameObject>::check(lua_State* state, const int32 index)
{
  const GameObjectHandle* handle = static_cast<const GameObjectHandle*>(
      luaL_checkudata(state, index, ScriptHandleType<GameObject>::NAME));
  return checkAlive(state, handle->object);
}

/*
 */
void
ScriptTraits<GameObject>::release(GameObject& gameObject) noexcept
{
  if (!ScriptManager::isStarted()) {
    return;
  }
  lua_State* const state = ScriptManager::instance().getState();
  lua_rawgeti(state, LUA_REGISTRYINDEX, gameObject.m_scriptHandle);
  if (GameObjectHandle* handle = static_cast<GameObjectHandle*>(lua_touserdata(state, -1))) {
    handle->object = nullptr;
  }
  lua_pop(state, 1);
  luaL_unref(state, LUA_REGISTRYINDEX, gameObject.m_scriptHandle);
  gameObject.m_scriptHandle = ScriptClassInfo::NO_REFERENCE;
}

/*
 */
void
ScriptTraits<Transform>::pushOf(lua_State* state, GameObject& gameObject)
{
  ScriptTraits<GameObject>::push(state, gameObject);
  if (lua_getiuservalue(state, -1, 1) == LUA_TUSERDATA) {
    lua_remove(state, -2);
    return;
  }
  lua_pop(state, 1);

  GameObjectHandle* const objectHandle = static_cast<GameObjectHandle*>(lua_touserdata(state, -1));
  TransformHandle* const handle =
      static_cast<TransformHandle*>(lua_newuserdatauv(state, sizeof(TransformHandle), 1));
  handle->object = &objectHandle->object;
  luaL_setmetatable(state, ScriptHandleType<Transform>::NAME);
  // The transform keeps the object handle alive, which caches the transform in turn.
  lua_pushvalue(state, -2);
  lua_setiuservalue(state, -2, 1);
  lua_pushvalue(state, -1);
  lua_setiuservalue(state, -3, 1);
  lua_remove(state, -2);
}

/*
 */
Transform&
ScriptTraits<Transform>::check(lua_State* state, const int32 index)
{
  return checkOwner(state, index).getTransform();
}

/*
 */
GameObject&
ScriptTraits<Transform>::checkOwner(lua_State* state, const int32 index)
{
  const TransformHandle* handle = static_cast<const TransformHandle*>(
      luaL_checkudata(state, index, ScriptHandleType<Transform>::NAME));
  return checkAlive(state, *handle->object);
}

/*
 */
void
ScriptBindings::registerObjects(lua_State* state)
{
  ScriptClass<GameObject>(state)
      .rawMethod("isValid", &isValid, "fun(): boolean")
      .method<&getName>("getName")
      .method<&setName>("setName", "name")
      .rawMethod("getTransform", &getTransform, "fun(): Transform")
      .rawMethod("getParent", &getParent, "fun(): GameObject?")
      .method<&getChildCount>("getChildCount")
      .rawMethod("getChild", &getChild, "fun(index: integer): GameObject?")
      .rawMethod("findChild", &findChild, "fun(name: string): GameObject?")
      .rawMethod("findDescendant", &findDescendant, "fun(name: string): GameObject?")
      .rawMethod("getScript", &getScript, "fun(name: string): Script?")
      .rawMethod("setScriptEnabled", &setScriptEnabled, "fun(name: string, enabled: boolean)")
      .rawMethod("isScriptEnabled", &isScriptEnabled, "fun(name: string): boolean");
  setToString(state, ScriptHandleType<GameObject>::NAME, &gameObjectToString);

  const ANSICHAR* const kGetVector = "fun(out?: Vector3): Vector3";
  const ANSICHAR* const kGetRotation = "fun(out?: Quaternion): Quaternion";
  const ANSICHAR* const kNumbers = "fun(x: number, y: number, z: number)";
  const ANSICHAR* const kEuler = "fun(pitch: number, yaw: number, roll: number)";
  ScriptClass<Transform>(state)
      .rawMethod("getGameObject", &getGameObjectOfTransform, "fun(): GameObject")
      .rawMethod("getLocalPosition", &getLocalPosition, kGetVector)
      .rawMethod("setLocalPosition", &setLocalPosition, "fun(position: Vector3)", kNumbers)
      .rawMethod("getWorldPosition", &getWorldPosition, kGetVector)
      .rawMethod("setWorldPosition", &setWorldPositionScript, "fun(position: Vector3)",
                 kNumbers)
      .rawMethod("getLocalRotation", &getLocalRotation, kGetRotation)
      .rawMethod("setLocalRotation", &setLocalRotation, "fun(rotation: Quaternion)")
      .rawMethod("getWorldRotation", &getWorldRotation, kGetRotation)
      .rawMethod("setWorldRotation", &setWorldRotationScript, "fun(rotation: Quaternion)")
      .rawMethod("getLocalScale", &getLocalScale, kGetVector)
      .rawMethod("setLocalScale", &setLocalScale, "fun(scale: Vector3)", kNumbers)
      .rawMethod("translateWorld", &translateWorld, "fun(offset: Vector3)", kNumbers)
      .rawMethod("translateLocal", &translateLocal, "fun(offset: Vector3)", kNumbers)
      .rawMethod("rotateLocal", &rotateLocal, kEuler)
      .rawMethod("rotateWorld", &rotateWorld, kEuler)
      .rawMethod("lookAt", &lookAt, "fun(point: Vector3)", kNumbers)
      .rawMethod("getForward", &getForward, kGetVector)
      .rawMethod("getRight", &getRight, kGetVector)
      .rawMethod("getUp", &getUp, kGetVector);
  setToString(state, ScriptHandleType<Transform>::NAME, &transformToString);

  ScriptTable(state, "Scene", "The scene of the running script.")
      .rawFunction("find", &findInScene, "fun(name: string): GameObject?");
}

} // namespace chEngineSDK
