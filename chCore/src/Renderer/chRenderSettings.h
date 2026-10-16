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
 * shadow cascade each pixel reads; ShadowMaps shows the shadow atlases of the frame.
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
  ScreenSpaceAO,
  Emissive,
  ShadowCascades,
  ShadowMaps,
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

// Lights with Cast Shadows draw shadows.
extern CH_CORE_EXPORT ConsoleVariable<bool> g_cvarShadows;

// Parts of the lighting that can be turned off to see what each one adds or costs.
extern CH_CORE_EXPORT ConsoleVariable<bool> g_cvarNormalMaps;
extern CH_CORE_EXPORT ConsoleVariable<bool> g_cvarOcclusionMaps;
extern CH_CORE_EXPORT ConsoleVariable<bool> g_cvarEmissive;
extern CH_CORE_EXPORT ConsoleVariable<bool> g_cvarDirectionalLights;
extern CH_CORE_EXPORT ConsoleVariable<bool> g_cvarPointLights;
extern CH_CORE_EXPORT ConsoleVariable<bool> g_cvarSpotLights;

// The ACES curve of the tonemap; off, the exposed color is only clamped.
extern CH_CORE_EXPORT ConsoleVariable<bool> g_cvarTonemap;

// Comma separated names of optional render passes that do not run
// (-Renderer.DisabledPasses=ShadowDepth,GBuffer).
extern CH_CORE_EXPORT ConsoleVariable<String> g_cvarDisabledPasses;

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

// How many spot lights (0 to 16) draw shadows in one frame; the ones that cover the most of
// the screen win.
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarSpotShadowMaxLights;

// Width and height in texels of the shadow map of each spot light.
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarSpotShadowMapSize;

// How far from the camera, in world units, a spot light still draws its shadow.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarSpotShadowDistance;

// Fraction of the spot shadow distance where the shadows fade out.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarSpotShadowFade;

// Distance from a spot light to the near plane of its shadow map.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarSpotShadowNearPlane;

// Texels around each pixel the spot shadow test blends.
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarSpotShadowFilterRadius;

// Texels a receiver moves along its normal before the spot shadow test.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarSpotShadowNormalOffset;

// Depth bias of the spot shadow casters, constant and by slope.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarSpotShadowDepthBias;
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarSpotShadowSlopeBias;

// How many point lights (0 to 8) draw shadows in one frame; the ones that cover the most of
// the screen win.
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarPointShadowMaxLights;

// Width and height in texels of each face of the cube shadow map of a point light.
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarPointShadowMapSize;

// How far from the camera, in world units, a point light still draws its shadow.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarPointShadowDistance;

// Fraction of the point shadow distance where the shadows fade out.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarPointShadowFade;

// Distance from a point light to the near plane of its cube faces.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarPointShadowNearPlane;

// Texels around each pixel the point shadow test blends.
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarPointShadowFilterRadius;

// Texels a receiver moves along its normal before the point shadow test.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarPointShadowNormalOffset;

// Depth bias of the point shadow casters, constant and by slope.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarPointShadowDepthBias;
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarPointShadowSlopeBias;

// The sky light lights the scene through image based lighting; off, or without a sky light,
// the constant Renderer.Ambient is used.
extern CH_CORE_EXPORT ConsoleVariable<bool> g_cvarImageBasedLighting;

// Width and height of each face of the cube the sky image is turned into.
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarIBLCubeSize;

// Size of each face of the irradiance cube (diffuse light), and its samples per texel.
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarIBLIrradianceSize;
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarIBLIrradianceSamples;

// Size of each face of the prefiltered cube (reflections), how many mips spread the
// roughness from 0 to 1, and its samples per texel.
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarIBLPrefilterSize;
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarIBLPrefilterMips;
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarIBLPrefilterSamples;

// Size of the split sum lookup table and its samples per texel.
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarIBLBrdfLutSize;
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarIBLBrdfLutSamples;

// The deferred path darkens the ambient light where nearby surfaces hide the sky, measured
// on the depth of the frame (ground truth ambient occlusion).
extern CH_CORE_EXPORT ConsoleVariable<bool> g_cvarSSAO;

// Computes the occlusion at half the width and height, then scales it up.
extern CH_CORE_EXPORT ConsoleVariable<bool> g_cvarSSAOHalfResolution;

// How far around each pixel, in world units, surfaces occlude it.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarSSAORadius;

// Fraction of the radius over which far occluders fade out.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarSSAOFalloff;

// Directions around each pixel and depth samples along each side of a direction.
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarSSAOSlices;
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarSSAOSteps;

// Power applied to the visibility; above 1 darkens the occlusion.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarSSAOPower;

// Largest radius on screen, in pixels, so a surface close to the camera does not read the
// whole screen.
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarSSAOMaxPixelRadius;

// Texels on each side the blur averages (0 turns it off), and how much depth, as a fraction
// of the pixel's distance, it still blends across.
extern CH_CORE_EXPORT ConsoleVariable<int32> g_cvarSSAOBlurRadius;
extern CH_CORE_EXPORT ConsoleVariable<float> g_cvarSSAODepthTolerance;

// Occluded surfaces get back some of the light bouncing between them, more the brighter
// their color; without it bright occluded areas turn too dark.
extern CH_CORE_EXPORT ConsoleVariable<bool> g_cvarAOMultiBounce;

} // namespace chEngineSDK
