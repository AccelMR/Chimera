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

#include "chAnimationClipAsset.h"
#include "chAnimationGraphAsset.h"
#include "chAnimatorComponent.h"
#include "chAssetManager.h"
#include "chGameObject.h"
#include "chMath.h"
#include "chMatrix4.h"
#include "chRay.h"
#include "chScene.h"
#include "chScriptApi.h"
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
 * The handle of what belongs to an object (its transform, its animator). Points at the
 * object pointer inside the GameObject handle, which its user value keeps alive, so both
 * handles die with the object at once.
 */
struct PartHandle
{
  GameObject* const* object;
};

// The GameObject handle's user values, each the handle of one part, made on first use.
constexpr int32 kTransformUserValue = 1;
constexpr int32 kAnimatorUserValue = 2;
constexpr int32 kUserValueCount = 2;

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
 * Writes value into the Vector3 the table holds under name, or puts a new one there, so a
 * hit table passed back every frame keeps its vectors.
 */
void
setVectorField(lua_State* state, const int32 table, const ANSICHAR* name, const Vector3& value)
{
  lua_getfield(state, table, name);
  if (Vector3* existing = static_cast<Vector3*>(
          luaL_testudata(state, -1, ScriptValueType<Vector3>::NAME))) {
    *existing = value;
    lua_pop(state, 1);
    return;
  }
  lua_pop(state, 1);
  ScriptTraits<Vector3>::push(state, value);
  lua_setfield(state, table, name);
}

/*
 * Scene.raycast(origin, direction, maxDistance?, ignore?, out?): nil on a miss, else a table
 * with gameObject, distance, point and normal; out is filled instead of a new table. Tests
 * the triangles on the CPU, so a few rays per frame are fine, not hundreds.
 */
int32
raycastScene(lua_State* state)
{
  const Vector3 origin = ScriptTraits<Vector3>::check(state, 1);
  Vector3 direction = ScriptTraits<Vector3>::check(state, 2);
  const float maxDistance =
      static_cast<float>(luaL_optnumber(state, 3, static_cast<lua_Number>(Math::MAX_FLOAT)));

  // A destroyed object has no items left in the scene, so it simply ignores nothing.
  const GameObject* ignore = nullptr;
  if (!lua_isnoneornil(state, 4)) {
    luaL_checkudata(state, 4, ScriptHandleType<GameObject>::NAME);
    ignore = ScriptTraits<GameObject>::test(state, 4);
  }
  const bool bHasOut = !lua_isnoneornil(state, 5);
  if (bHasOut) {
    luaL_checktype(state, 5, LUA_TTABLE);
  }

  if (!direction.normalize()) {
    return luaL_error(state, "Scene.raycast needs a direction that is not zero");
  }
  const Scene* scene = ScriptManager::instance().getCurrentScene();
  if (!scene) {
    return luaL_error(state, "Scene.raycast needs a running script's scene");
  }

  SceneRayHit hit;
  if (!scene->raycast(Ray(origin, direction), hit, maxDistance, ignore) || !hit.gameObject) {
    lua_pushnil(state);
    return 1;
  }

  if (bHasOut) {
    lua_pushvalue(state, 5);
  }
  else {
    lua_createtable(state, 0, 4);
  }
  const int32 table = lua_gettop(state);
  ScriptTraits<GameObject>::push(state, *hit.gameObject);
  lua_setfield(state, table, "gameObject");
  lua_pushnumber(state, static_cast<lua_Number>(hit.distance));
  lua_setfield(state, table, "distance");
  setVectorField(state, table, "point", hit.point);
  setVectorField(state, table, "normal", hit.normal);
  return 1;
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
  const PartHandle* handle = static_cast<const PartHandle*>(
      luaL_checkudata(state, 1, ScriptHandleType<Transform>::NAME));
  if (isAlive(*handle->object)) {
    lua_pushfstring(state, "Transform(%s)", (*handle->object)->getName().c_str());
  }
  else {
    lua_pushliteral(state, "Transform(destroyed)");
  }
  return 1;
}

/*
 * Pushes the part handle the object handle keeps in userValue, making it on first use.
 */
