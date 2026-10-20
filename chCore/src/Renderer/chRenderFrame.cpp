/************************************************************************/
/**
 * @file chRenderFrame.cpp
 * @author AccelMR
 * @date 2026/10/20
 * @brief The copy of a scene that the renderer draws one frame from.
 */
/************************************************************************/
#include "chRenderFrame.h"

#include "chRenderCulling.h"
#include "chScene.h"

namespace chEngineSDK {

/*
 */
void
RenderFrame::extract(const Scene& scene,
                     const Camera& camera,
                     const LinearColor& clearColor,
                     const DebugDraw* extraLines)
{
  m_camera = camera;
  m_clearColor = clearColor;

  // Free slots are left out, so the renderer walks only what draws.
  m_items.clear();
  for (const RenderItem& item : scene.getRenderItemSlots()) {
    if (item.mesh) {
      m_items.push_back(item);
    }
  }
  m_lights.clear();
  for (const LightItem& light : scene.getLightSlots()) {
    if (light.bInUse) {
      m_lights.push_back(light);
    }
  }

  const SkyLightItem* skyLight = scene.getActiveSkyLight();
  m_bHasSkyLight = skyLight != nullptr;
  if (skyLight) {
    m_skyLight = *skyLight;
  }

  m_sceneLines = scene.getDebugDraw();
  m_fixedSceneLines = scene.getFixedDebugDraw();
  if (extraLines) {
    m_extraLines = *extraLines;
  }
  else {
    m_extraLines.clear();
  }
}

/*
 */
void
RenderFrame::gatherItems(const Frustum& frustum, Vector<const RenderItem*>& outItems) const
{
  RenderCulling::gatherItems(m_items, frustum, outItems);
}

/*
 */
void
RenderFrame::gatherLights(const Frustum& frustum, Vector<const LightItem*>& outLights) const
{
  RenderCulling::gatherLights(m_lights, frustum, outLights);
}

} // namespace chEngineSDK
