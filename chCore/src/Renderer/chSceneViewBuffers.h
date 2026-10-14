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

namespace chEngineSDK {

struct RenderView;

/**
 * Exists so every lit render path hands the same camera and light data to its shaders
 * (chView.hlsli, chLights.hlsli): a constant buffer with the view and a storage buffer with
 * the visible lights, one of each per frame in flight, because the CPU writes them while the
 * GPU may still read the previous frame's.
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
    float padding[3];
  };
  static_assert(sizeof(GpuLight) == 64);

  Array<SPtr<IBuffer>, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_viewBuffers;
  Array<SPtr<IBuffer>, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_lightBuffers;
  // Lights each buffer of m_lightBuffers holds; a buffer is made again, bigger, when a
  // frame has more.
  Array<uint32, GraphicsLimits::MAX_FRAMES_IN_FLIGHT> m_lightBufferCapacities{};
  // Kept between frames, so converting the lights does not allocate.
  Vector<GpuLight> m_gpuLights;
};

} // namespace chEngineSDK
