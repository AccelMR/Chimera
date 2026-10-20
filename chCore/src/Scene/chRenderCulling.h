/************************************************************************/
/**
 * @file chRenderCulling.h
 * @author AccelMR
 * @date 2026/10/20
 * @brief Picks the render items and lights a frustum can see.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class Frustum;
struct LightItem;
struct RenderItem;

/**
 * Exists so a Scene and the RenderFrame copied from it pick what a frustum sees the same
 * way. Free slots (no mesh, light not in use) are skipped, so a Scene can pass its slots as
 * they are.
 */
class CH_CORE_EXPORT RenderCulling
{
 public:
  /**
   * Replaces the contents of outItems with the items whose bounds touch the frustum.
   * Reusing outItems every frame keeps it from allocating.
   */
  static void
  gatherItems(Span<const RenderItem> items,
              const Frustum& frustum,
              Vector<const RenderItem*>& outItems);

  /**
   * Replaces the contents of outLights with every directional light and the point and spot
   * lights whose range touches the frustum.
   */
  static void
  gatherLights(Span<const LightItem> lights,
               const Frustum& frustum,
               Vector<const LightItem*>& outLights);
};

} // namespace chEngineSDK
