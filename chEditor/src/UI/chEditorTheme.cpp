/************************************************************************/
/**
 * @file chEditorTheme.cpp
 * @author AccelMR
 * @date 2026/10/15
 * @brief
 *  Colors, sizes and font of the editor UI.
 */
/************************************************************************/
#include "chEditorTheme.h"

#include "chConsoleVariable.h"
#include "chEngineConfig.h"
#include "chEnginePaths.h"
#include "chFileSystem.h"
#include "chLogger.h"
#include "chMath.h"
#include "chPath.h"
#include "chStringUtils.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"
#include "imgui.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(EditorThemeLog, All);

namespace {
ConsoleVariable<String> g_cvarTheme("Editor.Theme",
                                    String("CatppuccinMocha"),
                                    "Color theme of the editor: Classic or CatppuccinMocha.",
                                    "Theme");
ConsoleVariable<String> g_cvarFont("Editor.Font",
                                   String("JetBrainsMono"),
                                   "Font of the editor: JetBrainsMono, Inter or FiraCode.",
                                   "Font");
ConsoleVariable<float> g_cvarFontSize("Editor.FontSize",
                                      20.0f,
                                      "Height of the editor text in pixels, before UIScale.",
                                      "FontSize");
ConsoleVariable<float> g_cvarUIScale(
    "Editor.UIScale",
    1.0f,
    "Scales the editor text, paddings and roundings together.",
    "UIScale");
ConsoleVariable<float> g_cvarUIOpacity(
    "Editor.UIOpacity",
    0.92f,
    "Opacity of the editor panel backgrounds; lower shows more of the scene behind them.",
    "UIOpacity");

constexpr StringView kThemeNames[] = {"Classic", "CatppuccinMocha"};
static_assert(sizeof(kThemeNames) / sizeof(kThemeNames[0]) ==
              static_cast<SIZE_T>(EditorThemeType::COUNT));

constexpr StringView kFontNames[] = {"JetBrainsMono", "Inter", "FiraCode"};
static_assert(sizeof(kFontNames) / sizeof(kFontNames[0]) ==
              static_cast<SIZE_T>(EditorFontType::COUNT));

constexpr const ANSICHAR* kFontFiles[] = {"Fonts/JetBrainsMono-Regular.ttf",
                                          "Fonts/Inter-Regular.ttf",
                                          "Fonts/FiraCodeNerdFont-Regular.ttf"};
static_assert(sizeof(kFontFiles) / sizeof(kFontFiles[0]) ==
              static_cast<SIZE_T>(EditorFontType::COUNT));

// Font Awesome glyphs are taller than letters of the same size.
constexpr float kIconScale = 0.85f;
constexpr float kMinFontSize = 10.0f;
constexpr float kMaxFontSize = 40.0f;
constexpr float kMinUIScale = 0.5f;
constexpr float kMaxUIScale = 3.0f;
constexpr float kMinUIOpacity = 0.3f;

struct ThemeState
{
  ImFont* fonts[static_cast<SIZE_T>(EditorFontType::COUNT)] = {};
  EditorThemeType theme = EditorThemeType::CatppuccinMocha;
  EditorFontType font = EditorFontType::JetBrainsMono;
  bool bDirty = false;
};
ThemeState g_state;

template<typename T, SIZE_T N>
T
findByName(const StringView (&names)[N], StringView name, T fallback)
{
  for (SIZE_T i = 0; i < N; ++i) {
    if (StringUtils::equalsIgnoreCase(names[i], name)) {
      return static_cast<T>(i);
    }
  }
  return fallback;
}

constexpr ImVec4
fromHex(uint32 rgb, float alpha = 1.0f)
{
  return ImVec4(static_cast<float>((rgb >> 16) & 0xFF) / 255.0f,
                static_cast<float>((rgb >> 8) & 0xFF) / 255.0f,
                static_cast<float>(rgb & 0xFF) / 255.0f,
                alpha);
}

constexpr ImVec4
withAlpha(const ImVec4& color, float alpha)
{
  return ImVec4(color.x, color.y, color.z, alpha);
}

void
applyClassic(ImGuiStyle& style)
{
  ImVec4* colors = style.Colors;
  const ImVec4 flamingo = ImVec4(0.95f, 0.79f, 0.79f, 1.00f);
  const ImVec4 rosewater = ImVec4(0.96f, 0.89f, 0.89f, 1.00f);
  const ImVec4 pink = ImVec4(0.96f, 0.76f, 0.89f, 1.00f);
  const ImVec4 mauve = ImVec4(0.79f, 0.64f, 0.96f, 1.00f);
  const ImVec4 red = ImVec4(0.96f, 0.48f, 0.42f, 1.00f);
  const ImVec4 maroon = ImVec4(0.92f, 0.49f, 0.64f, 1.00f);
  const ImVec4 peach = ImVec4(0.96f, 0.67f, 0.41f, 1.00f);
  const ImVec4 yellow = ImVec4(0.96f, 0.93f, 0.51f, 1.00f);
  const ImVec4 green = ImVec4(0.65f, 0.89f, 0.63f, 1.00f);
  const ImVec4 teal = ImVec4(0.58f, 0.90f, 0.78f, 1.00f);
  const ImVec4 sky = ImVec4(0.54f, 0.88f, 0.96f, 1.00f);
  const ImVec4 sapphire = ImVec4(0.45f, 0.76f, 0.98f, 1.00f);
  const ImVec4 blue = ImVec4(0.54f, 0.69f, 0.96f, 1.00f);
  const ImVec4 lavender = ImVec4(0.71f, 0.73f, 0.96f, 1.00f);
  const ImVec4 text = ImVec4(0.80f, 0.82f, 0.96f, 1.00f);
  const ImVec4 subtext0 = ImVec4(0.65f, 0.68f, 0.82f, 1.00f);
  const ImVec4 overlay1 = ImVec4(0.50f, 0.54f, 0.69f, 1.00f);
  const ImVec4 overlay0 = ImVec4(0.43f, 0.47f, 0.62f, 1.00f);
  const ImVec4 surface2 = ImVec4(0.36f, 0.40f, 0.56f, 1.00f);
  const ImVec4 surface1 = ImVec4(0.29f, 0.33f, 0.49f, 1.00f);
  const ImVec4 surface0 = ImVec4(0.23f, 0.26f, 0.42f, 1.00f);
  const ImVec4 base = ImVec4(0.11f, 0.11f, 0.16f, 1.00f);
  const ImVec4 mantle = ImVec4(0.09f, 0.09f, 0.13f, 1.00f);

  colors[ImGuiCol_Text] = text;
  colors[ImGuiCol_TextDisabled] = subtext0;
  colors[ImGuiCol_WindowBg] = withAlpha(base, 0.85f);
  colors[ImGuiCol_ChildBg] = withAlpha(surface0, 0.2f);
  colors[ImGuiCol_PopupBg] = withAlpha(surface0, 0.98f);
  colors[ImGuiCol_Border] = withAlpha(overlay0, 0.6f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  colors[ImGuiCol_FrameBg] = withAlpha(surface0, 0.6f);
  colors[ImGuiCol_FrameBgHovered] = withAlpha(surface1, 0.8f);
  colors[ImGuiCol_FrameBgActive] = withAlpha(surface2, 0.9f);
  colors[ImGuiCol_TitleBg] = withAlpha(mantle, 0.95f);
  colors[ImGuiCol_TitleBgActive] = withAlpha(surface0, 0.95f);
  colors[ImGuiCol_TitleBgCollapsed] = withAlpha(mantle, 0.7f);

  colors[ImGuiCol_Button] = withAlpha(surface1, 0.8f);
  colors[ImGuiCol_ButtonHovered] = withAlpha(mauve, 0.3f);
  colors[ImGuiCol_ButtonActive] = withAlpha(mauve, 0.5f);
  colors[ImGuiCol_Header] = withAlpha(surface1, 0.6f);
  colors[ImGuiCol_HeaderHovered] = withAlpha(pink, 0.4f);
  colors[ImGuiCol_HeaderActive] = withAlpha(pink, 0.6f);

  colors[ImGuiCol_CheckMark] = flamingo;
  colors[ImGuiCol_SliderGrab] = withAlpha(rosewater, 0.8f);
  colors[ImGuiCol_SliderGrabActive] = flamingo;

  colors[ImGuiCol_ScrollbarBg] = withAlpha(surface0, 0.4f);
  colors[ImGuiCol_ScrollbarGrab] = withAlpha(teal, 0.6f);
  colors[ImGuiCol_ScrollbarGrabHovered] = withAlpha(sky, 0.8f);
  colors[ImGuiCol_ScrollbarGrabActive] = withAlpha(sapphire, 0.9f);

  colors[ImGuiCol_Separator] = withAlpha(peach, 0.4f);
  colors[ImGuiCol_SeparatorHovered] = withAlpha(peach, 0.6f);
  colors[ImGuiCol_SeparatorActive] = withAlpha(yellow, 0.8f);
  colors[ImGuiCol_ResizeGrip] = withAlpha(overlay0, 0.3f);
  colors[ImGuiCol_ResizeGripHovered] = withAlpha(maroon, 0.6f);
  colors[ImGuiCol_ResizeGripActive] = withAlpha(red, 0.8f);

  colors[ImGuiCol_Tab] = withAlpha(surface0, 0.6f);
  colors[ImGuiCol_TabHovered] = withAlpha(green, 0.4f);
  colors[ImGuiCol_TabSelected] = withAlpha(green, 0.6f);
  colors[ImGuiCol_TabDimmed] = withAlpha(surface0, 0.4f);
  colors[ImGuiCol_TabDimmedSelected] = withAlpha(surface1, 0.6f);

  colors[ImGuiCol_DockingPreview] = withAlpha(lavender, 0.4f);
  colors[ImGuiCol_DockingEmptyBg] = base;

  colors[ImGuiCol_PlotLines] = blue;
  colors[ImGuiCol_PlotLinesHovered] = sky;
  colors[ImGuiCol_PlotHistogram] = green;
  colors[ImGuiCol_PlotHistogramHovered] = teal;

  colors[ImGuiCol_TableHeaderBg] = withAlpha(surface1, 0.8f);
  colors[ImGuiCol_TableBorderStrong] = withAlpha(overlay1, 0.8f);
  colors[ImGuiCol_TableBorderLight] = withAlpha(overlay0, 0.5f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  colors[ImGuiCol_TableRowBgAlt] = withAlpha(surface0, 0.1f);

  colors[ImGuiCol_TextSelectedBg] = withAlpha(lavender, 0.3f);
  colors[ImGuiCol_DragDropTarget] = withAlpha(lavender, 0.8f);
  colors[ImGuiCol_NavCursor] = withAlpha(lavender, 0.8f);
  colors[ImGuiCol_NavWindowingHighlight] = withAlpha(text, 0.8f);
  colors[ImGuiCol_NavWindowingDimBg] = withAlpha(overlay0, 0.2f);
  colors[ImGuiCol_ModalWindowDimBg] = withAlpha(overlay0, 0.4f);

  style.WindowRounding = 8.0f;
  style.FrameRounding = 6.0f;
  style.PopupRounding = 6.0f;
  style.ScrollbarRounding = 12.0f;
  style.GrabRounding = 6.0f;
  style.TabRounding = 6.0f;
  style.ChildRounding = 6.0f;

  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 0.0f;
  style.PopupBorderSize = 1.0f;
  style.ChildBorderSize = 1.0f;
  style.TabBorderSize = 0.0f;

  style.WindowPadding = ImVec2(12.0f, 12.0f);
  style.FramePadding = ImVec2(8.0f, 6.0f);
  style.ItemSpacing = ImVec2(8.0f, 6.0f);
  style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
  style.IndentSpacing = 20.0f;
  style.ScrollbarSize = 16.0f;
  style.GrabMinSize = 12.0f;

  style.DisabledAlpha = 0.6f;
}

void
applyCatppuccinMocha(ImGuiStyle& style)
{
  const ImVec4 text = fromHex(0xCDD6F4);
  const ImVec4 overlay0 = fromHex(0x6C7086);
  const ImVec4 surface2 = fromHex(0x585B70);
  const ImVec4 surface1 = fromHex(0x45475A);
  const ImVec4 surface0 = fromHex(0x313244);
  const ImVec4 base = fromHex(0x1E1E2E);
  const ImVec4 mantle = fromHex(0x181825);
  const ImVec4 crust = fromHex(0x11111B);
  const ImVec4 lavender = fromHex(0xB4BEFE);
  const ImVec4 sapphire = fromHex(0x74C7EC);
  const ImVec4 mauve = fromHex(0xCBA6F7);
  const ImVec4 peach = fromHex(0xFAB387);
  const ImVec4 green = fromHex(0xA6E3A1);
  const ImVec4 red = fromHex(0xF38BA8);

  ImVec4* colors = style.Colors;
  colors[ImGuiCol_Text] = text;
  colors[ImGuiCol_TextDisabled] = overlay0;

  colors[ImGuiCol_WindowBg] = base;
  colors[ImGuiCol_ChildBg] = withAlpha(mantle, 0.0f);
  colors[ImGuiCol_PopupBg] = withAlpha(crust, 0.96f);

  colors[ImGuiCol_Border] = surface0;
  colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

  colors[ImGuiCol_FrameBg] = surface0;
  colors[ImGuiCol_FrameBgHovered] = surface1;
  colors[ImGuiCol_FrameBgActive] = surface2;

  colors[ImGuiCol_TitleBg] = mantle;
  colors[ImGuiCol_TitleBgActive] = base;
  colors[ImGuiCol_TitleBgCollapsed] = crust;
  colors[ImGuiCol_MenuBarBg] = mantle;

  colors[ImGuiCol_ScrollbarBg] = withAlpha(mantle, 0.0f);
  colors[ImGuiCol_ScrollbarGrab] = surface1;
  colors[ImGuiCol_ScrollbarGrabHovered] = surface2;
  colors[ImGuiCol_ScrollbarGrabActive] = overlay0;

  colors[ImGuiCol_CheckMark] = lavender;
  colors[ImGuiCol_SliderGrab] = sapphire;
  colors[ImGuiCol_SliderGrabActive] = withAlpha(sapphire, 0.8f);
  // Hover and press only tint the button, so the light text stays readable on it.
  colors[ImGuiCol_Button] = surface0;
  colors[ImGuiCol_ButtonHovered] = withAlpha(mauve, 0.35f);
  colors[ImGuiCol_ButtonActive] = withAlpha(mauve, 0.55f);
  colors[ImGuiCol_Header] = surface0;
  colors[ImGuiCol_HeaderHovered] = surface1;
  colors[ImGuiCol_HeaderActive] = surface2;

  colors[ImGuiCol_Separator] = surface0;
  colors[ImGuiCol_SeparatorHovered] = withAlpha(lavender, 0.6f);
  colors[ImGuiCol_SeparatorActive] = lavender;
  colors[ImGuiCol_ResizeGrip] = withAlpha(surface1, 0.0f);
  colors[ImGuiCol_ResizeGripHovered] = withAlpha(lavender, 0.6f);
  colors[ImGuiCol_ResizeGripActive] = lavender;
  colors[ImGuiCol_InputTextCursor] = text;

  colors[ImGuiCol_Tab] = mantle;
  colors[ImGuiCol_TabHovered] = surface1;
  colors[ImGuiCol_TabSelected] = surface0;
  colors[ImGuiCol_TabSelectedOverline] = mauve;
  colors[ImGuiCol_TabDimmed] = mantle;
  colors[ImGuiCol_TabDimmedSelected] = base;
  colors[ImGuiCol_TabDimmedSelectedOverline] = withAlpha(mauve, 0.0f);

  colors[ImGuiCol_DockingPreview] = withAlpha(lavender, 0.5f);
  colors[ImGuiCol_DockingEmptyBg] = base;

  colors[ImGuiCol_PlotLines] = peach;
  colors[ImGuiCol_PlotLinesHovered] = red;
  colors[ImGuiCol_PlotHistogram] = green;
  colors[ImGuiCol_PlotHistogramHovered] = peach;

  colors[ImGuiCol_TableHeaderBg] = surface0;
  colors[ImGuiCol_TableBorderStrong] = surface1;
  colors[ImGuiCol_TableBorderLight] = surface0;
  colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  colors[ImGuiCol_TableRowBgAlt] = withAlpha(surface0, 0.3f);

  colors[ImGuiCol_TextLink] = sapphire;
  colors[ImGuiCol_TextSelectedBg] = withAlpha(surface2, 0.8f);
  colors[ImGuiCol_TreeLines] = surface1;
  colors[ImGuiCol_DragDropTarget] = mauve;
  colors[ImGuiCol_UnsavedMarker] = peach;
  colors[ImGuiCol_NavCursor] = lavender;
  colors[ImGuiCol_NavWindowingHighlight] = withAlpha(lavender, 0.7f);
  colors[ImGuiCol_NavWindowingDimBg] = withAlpha(crust, 0.5f);
  colors[ImGuiCol_ModalWindowDimBg] = withAlpha(crust, 0.6f);

  style.WindowPadding = ImVec2(12.0f, 12.0f);
  style.FramePadding = ImVec2(6.0f, 4.0f);
  style.ItemSpacing = ImVec2(8.0f, 6.0f);
  style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
  style.IndentSpacing = 18.0f;
  style.ScrollbarSize = 14.0f;
  style.GrabMinSize = 12.0f;

  style.WindowRounding = 8.0f;
  style.ChildRounding = 5.0f;
  style.FrameRounding = 5.0f;
  style.PopupRounding = 5.0f;
  style.ScrollbarRounding = 12.0f;
  style.GrabRounding = 5.0f;
  style.TabRounding = 5.0f;

  style.WindowBorderSize = 1.0f;
  style.ChildBorderSize = 1.0f;
  style.FrameBorderSize = 0.0f;
  style.PopupBorderSize = 1.0f;
  style.TabBorderSize = 0.0f;
  style.TabBarOverlineSize = 2.0f;
  style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
  style.WindowMenuButtonPosition = ImGuiDir_None;

  style.DisabledAlpha = 0.5f;
}

void
applyOpacity(ImGuiStyle& style, float opacity)
{
  constexpr ImGuiCol kBackgrounds[] = {ImGuiCol_WindowBg,
                                       ImGuiCol_ChildBg,
                                       ImGuiCol_TitleBg,
                                       ImGuiCol_TitleBgActive,
                                       ImGuiCol_TitleBgCollapsed,
                                       ImGuiCol_MenuBarBg,
                                       ImGuiCol_Tab,
                                       ImGuiCol_TabDimmed,
                                       ImGuiCol_DockingEmptyBg};
  for (const ImGuiCol color : kBackgrounds) {
    style.Colors[color].w *= opacity;
  }
  // Popups are drawn over the text of other panels, so they lose only half as much.
  style.Colors[ImGuiCol_PopupBg].w *= (1.0f + opacity) * 0.5f;
}

void
applyStyle()
{
  // Starts from a clean style (ImGui's dark colors), so no size or color of the previous
  // theme stays behind.
  ImGuiStyle style;
  if (g_state.theme == EditorThemeType::Classic) {
    applyClassic(style);
  }
  else {
    applyCatppuccinMocha(style);
  }
  applyOpacity(style, Math::clamp(g_cvarUIOpacity.get(), kMinUIOpacity, 1.0f));

  const float scale = Math::clamp(g_cvarUIScale.get(), kMinUIScale, kMaxUIScale);
  style.ScaleAllSizes(scale);
  style.FontScaleDpi = scale;
  style.FontSizeBase = Math::clamp(g_cvarFontSize.get(), kMinFontSize, kMaxFontSize);
  // ImGui keeps its own copy of the size while a frame runs and copies this one at the next.
  style._NextFrameFontSizeBase = style.FontSizeBase;

  ImGui::GetStyle() = style;

  ImFont* font = g_state.fonts[static_cast<SIZE_T>(g_state.font)];
  if (font) {
    ImGui::GetIO().FontDefault = font;
  }
}

void
saveSetting(StringView key, StringView value)
{
  EngineConfig::setUserValue("Editor", "Editor", key, value);
}

void
saveSetting(StringView key, float value)
{
  ANSICHAR buffer[StringUtils::MAX_FLOAT_CHARS];
  saveSetting(key, StringUtils::toChars(buffer, value));
}

// Applies on every change, so a drag shows at once, and writes the file only when the drag
// ends.
void
editSetting(const ANSICHAR* label,
            ConsoleVariable<float>& cvar,
            StringView key,
            float speed,
            float min,
            float max,
            const ANSICHAR* format)
{
  float value = cvar.get();
  if (ImGui::DragFloat(label, &value, speed, min, max, format,
                       ImGuiSliderFlags_AlwaysClamp)) {
    cvar.set(value, ConsoleVariableSource::Console);
    g_state.bDirty = true;
  }
  ImGui::SetItemTooltip("%s", cvar.getHelp().c_str());
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    saveSetting(key, cvar.get());
  }
}

void
loadFonts()
{
  ImGuiIO& io = ImGui::GetIO();
  const float fontSize = Math::clamp(g_cvarFontSize.get(), kMinFontSize, kMaxFontSize);
  const float iconSize = fontSize * kIconScale;

  // The atlas keeps the pointer, so the ranges must outlive it.
  static const ImWchar kIconRanges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};

  // ImGui opens the files itself, so it needs the real paths, not the virtual ones.
  const Path& contentDirectory = EnginePaths::getEditorContentDirectory();
  const String iconFile =
      FileSystem::absolutePath(contentDirectory.join(Path("Fonts/fa-solid-900.ttf")))
          .toString();
  const bool bHasIcons = FileSystem::isFile(Path(iconFile));

  for (SIZE_T i = 0; i < static_cast<SIZE_T>(EditorFontType::COUNT); ++i) {
    const Path fontPath = FileSystem::absolutePath(contentDirectory.join(Path(kFontFiles[i])));
    // ImGui asserts on a missing file, so it is checked first.
    if (!FileSystem::isFile(fontPath)) {
      CH_LOG_ERROR(EditorThemeLog, "Font file not found: {0}", fontPath);
      continue;
    }

    // Sizes are rasterized when a frame first asks for them, so loading every font only
    // reads the files.
    ImFontConfig fontConfig;
    StringUtils::copyToBuffer(fontConfig.Name, kFontNames[i]);
    g_state.fonts[i] =
        io.Fonts->AddFontFromFileTTF(fontPath.toString().c_str(), fontSize, &fontConfig);

    if (g_state.fonts[i] && bHasIcons) {
      ImFontConfig iconConfig;
      iconConfig.MergeMode = true;
      iconConfig.PixelSnapH = true;
      iconConfig.GlyphMinAdvanceX = iconSize;
      io.Fonts->AddFontFromFileTTF(iconFile.c_str(), iconSize, &iconConfig, kIconRanges);
    }
  }

  bool bAnyFont = false;
  for (const ImFont* font : g_state.fonts) {
    bAnyFont = bAnyFont || font != nullptr;
  }
  if (!bAnyFont) {
    io.Fonts->AddFontDefault();
  }
}
} // namespace