void
pushPartHandle(lua_State* state,
               GameObject& gameObject,
               const int32 userValue,
               const ANSICHAR* typeName)
{
  ScriptTraits<GameObject>::push(state, gameObject);
  if (lua_getiuservalue(state, -1, userValue) == LUA_TUSERDATA) {
    lua_remove(state, -2);
    return;
  }
  lua_pop(state, 1);

  GameObjectHandle* const objectHandle =
      static_cast<GameObjectHandle*>(lua_touserdata(state, -1));
  PartHandle* const handle =
      static_cast<PartHandle*>(lua_newuserdatauv(state, sizeof(PartHandle), 1));
  handle->object = &objectHandle->object;
  luaL_setmetatable(state, typeName);
  // The part keeps the object handle alive, which caches the part in turn.
  lua_pushvalue(state, -2);
  lua_setiuservalue(state, -2, 1);
  lua_pushvalue(state, -1);
  lua_setiuservalue(state, -3, userValue);
  lua_remove(state, -2);
}

/*
 * Nil when the object has no animator.
 */
int32
getAnimator(lua_State* state)
{
  GameObject& gameObject = ScriptTraits<GameObject>::check(state, 1);
  if (!gameObject.getComponent<AnimatorComponent>()) {
    lua_pushnil(state);
    return 1;
  }
  ScriptTraits<AnimatorComponent>::pushOf(state, gameObject);
  return 1;
}

/*
 * animator:play(clip, fadeTime?): clip is a path or a name (see AnimatorComponent::findClip),
 * nil fades back to the rest pose. The clip a text names is remembered per animator, so
 * calling it every frame costs a short search, not a load.
 */
int32
playAnimation(lua_State* state)
{
  AnimatorComponent& animator = ScriptTraits<AnimatorComponent>::check(state, 1);
  const float fadeTime = static_cast<float>(luaL_optnumber(state, 3, 0.0));
  if (lua_isnoneornil(state, 2)) {
    animator.play(nullptr, fadeTime);
    return 0;
  }
  const StringView text = ScriptTraits<StringView>::check(state, 2);
  String error;
  SPtr<AnimationClipAsset> clip = animator.findClip(text, error);
  if (!clip) {
    return luaL_error(state, "Animator.play: %s", error.c_str());
  }
  animator.play(clip, fadeTime);
  return 0;
}

int32
stopAnimation(lua_State* state)
{
  AnimatorComponent& animator = ScriptTraits<AnimatorComponent>::check(state, 1);
  animator.stop(static_cast<float>(luaL_optnumber(state, 2, 0.0)));
  return 0;
}

void
pauseAnimation(AnimatorComponent& animator)
{
  animator.setPlaying(false);
}

void
resumeAnimation(AnimatorComponent& animator)
{
  animator.setPlaying(true);
}

bool
isAnimationPlaying(const AnimatorComponent& animator)
{
  return animator.isPlaying();
}

void
setAnimationLooping(AnimatorComponent& animator, const bool bLooping)
{
  animator.setLooping(bLooping);
}

bool
isAnimationLooping(const AnimatorComponent& animator)
{
  return animator.isLooping();
}

void
setAnimationSpeed(AnimatorComponent& animator, const float speed)
{
  animator.setSpeed(speed);
}

float
getAnimationSpeed(const AnimatorComponent& animator)
{
  return animator.getSpeed();
}

void
setAnimationTime(AnimatorComponent& animator, const float time)
{
  animator.setTime(time);
}

float
getAnimationTime(const AnimatorComponent& animator)
{
  return animator.getTime();
}

float
getAnimationDuration(const AnimatorComponent& animator)
{
  return animator.getDuration();
}

/*
 * The name of the clip's asset; nil without a clip.
 */
int32
getClipName(lua_State* state)
{
  const AnimatorComponent& animator = ScriptTraits<AnimatorComponent>::check(state, 1);
  const SPtr<AnimationClipAsset>& clip = animator.getClipAsset();
  if (clip) {
    lua_pushstring(state, clip->getName());
  }
  else {
    lua_pushnil(state);
  }
  return 1;
}

/*
 * animator:setGraph(graph?): a path or the name of a graph asset; nil goes back to playing
 * clips. Loads the graph, so it is meant for onStart, not every frame.
 */
