/************************************************************************/
/**
 * @file chRenderSettings.h
 * @author AccelMR
 * @date 2026/10/07
 * @brief Console variables of the scene renderer, section [Renderer] of Engine.ini.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chConsoleVariable.h"

namespace chEngineSDK {

/**
 * What the scene renderer shows. Lit uses the active render path; the others are views for
 * looking at the scene data. The GBuffer modes (BaseColor to Emissive) show what the
 * deferred path stores, whichever path is active. ShadowCascades is Lit tinted by the
 * shadow cascade each pixel reads.
 */
enum class ViewMode : uint32
{
  Lit,
  LitWireframe,
  Wireframe,
  Unlit,
  Normals,
  Depth,
  TexCoords,
  BaseColor,
  Metallic,
  Roughness,
  AmbientOcclusion,
  Emissive,
  ShadowCascades,
  COUNT
};

/**
 * How the Lit view modes light the scene.
 */
enum class RenderPathType : uint32
{
  Forward,
  Deferred,
  COUNT
};

/**
 * Exists so the view mode and the render path can be written as names in .ini files, on
 * the command line and in menus. Names are null terminated literals, so they can be handed
 * to C APIs as they are; lookups ignore case, and an unknown name gives nothing.
 */
class CH_CORE_EXPORT RenderSettingsUtils
{
 public:
  NODISCARD static StringView
  getName(ViewMode mode);

  NODISCARD static StringView
  getName(RenderPathType path);

  NODISCARD static Optional<ViewMode>
  viewModeFromName(StringView name);

  NODISCARD static Optional<RenderPathType>
  renderPathFromName(StringView name);
};

// Name of a ViewMode (-ViewMode=Wireframe).
extern CH_CORE_EXPORT ConsoleVariable<String> g_cvarViewMode;

// Name of a RenderPathType (-RenderPath=Forward).
extern CH_CORE_EXPORT ConsoleVariable<String> g_cvarRenderPath;

// Light that reaches every surface from all sides until image based lighting exists.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarAmbient;

// Directional lights with Cast Shadows draw shadows.
extern CH_CORE_EXPORT ConsoleVariable<bool> g_cvarShadows;

// Width and height in texels of each cascade of the directional shadow map.
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarShadowMapSize;

// How far from the camera, in world units, the directional shadow reaches.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarShadowDistance;

// How many cascades (1 to 4) split the shadow distance.
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarShadowCascades;

// 0 splits the cascades in even steps, 1 in steps that grow with the distance.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarShadowSplitLambda;

// Fraction of each cascade that blends into the next one.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarShadowCascadeBlend;

// Fraction of the shadow distance where the shadow fades out.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarShadowFade;

// Texels around each pixel the shadow test blends.
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarShadowFilterRadius;

// Texels a receiver moves along its normal before the shadow test.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarShadowNormalOffset;

// Depth bias of the shadow casters, constant and by slope.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarShadowDepthBias;
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarShadowSlopeBias;

} // namespace chEngineSDK
