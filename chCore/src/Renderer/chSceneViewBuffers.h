/************************************************************************/
/**
 * @file chSceneViewBuffers.h
 * @author AccelMR
 * @date 2026/10/13
 * @brief Camera and light data of a frame, as the lighting shaders read it.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chGraphicsTypes.h"
#include "chMatrix4.h"

namespace chEngineSDK {

struct RenderView;

/**
 * Exists so every lit render path hands the same camera, light and shadow data to its
 * shaders (chView.hlsli, chLights.hlsli, chShadows.hlsli): a constant buffer with the view
 * and storage buffers with the visible lights and the shadow maps, one of each per frame in
 * flight, because the CPU writes them while the GPU may still read the previous frame's.
 *
 * Needs a started IGraphicsAPI and must be destroyed before it shuts down.
 */
class CH_CORE_EXPORT SceneViewBuffers
{
 public:
  SceneViewBuffers();

  ~SceneViewBuffers();

  SceneViewBuffers(const SceneViewBuffers&) = delete;

  SceneViewBuffers&
  operator=(const SceneViewBuffers&) = delete;

  /**
   * Writes the buffers of this frame slot. Call it once per frame, while building the graph.
   *
   * @return the bindless index of the view constant buffer.
   */
  NODISCARD uint32
  update(const RenderView& view);

 private:
  // Must match LightData in chLights.hlsli.
  struct GpuLight
  {
    float position[3];
    uint32 type;
    float direction[3];
    float range;
    float color[3];
    float spotScale;
    float spotOffset;
    // Index into the shadow buffer, kNoShadow when the light has no shadow map.
    uint32 shadowIndex;
    float padding[2];
  };
  static_assert(sizeof(GpuLight) == 64);

  // Must match ShadowCascadeData in chShadows.hlsli.
  struct GpuShadowCascade
  {
    Matrix4 viewProjection;
    float atlasOffset[2];
    float normalOffset;
    float padding;
  };
  static_assert(sizeof(GpuShadowCascade) == 80);

  // Must match ShadowData in chShadows.hlsli, which reads the cascades after it on their own.
  struct GpuShadow
  {
    uint32 textureIndex;
    uint32 cascadeCount;
    uint32 filterRadius;
    float fadeStart;
    float texelSize[2];
    float atlasScale[2];
    float splitDepths[4];
    float blendStarts[4];
    float fadeEnd;
    // ShadowMapType.
    uint32 type;
    float lightFade;
    float padding0;
    // Point maps: where the light is, the cube of the array, how the depth of a face follows
    // the distance along its axis, and the normal offset per unit of distance. The position
    // starts on 16 bytes, where a raw load may expect a float3.
    float lightPosition[3];
    uint32 cubeIndex;
    float cubeDepthScale;
    float cubeDepthOffset;
    float cubeNormalOffset;
    float padding1;
    GpuShadowCascade cascades[4];
  };
  static_assert(sizeof(GpuShadow) == 432);

  Array<SPtr<IBuffer>, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_viewBuffers;
  Array<SPtr<IBuffer>, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_lightBuffers;
  // Lights each buffer of m_lightBuffers holds; a buffer is made again, bigger, when a
  // frame has more.
  Array<uint32, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_lightBufferCapacities{};
  Array<SPtr<IBuffer>, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_shadowBuffers;
  Array<uint32, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_shadowBufferCapacities{};
  // Kept between frames, so converting the lights and shadows does not allocate.
  Vector<GpuLight> m_gpuLights;
  Vector<GpuShadow> m_gpuShadows;
  // Compares against the shadow maps, with bilinear filtering of the results.
  SPtr<ISampler> m_shadowSampler;
};

} // namespace chEngineSDK
