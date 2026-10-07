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
constexpr StringView kViewModeNames[] = {
    "Lit", "LitWireframe", "Wireframe", "Unlit", "Normals", "Depth", "TexCoords"};
static_assert(sizeof(kViewModeNames) / sizeof(kViewModeNames[0]) ==
              static_cast<SIZE_T>(ViewMode::COUNT));
} // namespace

ConsoleVariable<String> g_cvarViewMode(
    "Renderer.ViewMode",
    "Lit",
    "What the scene shows: Lit, LitWireframe, Wireframe, Unlit, Normals, Depth, TexCoords.",
    "ViewMode");

/*
 */
StringView
ViewModeUtils::getName(ViewMode mode)
{
  CH_ASSERT(mode < ViewMode::COUNT);
  return kViewModeNames[static_cast<uint32>(mode)];
}

/*
 */
Optional<ViewMode>
ViewModeUtils::fromName(StringView name)
{
  for (uint32 i = 0; i < static_cast<uint32>(ViewMode::COUNT); ++i) {
    if (StringUtils::equalsIgnoreCase(name, kViewModeNames[i])) {
      return static_cast<ViewMode>(i);
    }
  }
  return {};
}

} // namespace chEngineSDK
