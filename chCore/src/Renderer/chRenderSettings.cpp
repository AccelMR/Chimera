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
                                         "Emissive"};
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
    "or a GBuffer channel: BaseColor, Metallic, Roughness, AmbientOcclusion, Emissive.",
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
