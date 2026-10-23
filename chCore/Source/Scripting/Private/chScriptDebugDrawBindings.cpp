/************************************************************************/
/**
 * @file chScriptDebugDrawBindings.cpp
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  The lines scripts draw over the scene.
 */
/************************************************************************/
#include "chScriptBindings.h"

#include "chScriptBinder.h"

#if USING(CH_DEBUG_DRAW)
#include "chBox.h"
#include "chCapsule.h"
#include "chDebugDraw.h"
#include "chLinearColor.h"
#include "chMatrixHelpers.h"
#include "chOrientedBox.h"
#include "chRay.h"
#include "chScene.h"
#include "chScriptManager.h"
#include "chScriptMathTypes.h"
#include "chSphere.h"
#endif

namespace chEngineSDK {
namespace {
#if USING(CH_DEBUG_DRAW)
/*
 * The list of the running script's scene for the phase it runs in, so lines drawn in
 * fixedUpdate stay until the next step and lines drawn in update last one frame. Null
 * without a scene: there is nowhere to draw.
 */
DebugDraw*
getDebugDraw()
{
  const ScriptManager& scriptManager = ScriptManager::instance();
  Scene* const scene = scriptManager.getCurrentScene();
  if (!scene) {
    return nullptr;
  }
  return scriptManager.isInFixedStep() ? &scene->getFixedDebugDraw() : &scene->getDebugDraw();
}

LinearColor
readColor(lua_State* state, const int32 index)
{
  if (lua_isnoneornil(state, index)) {
    return LinearColor::WHITE;
  }
  return ScriptTraits<LinearColor>::check(state, index);
}

DebugDepth
readDepth(lua_State* state, const int32 index)
{
  return lua_toboolean(state, index) != 0 ? DebugDepth::OnTop : DebugDepth::Tested;
}

int32
drawLine(lua_State* state)
{
  const Vector3 start = ScriptTraits<Vector3>::check(state, 1);
  const Vector3 end = ScriptTraits<Vector3>::check(state, 2);
  const LinearColor color = readColor(state, 3);
  if (DebugDraw* debugDraw = getDebugDraw()) {
    debugDraw->line(start, end, color, readDepth(state, 4));
  }
  return 0;
}

/*
 * The direction is made unit first, so length is in world units whatever it was.
 */
int32
drawRay(lua_State* state)
{
  const Vector3 origin = ScriptTraits<Vector3>::check(state, 1);
  Vector3 direction = ScriptTraits<Vector3>::check(state, 2);
  const float length = ScriptTraits<float>::check(state, 3);
  const LinearColor color = readColor(state, 4);
  DebugDraw* debugDraw = getDebugDraw();
  if (debugDraw && direction.normalize()) {
    debugDraw->ray(Ray(origin, direction), length, color, readDepth(state, 5));
  }
  return 0;
}

int32
drawBox(lua_State* state)
{
  const Vector3 center = ScriptTraits<Vector3>::check(state, 1);
  const Vector3 extent = ScriptTraits<Vector3>::check(state, 2);
  const LinearColor color = readColor(state, 3);
  if (DebugDraw* debugDraw = getDebugDraw()) {
    debugDraw->box(AABox(center - extent, center + extent), color, readDepth(state, 4));
  }
  return 0;
}

int32
drawOrientedBox(lua_State* state)
{
  const Vector3 center = ScriptTraits<Vector3>::check(state, 1);
  const Vector3 extent = ScriptTraits<Vector3>::check(state, 2);
  const Quaternion rotation = ScriptTraits<Quaternion>::check(state, 3).getNormalized();
  const LinearColor color = readColor(state, 4);
  if (DebugDraw* debugDraw = getDebugDraw()) {
    debugDraw->orientedBox(OrientedBox(center, extent, rotation), color, readDepth(state, 5));
  }
  return 0;
}

int32
drawSphere(lua_State* state)
{
  const Vector3 center = ScriptTraits<Vector3>::check(state, 1);
  const float radius = ScriptTraits<float>::check(state, 2);
  const LinearColor color = readColor(state, 3);
  if (DebugDraw* debugDraw = getDebugDraw()) {
    debugDraw->sphere(Sphere(center, radius), color, readDepth(state, 4));
  }
  return 0;
}

int32
drawCircle(lua_State* state)
{
  const Vector3 center = ScriptTraits<Vector3>::check(state, 1);
  Vector3 normal = ScriptTraits<Vector3>::check(state, 2);
  const float radius = ScriptTraits<float>::check(state, 3);
  const LinearColor color = readColor(state, 4);
  DebugDraw* debugDraw = getDebugDraw();
  if (debugDraw && normal.normalize()) {
    debugDraw->circle(center, normal, radius, color, readDepth(state, 5));
  }
  return 0;
}

int32
drawCapsule(lua_State* state)
{
  const Vector3 start = ScriptTraits<Vector3>::check(state, 1);
  const Vector3 end = ScriptTraits<Vector3>::check(state, 2);
  const float radius = ScriptTraits<float>::check(state, 3);
  const LinearColor color = readColor(state, 4);
  if (DebugDraw* debugDraw = getDebugDraw()) {
    debugDraw->capsule(Capsule(start, end, radius), color, readDepth(state, 5));
  }
  return 0;
}

int32
drawAxes(lua_State* state)
{
  const Vector3 position = ScriptTraits<Vector3>::check(state, 1);
  const Quaternion rotation = ScriptTraits<Quaternion>::check(state, 2).getNormalized();
  const float size = ScriptTraits<float>::check(state, 3);
  if (DebugDraw* debugDraw = getDebugDraw()) {
    debugDraw->axes(ScaleRotationTranslationMatrix(Vector3::UNIT, rotation, position), size,
                    readDepth(state, 4));
  }
  return 0;
}
#else
/*
 * Builds without debug lines keep the table, so a script that draws runs the same; its
 * functions do not even read their arguments.
 */
int32
doNothing(lua_State*)
{
  return 0;
}

constexpr lua_CFunction drawLine = &doNothing;
constexpr lua_CFunction drawRay = &doNothing;
constexpr lua_CFunction drawBox = &doNothing;
constexpr lua_CFunction drawOrientedBox = &doNothing;
constexpr lua_CFunction drawSphere = &doNothing;
constexpr lua_CFunction drawCircle = &doNothing;
constexpr lua_CFunction drawCapsule = &doNothing;
constexpr lua_CFunction drawAxes = &doNothing;
#endif // USING(CH_DEBUG_DRAW)
} // namespace

/*
 */
void
ScriptBindings::registerDebugDraw(lua_State* state)
{
  ScriptTable(state, "DebugDraw",
              "Lines over the scene, in world space. Drawn in update they last one frame; in "
              "fixedUpdate, until the next step. onTop shows them through the scene. "
              "Packaged games draw nothing, so call them freely.")
      .rawFunction("line", drawLine,
                   "fun(start: Vector3, finish: Vector3, color?: Color, onTop?: boolean)")
      .rawFunction("ray", drawRay,
                   "fun(origin: Vector3, direction: Vector3, length: number, color?: Color, "
                   "onTop?: boolean)")
      .rawFunction("box", drawBox,
                   "fun(center: Vector3, extent: Vector3, color?: Color, onTop?: boolean)")
      .rawFunction("orientedBox", drawOrientedBox,
                   "fun(center: Vector3, extent: Vector3, rotation: Quaternion, color?: Color, "
                   "onTop?: boolean)")
      .rawFunction("sphere", drawSphere,
                   "fun(center: Vector3, radius: number, color?: Color, onTop?: boolean)")
      .rawFunction("circle", drawCircle,
                   "fun(center: Vector3, normal: Vector3, radius: number, color?: Color, "
                   "onTop?: boolean)")
      .rawFunction("capsule", drawCapsule,
                   "fun(start: Vector3, finish: Vector3, radius: number, color?: Color, "
                   "onTop?: boolean)")
      .rawFunction("axes", drawAxes,
                   "fun(position: Vector3, rotation: Quaternion, size: number, "
                   "onTop?: boolean)");
}

} // namespace chEngineSDK