int32
setAnimationGraph(lua_State* state)
{
  AnimatorComponent& animator = ScriptTraits<AnimatorComponent>::check(state, 1);
  if (lua_isnoneornil(state, 2)) {
    animator.setGraph(nullptr);
    return 0;
  }
  const StringView text = ScriptTraits<StringView>::check(state, 2);
  if (!AssetManager::isStarted()) {
    return luaL_error(state, "Animator.setGraph: there are no assets to search");
  }
  AssetManager& assetManager = AssetManager::instance();
  const SPtr<IAsset> asset =
      assetManager.findAsset(AssetTypeTraits<AnimationGraphAsset>::getTypeId(), text);
  if (!asset || !assetManager.syncLoadAsset(asset) ||
      !asset->as<AnimationGraphAsset>()->getGraph()) {
    return luaL_error(state, "Animator.setGraph: no animation graph %s could be loaded",
                      String(text).c_str());
  }
  animator.setGraph(asset->as<AnimationGraphAsset>());
  return 0;
}

/*
 * The name of the graph's asset; nil without a graph.
 */
int32
getGraphName(lua_State* state)
{
  const AnimatorComponent& animator = ScriptTraits<AnimatorComponent>::check(state, 1);
  const SPtr<AnimationGraphAsset>& graph = animator.getGraphAsset();
  if (graph) {
    lua_pushstring(state, graph->getName());
  }
  else {
    lua_pushnil(state);
  }
  return 1;
}

bool
hasAnimationGraph(const AnimatorComponent& animator)
{
  return animator.hasGraph();
}

bool
isClipOverGraph(const AnimatorComponent& animator)
{
  return animator.isClipOverGraph();
}

/*
 * The animator's graph for this object; an error without one, or without a model.
 */
AnimationGraphInstance&
checkGraph(lua_State* state, AnimatorComponent& animator)
{
  AnimationGraphInstance& instance = animator.getGraphInstance();
  if (!animator.hasGraph() || !instance.isBound()) {
    luaL_error(state, "the Animator of '%s' has no animation graph running",
               animator.getOwner()->getName().c_str());
  }
  return instance;
}

/*
 * The id findParameter gave, or the parameter's name (a search, for scripts that do not
 * keep ids); its type must be one of the two given.
 */
uint32
checkParameter(lua_State* state,
               const AnimationGraphInstance& instance,
               const int32 index,
               const AnimationParameterType type,
               const AnimationParameterType alsoAccepted)
{
  const AnimationGraph& graph = *instance.getGraph();
  const Vector<AnimationParameter>& parameters = graph.getParameters();
  uint32 parameter = AnimationGraph::NOT_FOUND;
  if (lua_type(state, index) == LUA_TNUMBER) {
    const lua_Integer id = luaL_checkinteger(state, index);
    luaL_argcheck(state, id >= 0 && id < static_cast<lua_Integer>(parameters.size()), index,
                  "unknown animation parameter id");
    parameter = static_cast<uint32>(id);
  }
  else {
    const ANSICHAR* const name = luaL_checkstring(state, index);
    parameter = graph.findParameter(name);
    if (parameter == AnimationGraph::NOT_FOUND) {
      luaL_error(state, "no animation parameter named '%s'", name);
    }
  }
  const AnimationParameter& found = parameters[parameter];
  if (found.type != type && found.type != alsoAccepted) {
    luaL_error(state, "animation parameter '%s' is a %s, not a %s", found.name.c_str(),
               AnimationGraph::getParameterTypeName(found.type),
               AnimationGraph::getParameterTypeName(type));
  }
  return parameter;
}

/*
 * A layer by name or by its place from 1 (the base layer); missing means the base layer.
 */
uint32
checkLayer(lua_State* state, const AnimationGraphInstance& instance, const int32 index)
{
  if (lua_isnoneornil(state, index)) {
    return 0;
  }
  const AnimationGraph& graph = *instance.getGraph();
  if (lua_type(state, index) == LUA_TNUMBER) {
    const lua_Integer layer = luaL_checkinteger(state, index);
    luaL_argcheck(state,
                  layer >= 1 && layer <= static_cast<lua_Integer>(graph.getLayers().size()),
                  index, "no such animation layer");
    return static_cast<uint32>(layer - 1);
  }
  const ANSICHAR* const name = luaL_checkstring(state, index);
  const uint32 layer = graph.findLayer(name);
  if (layer == AnimationGraph::NOT_FOUND) {
    luaL_error(state, "no animation layer named '%s'", name);
  }
  return layer;
}

