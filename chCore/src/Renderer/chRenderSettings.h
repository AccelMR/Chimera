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
 * looking at the scene data.
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
  COUNT
};

/**
 * Exists so the view mode can be written as a name in .ini files, on the command line and
 * in menus.
 */
class CH_CORE_EXPORT ViewModeUtils
{
 public:
  /**
   * A null terminated literal, so it can be handed to C APIs as is.
   */
  NODISCARD static StringView
  getName(ViewMode mode);

  /**
   * Ignores case; an unknown name gives nothing.
   */
  NODISCARD static Optional<ViewMode>
  fromName(StringView name);
};

// Name of a ViewMode (-ViewMode=Wireframe).
extern CH_CORE_EXPORT ConsoleVariable<String> g_cvarViewMode;

} // namespace chEngineSDK
