/************************************************************************/
/**
 * @file chRenderCulling.cpp
 * @author AccelMR
 * @date 2026/10/20
 * @brief Picks the render items and lights a frustum can see.
 */
/************************************************************************/
#include "chRenderCulling.h"

#include "chFrustum.h"
#include "chLightItem.h"
#include "chRenderItem.h"
#include "chShapeOverlap.h"
#include "chSphere.h"

namespace chEngineSDK {

/*
 */
void
RenderCulling::gatherItems(Span<const RenderItem> items,
                           const Frustum& frustum,
                           Vector<const RenderItem*>& outItems)
{
  outItems.clear();
  for (const RenderItem& item : items) {
    if (item.mesh && ShapeOverlap::frustumBounds(frustum, item.worldBounds)) {
      outItems.push_back(&item);
    }
  }
}

/*
 */
void
RenderCulling::gatherLights(Span<const LightItem> lights,
                            const Frustum& frustum,
                            Vector<const LightItem*>& outLights)
{
  outLights.clear();
  for (const LightItem& light : lights) {
    if (!light.bInUse) {
      continue;
    }
    // A spot is tested with the sphere of its range too: a cone test would save little for
    // what it costs.
    if (light.type == LightType::Directional ||
        ShapeOverlap::frustumSphere(frustum, Sphere(light.position, light.range))) {
      outLights.push_back(&light);
    }
  }
}

} // namespace chEngineSDK