/*
 * Nil when the animator has no graph or the graph has no such parameter.
 */
int32
findAnimationParameter(lua_State* state)
{
  AnimatorComponent& animator = ScriptTraits<AnimatorComponent>::check(state, 1);
  const StringView name = ScriptTraits<StringView>::check(state, 2);
  const uint32 parameter =
      animator.hasGraph() ? animator.getGraphInstance().getGraph()->findParameter(name)
                          : AnimationGraph::NOT_FOUND;
  if (parameter == AnimationGraph::NOT_FOUND) {
    lua_pushnil(state);
  }
  else {
    lua_pushinteger(state, static_cast<lua_Integer>(parameter));
  }
  return 1;
}

int32
setAnimationFloat(lua_State* state)
{
  AnimationGraphInstance& instance =
      checkGraph(state, ScriptTraits<AnimatorComponent>::check(state, 1));
  const uint32 parameter = checkParameter(state, instance, 2, AnimationParameterType::Float,
                                          AnimationParameterType::Float);
  instance.setFloat(parameter, static_cast<float>(luaL_checknumber(state, 3)));
  return 0;
}

int32
setAnimationInt(lua_State* state)
{
  AnimationGraphInstance& instance =
      checkGraph(state, ScriptTraits<AnimatorComponent>::check(state, 1));
  const uint32 parameter = checkParameter(state, instance, 2, AnimationParameterType::Int,
                                          AnimationParameterType::Int);
  instance.setInt(parameter, ScriptTraits<int32>::check(state, 3));
  return 0;
}

int32
setAnimationBool(lua_State* state)
{
  AnimationGraphInstance& instance =
      checkGraph(state, ScriptTraits<AnimatorComponent>::check(state, 1));
  const uint32 parameter = checkParameter(state, instance, 2, AnimationParameterType::Bool,
                                          AnimationParameterType::Bool);
  instance.setBool(parameter, lua_toboolean(state, 3) != 0);
  return 0;
}

int32
setAnimationTrigger(lua_State* state)
{
  AnimationGraphInstance& instance =
      checkGraph(state, ScriptTraits<AnimatorComponent>::check(state, 1));
  instance.setTrigger(checkParameter(state, instance, 2, AnimationParameterType::Trigger,
                                     AnimationParameterType::Trigger));
  return 0;
}

int32
resetAnimationTrigger(lua_State* state)
{
  AnimationGraphInstance& instance =
      checkGraph(state, ScriptTraits<AnimatorComponent>::check(state, 1));
  instance.resetTrigger(checkParameter(state, instance, 2, AnimationParameterType::Trigger,
                                       AnimationParameterType::Trigger));
  return 0;
}

int32
getAnimationFloat(lua_State* state)
{
  AnimationGraphInstance& instance =
      checkGraph(state, ScriptTraits<AnimatorComponent>::check(state, 1));
  const uint32 parameter = checkParameter(state, instance, 2, AnimationParameterType::Float,
                                          AnimationParameterType::Int);
  lua_pushnumber(state, static_cast<lua_Number>(instance.getFloat(parameter)));
  return 1;
}

int32
getAnimationInt(lua_State* state)
{
  AnimationGraphInstance& instance =
      checkGraph(state, ScriptTraits<AnimatorComponent>::check(state, 1));
  const uint32 parameter = checkParameter(state, instance, 2, AnimationParameterType::Int,
                                          AnimationParameterType::Int);
  lua_pushinteger(state, static_cast<lua_Integer>(instance.getInt(parameter)));
  return 1;
}

int32
getAnimationBool(lua_State* state)
{
  AnimationGraphInstance& instance =
      checkGraph(state, ScriptTraits<AnimatorComponent>::check(state, 1));
  const uint32 parameter = checkParameter(state, instance, 2, AnimationParameterType::Bool,
                                          AnimationParameterType::Trigger);
  lua_pushboolean(state, instance.getBool(parameter));
  return 1;
}

/*
 * The name of the layer's current state (during a fade, the one it goes to).
 */