/*
 */
void
EditorTheme::initialize()
{
  g_state.theme =
      findByName(kThemeNames, g_cvarTheme.get(), EditorThemeType::CatppuccinMocha);
  g_state.font = findByName(kFontNames, g_cvarFont.get(), EditorFontType::JetBrainsMono);
  loadFonts();
  applyStyle();
}

/*
 */
void
EditorTheme::applyPending()
{
  if (!g_state.bDirty) {
    return;
  }
  g_state.bDirty = false;
  applyStyle();
}

/*
 */
void
EditorTheme::renderSettings()
{
  if (ImGui::BeginMenu("Theme")) {
    for (SIZE_T i = 0; i < static_cast<SIZE_T>(EditorThemeType::COUNT); ++i) {
      const EditorThemeType theme = static_cast<EditorThemeType>(i);
      if (ImGui::MenuItem(kThemeNames[i].data(), nullptr, g_state.theme == theme) &&
          g_state.theme != theme) {
        g_state.theme = theme;
        g_cvarTheme.set(String(kThemeNames[i]), ConsoleVariableSource::Console);
        saveSetting("Theme", kThemeNames[i]);
        g_state.bDirty = true;
      }
    }
    ImGui::EndMenu();
  }
  if (ImGui::BeginMenu("Font")) {
    for (SIZE_T i = 0; i < static_cast<SIZE_T>(EditorFontType::COUNT); ++i) {
      // A font whose file was missing cannot be picked.
      ImGui::BeginDisabled(g_state.fonts[i] == nullptr);
      const EditorFontType font = static_cast<EditorFontType>(i);
      if (ImGui::MenuItem(kFontNames[i].data(), nullptr, g_state.font == font) &&
          g_state.font != font) {
        g_state.font = font;
        g_cvarFont.set(String(kFontNames[i]), ConsoleVariableSource::Console);
        saveSetting("Font", kFontNames[i]);
        g_state.bDirty = true;
      }
      ImGui::EndDisabled();
    }
    ImGui::EndMenu();
  }

  editSetting("Font Size", g_cvarFontSize, "FontSize", 0.1f, kMinFontSize, kMaxFontSize,
              "%.0f px");
  editSetting("UI Scale", g_cvarUIScale, "UIScale", 0.01f, kMinUIScale, kMaxUIScale,
              "%.2f");
  editSetting("Opacity", g_cvarUIOpacity, "UIOpacity", 0.005f, kMinUIOpacity, 1.0f,
              "%.2f");

  if (ImGui::MenuItem("Reset to Defaults")) {
    g_cvarFontSize.set(g_cvarFontSize.getDefault(), ConsoleVariableSource::Console);
    g_cvarUIScale.set(g_cvarUIScale.getDefault(), ConsoleVariableSource::Console);
    g_cvarUIOpacity.set(g_cvarUIOpacity.getDefault(), ConsoleVariableSource::Console);
    g_cvarTheme.set(g_cvarTheme.getDefault(), ConsoleVariableSource::Console);
    g_cvarFont.set(g_cvarFont.getDefault(), ConsoleVariableSource::Console);
    g_state.theme =
        findByName(kThemeNames, g_cvarTheme.get(), EditorThemeType::CatppuccinMocha);
    g_state.font = findByName(kFontNames, g_cvarFont.get(), EditorFontType::JetBrainsMono);
    saveSetting("Theme", g_cvarTheme.get());
    saveSetting("Font", g_cvarFont.get());
    saveSetting("FontSize", g_cvarFontSize.get());
    saveSetting("UIScale", g_cvarUIScale.get());
    saveSetting("UIOpacity", g_cvarUIOpacity.get());
    g_state.bDirty = true;
  }
}

} // namespace chEngineSDK
