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

#include "chLinearColor.h"
#include "chMatrix4.h"
#include "chRenderGraph.h"

namespace chEngineSDK {

/**
 * A shadow map the ShadowRenderer drew this frame for one light. Lit paths read its texture
 * in their lighting pass; SceneViewBuffers sends the rest to the shaders.
 */
struct ShadowView
{
  // Takes a world position to the shadow map: xy in NDC, z the depth stored in it.
  Matrix4 viewProjection;
  const LightItem* light = nullptr;
  RGTextureHandle texture;
  uint32 textureIndex = 0;
  // Size of one texel in texture coordinates.
  float texelSize = 0.0f;
  // How far receivers move along their normal before the test, in world units.
  float normalOffset = 0.0f;
  // Distance from the camera where the shadow has faded out.
  float fadeDistance = 0.0f;
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
  LinearColor clearColor = LinearColor::Black;
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