int32
getAnimationState(lua_State* state)
{
  AnimationGraphInstance& instance =
      checkGraph(state, ScriptTraits<AnimatorComponent>::check(state, 1));
  const uint32 layer = checkLayer(state, instance, 2);
  const AnimationState& current =
      instance.getGraph()->getLayers()[layer].states[instance.getCurrentState(layer)];
  ScriptTraits<String>::push(state, current.name);
  return 1;
}

int32
getAnimationStateTime(lua_State* state)
{
  AnimationGraphInstance& instance =
      checkGraph(state, ScriptTraits<AnimatorComponent>::check(state, 1));
  lua_pushnumber(state,
                 static_cast<lua_Number>(instance.getStateTime(checkLayer(state, instance, 2))));
  return 1;
}

int32
isAnimationInTransition(lua_State* state)
{
  AnimationGraphInstance& instance =
      checkGraph(state, ScriptTraits<AnimatorComponent>::check(state, 1));
  lua_pushboolean(state, instance.isInTransition(checkLayer(state, instance, 2)));
  return 1;
}

int32
setAnimationLayerWeight(lua_State* state)
{
  AnimationGraphInstance& instance =
      checkGraph(state, ScriptTraits<AnimatorComponent>::check(state, 1));
  luaL_checkany(state, 2);
  instance.setLayerWeight(checkLayer(state, instance, 2),
                          static_cast<float>(luaL_checknumber(state, 3)));
  return 0;
}

int32
getAnimationLayerWeight(lua_State* state)
{
  AnimationGraphInstance& instance =
      checkGraph(state, ScriptTraits<AnimatorComponent>::check(state, 1));
  lua_pushnumber(
      state, static_cast<lua_Number>(instance.getLayerWeight(checkLayer(state, instance, 2))));
  return 1;
}

int32
getGameObjectOfAnimator(lua_State* state)
{
  ScriptTraits<GameObject>::push(state, ScriptTraits<AnimatorComponent>::checkOwner(state, 1));
  return 1;
}

