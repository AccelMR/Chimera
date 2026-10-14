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
 * deferred path stores, whichever path is active.
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

} // namespace chEngineSDK
