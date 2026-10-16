/************************************************************************/
/**
 * @file chRenderSettings.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief Console variables of the scene renderer, section [Renderer] of Engine.ini.
 */
/************************************************************************/

#include "chRenderSettings.h"

#include "chStringUtils.h"

namespace chEngineSDK {

namespace {
constexpr StringView kViewModeNames[] = {"Lit",
                                         "LitWireframe",
                                         "Wireframe",
                                         "Unlit",
                                         "Normals",
                                         "Depth",
                                         "TexCoords",
                                         "BaseColor",
                                         "Metallic",
                                         "Roughness",
                                         "AmbientOcclusion",
                                         "Emissive",
                                         "ShadowCascades",
                                         "ShadowMaps"};
static_assert(sizeof(kViewModeNames) / sizeof(kViewModeNames[0]) ==
              static_cast<SIZE_T>(ViewMode::COUNT));

constexpr StringView kRenderPathNames[] = {"Forward", "Deferred"};
static_assert(sizeof(kRenderPathNames) / sizeof(kRenderPathNames[0]) ==
              static_cast<SIZE_T>(RenderPathType::COUNT));

template<typename Enum>
NODISCARD Optional<Enum>
findByName(Span<const StringView> names, StringView name)
{
  for (SIZE_T i = 0; i < names.size(); ++i) {
    if (StringUtils::equalsIgnoreCase(name, names[i])) {
      return static_cast<Enum>(i);
    }
  }
  return {};
}
} // namespace

ConsoleVariable<String> g_cvarViewMode(
    "Renderer.ViewMode",
    "Lit",
    "What the scene shows: Lit, LitWireframe, Wireframe, Unlit, Normals, Depth, TexCoords, "
    "a GBuffer channel: BaseColor, Metallic, Roughness, AmbientOcclusion, Emissive, or "
    "ShadowCascades (Lit tinted by the shadow cascade of each pixel) or ShadowMaps (the shadow "
    "atlases of the frame).",
    "ViewMode");

ConsoleVariable<String> g_cvarRenderPath("Renderer.Path",
                                         "Deferred",
                                         "How the Lit view modes light the scene: Forward or "
                                         "Deferred.",
                                         "RenderPath");

ConsoleVariable<float> g_cvarAmbient(
    "Renderer.Ambient",
    0.03f,
    "Light that reaches every surface from all sides, scaled by the material occlusion.",
    "Ambient");

ConsoleVariable<bool> g_cvarShadows("Renderer.Shadows",
                                    true,
                                    "Lights with Cast Shadows draw shadows.",
                                    "Shadows");

ConsoleVariable<bool> g_cvarNormalMaps("Renderer.NormalMaps",
                                       true,
                                       "Materials bend their normals with their normal maps.");

ConsoleVariable<bool> g_cvarOcclusionMaps(
    "Renderer.OcclusionMaps",
    true,
    "Materials darken the ambient light with their occlusion maps.");

ConsoleVariable<bool> g_cvarEmissive("Renderer.Emissive",
                                     true,
                                     "Materials add their emissive color.");

ConsoleVariable<bool> g_cvarDirectionalLights("Renderer.DirectionalLights",
                                              true,
                                              "Directional lights light the scene.");

ConsoleVariable<bool> g_cvarPointLights("Renderer.PointLights",
                                        true,
                                        "Point lights light the scene.");

ConsoleVariable<bool> g_cvarSpotLights("Renderer.SpotLights",
                                       true,
                                       "Spot lights light the scene.");

ConsoleVariable<bool> g_cvarTonemap(
    "Renderer.Tonemap",
    true,
    "Applies the ACES curve after the exposure; off, the color is only clamped.");

ConsoleVariable<String> g_cvarDisabledPasses(
    "Renderer.DisabledPasses",
    "",
    "Comma separated names of optional render passes that do not run.",
    "DisabledPasses");

ConsoleVariable<int32> g_cvarShadowMapSize(
    "Renderer.ShadowMapSize",
    2048,
    "Width and height in texels of each cascade of the directional shadow map.",
    "ShadowMapSize");

ConsoleVariable<float> g_cvarShadowDistance(
    "Renderer.ShadowDistance",
    200.0f,
    "How far from the camera, in world units, the directional shadow reaches. A bigger "
    "distance spreads the same texels over more ground.",
    "ShadowDistance");

ConsoleVariable<int32> g_cvarShadowCascades(
    "Renderer.ShadowCascades",
    4,
    "How many maps (1 to 4) split the directional shadow distance, the nearest one the "
    "sharpest.",
    "ShadowCascades");

ConsoleVariable<float> g_cvarShadowSplitLambda(
    "Renderer.ShadowSplitLambda",
    0.8f,
    "Where the cascades split, from 0 (even steps) to 1 (steps that grow with the distance, "
    "more detail near the camera).",
    "ShadowSplitLambda");

ConsoleVariable<float> g_cvarShadowCascadeBlend(
    "Renderer.ShadowCascadeBlend",
    0.15f,
    "Fraction of each cascade, at its far end, that blends into the next one so the change "
    "does not show. 0 cuts sharply.",
    "ShadowCascadeBlend");

ConsoleVariable<float> g_cvarShadowFade(
    "Renderer.ShadowFade",
    0.2f,
    "Fraction of the shadow distance, at its far end, where the shadow fades out.",
    "ShadowFade");

ConsoleVariable<int32> g_cvarShadowFilterRadius(
    "Renderer.ShadowFilterRadius",
    1,
    "Texels around each pixel the shadow test blends (0 to 3; 1 is a 3x3 filter). Softer "
    "edges cost more samples.",
    "ShadowFilterRadius");

ConsoleVariable<float> g_cvarShadowNormalOffset(
    "Renderer.ShadowNormalOffset",
    1.5f,
    "How far, in texels of its cascade, a receiver moves along its normal before the test. "
    "Too little leaves dark stripes on lit surfaces; too much detaches shadows.",
    "ShadowNormalOffset");

ConsoleVariable<float> g_cvarShadowDepthBias(
    "Renderer.ShadowDepthBias",
    1.0f,
    "Constant depth bias of the shadow casters, in steps of the depth format.",
    "ShadowDepthBias");

ConsoleVariable<float> g_cvarShadowSlopeBias(
    "Renderer.ShadowSlopeBias",
    2.0f,
    "Depth bias of the shadow casters that grows with how steeply they face away from the "
    "light.",
    "ShadowSlopeBias");

ConsoleVariable<int32> g_cvarSpotShadowMaxLights(
    "Renderer.SpotShadowMaxLights",
    8,
    "How many spot lights with Cast Shadows (0 to 16) draw shadows in one frame. When more "
    "are in view, the ones that cover the most of the screen (range over distance to the "
    "camera) win.",
    "SpotShadowMaxLights");

ConsoleVariable<int32> g_cvarSpotShadowMapSize(
    "Renderer.SpotShadowMapSize",
    1024,
    "Width and height in texels of the shadow map of each spot light.",
    "SpotShadowMapSize");

ConsoleVariable<float> g_cvarSpotShadowDistance(
    "Renderer.SpotShadowDistance",
    100.0f,
    "How far from the camera, in world units, the range of a spot light may start and still "
    "draw its shadow.",
    "SpotShadowDistance");

ConsoleVariable<float> g_cvarSpotShadowFade(
    "Renderer.SpotShadowFade",
    0.2f,
    "Fraction of the spot shadow distance, at its far end, where the shadow of a spot light "
    "fades out.",
    "SpotShadowFade");

ConsoleVariable<float> g_cvarSpotShadowNearPlane(
    "Renderer.SpotShadowNearPlane",
    0.1f,
    "Distance in world units from a spot light to the near plane of its shadow map. Casters "
    "closer than it still cast; a bigger value gives more depth precision.",
    "SpotShadowNearPlane");

ConsoleVariable<int32> g_cvarSpotShadowFilterRadius(
    "Renderer.SpotShadowFilterRadius",
    1,
    "Texels around each pixel the spot shadow test blends (0 to 3; 1 is a 3x3 filter).",
    "SpotShadowFilterRadius");

ConsoleVariable<float> g_cvarSpotShadowNormalOffset(
    "Renderer.SpotShadowNormalOffset",
    1.5f,
    "How far, in texels of the spot shadow map at the distance of the receiver, a receiver "
    "moves along its normal before the test.",
    "SpotShadowNormalOffset");

ConsoleVariable<float> g_cvarSpotShadowDepthBias(
    "Renderer.SpotShadowDepthBias",
    1.0f,
    "Constant depth bias of the spot shadow casters, in steps of the depth format.",
    "SpotShadowDepthBias");

ConsoleVariable<float> g_cvarSpotShadowSlopeBias(
    "Renderer.SpotShadowSlopeBias",
    2.0f,
    "Depth bias of the spot shadow casters that grows with how steeply they face away from "
    "the light.",
    "SpotShadowSlopeBias");

ConsoleVariable<int32> g_cvarPointShadowMaxLights(
    "Renderer.PointShadowMaxLights",
    4,
    "How many point lights with Cast Shadows (0 to 8) draw shadows in one frame. Each one "
    "draws its casters six times, once per cube face. When more are in view, the ones that "
    "cover the most of the screen (range over distance to the camera) win.",
    "PointShadowMaxLights");

ConsoleVariable<int32> g_cvarPointShadowMapSize(
    "Renderer.PointShadowMapSize",
    512,
    "Width and height in texels of each of the six faces of the cube shadow map of a point "
    "light (256 to 2048).",
    "PointShadowMapSize");

ConsoleVariable<float> g_cvarPointShadowDistance(
    "Renderer.PointShadowDistance",
    100.0f,
    "How far from the camera, in world units, the range of a point light may start and still "
    "draw its shadow.",
    "PointShadowDistance");

ConsoleVariable<float> g_cvarPointShadowFade(
    "Renderer.PointShadowFade",
    0.2f,
    "Fraction of the point shadow distance, at its far end, where the shadow of a point "
    "light fades out.",
    "PointShadowFade");

ConsoleVariable<float> g_cvarPointShadowNearPlane(
    "Renderer.PointShadowNearPlane",
    0.1f,
    "Distance in world units from a point light to the near plane of its cube faces. "
    "Casters closer than it still cast; a bigger value gives more depth precision.",
    "PointShadowNearPlane");

ConsoleVariable<int32> g_cvarPointShadowFilterRadius(
    "Renderer.PointShadowFilterRadius",
    1,
    "Texels around each pixel the point shadow test blends (0 to 3; 1 is a 3x3 filter).",
    "PointShadowFilterRadius");

ConsoleVariable<float> g_cvarPointShadowNormalOffset(
    "Renderer.PointShadowNormalOffset",
    1.5f,
    "How far, in texels of the cube face at the distance of the receiver, a receiver moves "
    "along its normal before the test.",
    "PointShadowNormalOffset");

ConsoleVariable<float> g_cvarPointShadowDepthBias(
    "Renderer.PointShadowDepthBias",
    1.0f,
    "Constant depth bias of the point shadow casters, in steps of the depth format.",
    "PointShadowDepthBias");

ConsoleVariable<float> g_cvarPointShadowSlopeBias(
    "Renderer.PointShadowSlopeBias",
    2.0f,
    "Depth bias of the point shadow casters that grows with how steeply they face away from "
    "the light.",
    "PointShadowSlopeBias");

ConsoleVariable<bool> g_cvarImageBasedLighting(
    "Renderer.IBL",
    true,
    "The sky light lights the scene with its image (diffuse and reflections); off, or "
    "without a sky light, surfaces get the constant Renderer.Ambient.",
    "IBL");

ConsoleVariable<int32> g_cvarIBLCubeSize(
    "Renderer.IBLCubeSize",
    512,
    "Width and height of each face of the cube the sky image is turned into (64 to 2048). "
    "The reflections and the diffuse light are filtered from it.");

ConsoleVariable<int32> g_cvarIBLIrradianceSize(
    "Renderer.IBLIrradianceSize",
    32,
    "Width and height of each face of the irradiance cube, the diffuse light (8 to 128). It "
    "is very blurred, so a small one is enough.");

ConsoleVariable<int32> g_cvarIBLIrradianceSamples(
    "Renderer.IBLIrradianceSamples",
    512,
    "Samples per texel of the irradiance cube (16 to 4096).");

ConsoleVariable<int32> g_cvarIBLPrefilterSize(
    "Renderer.IBLPrefilterSize",
    256,
    "Width and height of each face of the sharpest mip of the prefiltered cube, the "
    "reflections of smooth surfaces (32 to 1024).");

ConsoleVariable<int32> g_cvarIBLPrefilterMips(
    "Renderer.IBLPrefilterMips",
    6,
    "Mips of the prefiltered cube; roughness 0 reads the first and 1 the last (2 to 10, "
    "never more than its size allows).");

ConsoleVariable<int32> g_cvarIBLPrefilterSamples(
    "Renderer.IBLPrefilterSamples",
    256,
    "GGX samples per texel of the prefiltered cube (16 to 4096).");

ConsoleVariable<int32> g_cvarIBLBrdfLutSize(
    "Renderer.IBLBrdfLutSize",
    128,
    "Width and height of the split sum lookup table of the image based lighting (32 to "
    "1024). The table is smooth, so a small one is enough.");

ConsoleVariable<int32> g_cvarIBLBrdfLutSamples(
    "Renderer.IBLBrdfLutSamples",
    512,
    "GGX samples per texel of the split sum lookup table (16 to 8192). Built once, so its "
    "cost does not matter per frame.");

/*
 */
StringView
RenderSettingsUtils::getName(ViewMode mode)
{
  CH_ASSERT(mode < ViewMode::COUNT);
  return kViewModeNames[static_cast<uint32>(mode)];
}

/*
 */
StringView
RenderSettingsUtils::getName(RenderPathType path)
{
  CH_ASSERT(path < RenderPathType::COUNT);
  return kRenderPathNames[static_cast<uint32>(path)];
}

/*
 */
Optional<ViewMode>
RenderSettingsUtils::viewModeFromName(StringView name)
{
  return findByName<ViewMode>(kViewModeNames, name);
}

/*
 */
Optional<RenderPathType>
RenderSettingsUtils::renderPathFromName(StringView name)
{
  return findByName<RenderPathType>(kRenderPathNames, name);
}

} // namespace chEngineSDK