int32
animatorToString(lua_State* state)
{
  const PartHandle* handle = static_cast<const PartHandle*>(
      luaL_checkudata(state, 1, ScriptHandleType<AnimatorComponent>::NAME));
  if (isAlive(*handle->object)) {
    lua_pushfstring(state, "Animator(%s)", (*handle->object)->getName().c_str());
  }
  else {
    lua_pushliteral(state, "Animator(destroyed)");
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
  GameObjectHandle* handle = static_cast<GameObjectHandle*>(
      lua_newuserdatauv(state, sizeof(GameObjectHandle), kUserValueCount));
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
GameObject*
ScriptTraits<GameObject>::test(lua_State* state, const int32 index)
{
  const GameObjectHandle* handle = static_cast<const GameObjectHandle*>(
      luaL_testudata(state, index, ScriptHandleType<GameObject>::NAME));
  return handle && isAlive(handle->object) ? handle->object : nullptr;
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
  pushPartHandle(state, gameObject, kTransformUserValue, ScriptHandleType<Transform>::NAME);
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
  const PartHandle* handle = static_cast<const PartHandle*>(
      luaL_checkudata(state, index, ScriptHandleType<Transform>::NAME));
  return checkAlive(state, *handle->object);
}

/*
 */
void
ScriptTraits<AnimatorComponent>::pushOf(lua_State* state, GameObject& gameObject)
{
  pushPartHandle(state, gameObject, kAnimatorUserValue,
                 ScriptHandleType<AnimatorComponent>::NAME);
}

/*
 * The component is looked up each time (a short walk over the object's components), so a
 * removed animator is an error instead of a dangling pointer.
 */
AnimatorComponent&
ScriptTraits<AnimatorComponent>::check(lua_State* state, const int32 index)
{
  GameObject& gameObject = checkOwner(state, index);
  AnimatorComponent* animator = gameObject.getComponent<AnimatorComponent>();
  if (!animator) {
    luaL_error(state, "GameObject '%s' has no Animator", gameObject.getName().c_str());
  }
  return *animator;
}

/*
 */
GameObject&
ScriptTraits<AnimatorComponent>::checkOwner(lua_State* state, const int32 index)
{
  const PartHandle* handle = static_cast<const PartHandle*>(
      luaL_checkudata(state, index, ScriptHandleType<AnimatorComponent>::NAME));
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
      .rawMethod("getAnimator", &getAnimator, "fun(): Animator?")
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

  ScriptClass<AnimatorComponent>(state)
      .rawMethod("getGameObject", &getGameObjectOfAnimator, "fun(): GameObject")
      .rawMethod("play", &playAnimation, "fun(clip?: string, fadeTime?: number)")
      .rawMethod("stop", &stopAnimation, "fun(fadeTime?: number)")
      .method<&pauseAnimation>("pause")
      .method<&resumeAnimation>("resume")
      .method<&isAnimationPlaying>("isPlaying")
      .method<&setAnimationLooping>("setLooping", "looping")
      .method<&isAnimationLooping>("isLooping")
      .method<&setAnimationSpeed>("setSpeed", "speed")
      .method<&getAnimationSpeed>("getSpeed")
      .method<&setAnimationTime>("setTime", "time")
      .method<&getAnimationTime>("getTime")
      .method<&getAnimationDuration>("getDuration")
      .rawMethod("getClipName", &getClipName, "fun(): string?")
      .rawMethod("setGraph", &setAnimationGraph, "fun(graph?: string)")
      .rawMethod("getGraphName", &getGraphName, "fun(): string?")
      .method<&hasAnimationGraph>("hasGraph")
      .method<&isClipOverGraph>("isClipOverGraph")
      .rawMethod("findParameter", &findAnimationParameter,
                 "fun(name: string): AnimatorParameter?")
      .rawMethod("setFloat", &setAnimationFloat,
                 "fun(parameter: AnimatorParameter, value: number)")
      .rawMethod("setInt", &setAnimationInt,
                 "fun(parameter: AnimatorParameter, value: integer)")
      .rawMethod("setBool", &setAnimationBool,
                 "fun(parameter: AnimatorParameter, value: boolean)")
      .rawMethod("setTrigger", &setAnimationTrigger, "fun(parameter: AnimatorParameter)")
      .rawMethod("resetTrigger", &resetAnimationTrigger, "fun(parameter: AnimatorParameter)")
      .rawMethod("getFloat", &getAnimationFloat, "fun(parameter: AnimatorParameter): number")
      .rawMethod("getInt", &getAnimationInt, "fun(parameter: AnimatorParameter): integer")
      .rawMethod("getBool", &getAnimationBool, "fun(parameter: AnimatorParameter): boolean")
      .rawMethod("getState", &getAnimationState, "fun(layer?: AnimatorLayer): string")
      .rawMethod("getStateTime", &getAnimationStateTime, "fun(layer?: AnimatorLayer): number")
      .rawMethod("isInTransition", &isAnimationInTransition,
                 "fun(layer?: AnimatorLayer): boolean")
      .rawMethod("setLayerWeight", &setAnimationLayerWeight,
                 "fun(layer: AnimatorLayer, weight: number)")
      .rawMethod("getLayerWeight", &getAnimationLayerWeight,
                 "fun(layer?: AnimatorLayer): number");
  setToString(state, ScriptHandleType<AnimatorComponent>::NAME, &animatorToString);

  if (ScriptApiDescription* const api = ScriptApiDescription::find(state)) {
    // Parameters: the id findParameter gives, or the name. Layers: the name, or the place
    // from 1 (the base layer).
    api->addAlias("AnimatorParameter", "integer|string");
    api->addAlias("AnimatorLayer", "integer|string");
    ScriptApiType& hit = api->addType("RaycastHit", ScriptApiTypeKind::Shape);
    hit.description = "What Scene.raycast hit. Pass one back as out to reuse it.";
    hit.fields = {{"gameObject", "GameObject"},
                  {"distance", "number"},
                  {"point", "Vector3"},
                  {"normal", "Vector3"}};
  }

  ScriptTable(state, "Scene", "The scene of the running script.")
      .rawFunction("find", &findInScene, "fun(name: string): GameObject?")
      .rawFunction("raycast", &raycastScene,
                   "fun(origin: Vector3, direction: Vector3, maxDistance?: number, "
                   "ignore?: GameObject, out?: RaycastHit): RaycastHit?");
}

} // namespace chEngineSDK
