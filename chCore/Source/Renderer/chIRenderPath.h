/************************************************************************/
/**
 * @file chIRenderPath.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Interface for the ways of drawing a scene view into a render graph.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chImageBasedLighting.h"
#include "chLinearColor.h"
#include "chMatrix4.h"
#include "chRenderGraph.h"
#include "chVector2.h"

namespace chEngineSDK {

/**
 * One cascade of a directional shadow map: the part of the camera view, by view depth, that
 * one tile of the shadow atlas covers.
 */
struct ShadowCascade
{
  // Takes a world position to the cascade's tile: xy in NDC, z the depth stored in it.
  Matrix4 viewProjection;
  // Top left corner of the tile in the atlas, in texture coordinates.
  Vector2 atlasOffset;
  // View depth where the cascade ends.
  float splitDepth = 0.0f;
  // View depth where it starts to blend into the next cascade.
  float blendStart = 0.0f;
  // How far receivers move along their normal before the test, in world units; per unit of
  // distance from the light in a perspective map.
  float normalOffset = 0.0f;
};

/**
 * How a shadow map is laid out. The values are sent to the shaders (chShadows.hlsli).
 */
enum class ShadowMapType : uint32
{
  // Tiles of an atlas, one per cascade, picked by view depth.
  Cascaded = 0,
  // One tile of an atlas, seen through a perspective projection.
  Spot = 1,
  // Six faces of a cube map array, picked by the direction from the light.
  Point = 2
};

/**
 * A shadow map the ShadowRenderer drew this frame for one light. Lit paths read its texture
 * in their lighting pass; SceneViewBuffers sends the rest to the shaders. A directional
 * light has one cascade per tile of its atlas; a spot light has one cascade, its tile of
 * the spot atlas, seen through a perspective projection; a point light has no cascade, only
 * its cube in the point cube array.
 */
struct ShadowView
{
  static constexpr uint32 MAX_CASCADES = 4;

  ShadowMapType type = ShadowMapType::Cascaded;
  const LightItem* light = nullptr;
  RGTextureHandle texture;
  uint32 textureIndex = 0;
  uint32 cascadeCount = 0;
  Array<ShadowCascade, MAX_CASCADES> cascades;
  // Size of one tile in texture coordinates of the atlas.
  Vector2 atlasScale;
  // Size of one texel in texture coordinates of the atlas; for a point map, x is the size
  // of one texel of a face per unit of distance from the light.
  Vector2 texelSize;
  // Texels around each pixel the test blends.
  uint32 filterRadius = 0;
  // View depths where the shadow starts to fade and where it is gone.
  float fadeStart = 0.0f;
  float fadeEnd = 0.0f;
  // How much of the whole shadow has faded, from 0 (all of it shows) to 1 (none). Spot and
  // point shadows fade as their light gets far from the camera.
  float lightFade = 0.0f;
  // Point maps: which cube of the array, and the depth a face stores for a point at
  // distance d along its axis, depthScale + depthOffset / d.
  uint32 cubeIndex = 0;
  float cubeDepthScale = 0.0f;
  float cubeDepthOffset = 0.0f;
  // Point maps: how far receivers move along their normal, per unit of distance from the
  // light.
  float cubeNormalOffset = 0.0f;
};

/**
 * What one camera sees in one frame. The camera, the items, the lights and the shadows are
 * read when the graph runs, so they must stay alive until then.
 */
struct RenderView
{
  const Camera& camera;
  Span<const RenderItem* const> items;
  Span<const LightItem* const> lights;
  Span<const ShadowView> shadows{};
  // The sky light's image based lighting; without an environment the paths use the constant
  // ambient.
  ImageBasedLightingView ibl{};
  LinearColor clearColor = LinearColor::BLACK;
  // Tints the lit image by the shadow cascade each pixel reads.
  bool bShowShadowCascades = false;
};

/**
 * Exists so the way a scene is drawn (forward, deferred, a debug view) can be swapped at
 * runtime: each path only adds its passes to the frame's graph, and the graph works out
 * the textures and barriers between them.
 */
class CH_CORE_EXPORT IRenderPath
{
 public:
  virtual ~IRenderPath() = default;

  /**
   * Adds the passes that draw the view into output, which they overwrite completely.
   *
   * @return the depth of the scene, so later passes (overlays, the editor) can test against
   *         it; invalid when the path keeps no depth.
   */
  virtual RGTextureHandle
  addPasses(RenderGraph& graph, const RenderView& view, RGTextureHandle output) = 0;
};

} // namespace chEngineSDK
