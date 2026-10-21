/************************************************************************/
/**
 * @file chViewportGrid.cpp
 * @author AccelMR
 * @date 2026/10/16
 * @brief
 *  The ground grid of the editor viewport.
 */
/************************************************************************/
#include "chViewportGrid.h"

#include "chCamera.h"
#include "chConsoleVariable.h"
#include "chEngineConfig.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chIPipeline.h"
#include "chIShader.h"
#include "chITextureView.h"
#include "chStringUtils.h"

#include "imgui.h"

namespace chEngineSDK {

namespace {
ConsoleVariable<bool> g_cvarGrid("Editor.Grid",
                                 true,
                                 "Shows the ground grid (z = 0) in the viewport.",
                                 "Grid");
ConsoleVariable<float> g_cvarGridSpacing(
    "Editor.GridSpacing",
    1.0f,
    "Size of the smallest grid cell, in world units; every tenth line is brighter.");
ConsoleVariable<bool> g_cvarGridAdaptive(
    "Editor.GridAdaptive",
    true,
    "Makes the cells ten times bigger wherever they would look smaller than "
    "Editor.GridMinCellPixels, as in Unreal; off keeps Editor.GridSpacing everywhere.");
ConsoleVariable<float> g_cvarGridMinCellPixels(
    "Editor.GridMinCellPixels",
    12.0f,
    "With adaptive spacing, the smallest size in pixels a cell may have on screen.");
ConsoleVariable<float> g_cvarGridFadeDistance(
    "Editor.GridFadeDistance",
    500.0f,
    "Distance from the camera, in world units, where the grid has faded out; a camera high "
    "above the ground still sees it 30 times its height away.");
ConsoleVariable<float> g_cvarGridOpacity("Editor.GridOpacity",
                                         0.5f,
                                         "Opacity of the grid lines, from 0 to 1.");

// Must match PushConstants in grid.hlsl.
struct GridPushConstants
{
  Matrix4 clipToWorld;
  float cameraPositionX;
  float cameraPositionY;
  float cameraPositionZ;
  uint32 depthIndex;
  float spacing;
  float minCellPixels;
  float fadeDistance;
  float opacity;
  uint32 adaptive;
};
static_assert(sizeof(GridPushConstants) <= GraphicsLimits::PUSH_CONSTANTS_SIZE);

/*
 */
void
saveSetting(StringView key, float value)
{
  ANSICHAR buffer[StringUtils::MAX_FLOAT_CHARS];
  EngineConfig::setUserValue("Editor", "Editor", key, StringUtils::toChars(buffer, value));
}

/*
 */
void
saveSetting(StringView key, bool bValue)
{
  EngineConfig::setUserValue("Editor", "Editor", key, bValue ? "true" : "false");
}

/*
 */
void
saveSettings()
{
  saveSetting("Grid", g_cvarGrid.get());
  saveSetting("GridSpacing", g_cvarGridSpacing.get());
  saveSetting("GridAdaptive", g_cvarGridAdaptive.get());
  saveSetting("GridMinCellPixels", g_cvarGridMinCellPixels.get());
  saveSetting("GridFadeDistance", g_cvarGridFadeDistance.get());
  saveSetting("GridOpacity", g_cvarGridOpacity.get());
}

/*
 * Logarithmic for sizes and distances, because useful values go from centimeters to
 * kilometers.
 */
bool
settingSlider(const ANSICHAR* label,
              ConsoleVariable<float>& variable,
              float minValue,
              float maxValue,
              const ANSICHAR* format,
              bool bLogarithmic)
{
  float value = variable.get();
  ImGui::SetNextItemWidth(160.0f);
  if (ImGui::SliderFloat(label, &value, minValue, maxValue, format,
                         bLogarithmic ? ImGuiSliderFlags_Logarithmic : 0)) {
    variable.set(value, ConsoleVariableSource::Console);
  }
  ImGui::SetItemTooltip("%s", variable.getHelp().c_str());
  return ImGui::IsItemDeactivatedAfterEdit();
}
} // namespace

/*
 */
ViewportGrid::ViewportGrid()
{
  IGraphicsAPI& graphicsAPI = IGraphicsAPI::instance();
  m_vertexShader = graphicsAPI.loadShader(ShaderStage::Vertex, "grid");
  m_fragmentShader = graphicsAPI.loadShader(ShaderStage::Fragment, "grid");
}

/*
 */
ViewportGrid::~ViewportGrid() = default;

/*
 */
void
ViewportGrid::addPass(RenderGraph& graph,
                      const Camera& camera,
                      RGTextureHandle output,
                      RGTextureHandle depth)
{
  if (!depth.isValid()) {
    return;
  }
  updatePipeline(graph.getTextureDesc(output).format);

  const Vector3& cameraPosition = camera.getPosition();
  const GridPushConstants pushConstants{
      .clipToWorld = camera.getViewProjectionMatrix().getInverse(),
      .cameraPositionX = cameraPosition.x,
      .cameraPositionY = cameraPosition.y,
      .cameraPositionZ = cameraPosition.z,
      .depthIndex = GraphicsLimits::INVALID_BINDLESS_INDEX,
      .spacing = Math::max(g_cvarGridSpacing.get(), 0.001f),
      .minCellPixels = Math::max(g_cvarGridMinCellPixels.get(), 1.0f),
      .fadeDistance = Math::max(g_cvarGridFadeDistance.get(), 0.001f),
      .opacity = Math::clamp(g_cvarGridOpacity.get(), 0.0f, 1.0f),
      .adaptive = g_cvarGridAdaptive.get() ? 1u : 0u};

  graph.addPass("Grid")
      .read(depth)
      .writeColor(output, LoadOp::Load)
      .setOptional()
      .setExecute([this, pushConstants, depth](RenderPassContext& context) {
        ICommandList& commandList = context.getCommandList();
        GridPushConstants constants = pushConstants;
        constants.depthIndex = context.getView(depth).getBindlessIndex();
        commandList.bindPipeline(*m_pipeline);
        commandList.pushConstants(&constants, sizeof(constants));
        commandList.draw(3);
      });
}

/*
 */
bool
ViewportGrid::isVisible()
{
  return g_cvarGrid.get();
}

/*
 */
void
ViewportGrid::setVisible(bool bVisible)
{
  g_cvarGrid.set(bVisible, ConsoleVariableSource::Console);
  saveSetting("Grid", bVisible);
}

/*
 */
void
ViewportGrid::renderSettings()
{
  bool bSave = false;
  bool bVisible = g_cvarGrid.get();
  if (ImGui::Checkbox("Show Grid", &bVisible)) {
    g_cvarGrid.set(bVisible, ConsoleVariableSource::Console);
    bSave = true;
  }
  bool bAdaptive = g_cvarGridAdaptive.get();
  if (ImGui::Checkbox("Adaptive Spacing", &bAdaptive)) {
    g_cvarGridAdaptive.set(bAdaptive, ConsoleVariableSource::Console);
    bSave = true;
  }
  ImGui::SetItemTooltip("%s", g_cvarGridAdaptive.getHelp().c_str());

  bSave |= settingSlider("Spacing", g_cvarGridSpacing, 0.01f, 1000.0f, "%.2f", true);
  ImGui::BeginDisabled(!bAdaptive);
  bSave |= settingSlider("Min Cell Size", g_cvarGridMinCellPixels, 4.0f, 100.0f, "%.0f px",
                         false);
  ImGui::EndDisabled();
  bSave |= settingSlider("Fade Distance", g_cvarGridFadeDistance, 10.0f, 100000.0f, "%.0f",
                         true);
  bSave |= settingSlider("Opacity", g_cvarGridOpacity, 0.0f, 1.0f, "%.2f", false);

  if (ImGui::Button("Reset to Defaults")) {
    g_cvarGrid.set(g_cvarGrid.getDefault(), ConsoleVariableSource::Console);
    g_cvarGridSpacing.set(g_cvarGridSpacing.getDefault(), ConsoleVariableSource::Console);
    g_cvarGridAdaptive.set(g_cvarGridAdaptive.getDefault(), ConsoleVariableSource::Console);
    g_cvarGridMinCellPixels.set(g_cvarGridMinCellPixels.getDefault(),
                                ConsoleVariableSource::Console);
    g_cvarGridFadeDistance.set(g_cvarGridFadeDistance.getDefault(),
                               ConsoleVariableSource::Console);
    g_cvarGridOpacity.set(g_cvarGridOpacity.getDefault(), ConsoleVariableSource::Console);
    bSave = true;
  }
  if (bSave) {
    saveSettings();
  }
}

/*
 * Blended over the finished image; the depth test is done in the shader against the scene
 * depth, so the pass needs no depth attachment.
 */
void
ViewportGrid::updatePipeline(Format outputFormat)
{
  if (m_pipeline != nullptr && m_pipelineFormat == outputFormat) {
    return;
  }

  GraphicsPipelineDesc pipelineDesc{.vertexShader = m_vertexShader,
                                    .fragmentShader = m_fragmentShader,
                                    .raster = {.cullMode = CullMode::None},
                                    .depth = {.testEnable = false, .writeEnable = false},
                                    .colorAttachmentCount = 1};
  pipelineDesc.colorFormats[0] = outputFormat;
  pipelineDesc.blendStates[0] = {.enable = true,
                                 .srcColorFactor = BlendFactor::SrcAlpha,
                                 .dstColorFactor = BlendFactor::OneMinusSrcAlpha,
                                 .colorOp = BlendOp::Add,
                                 .srcAlphaFactor = BlendFactor::Zero,
                                 .dstAlphaFactor = BlendFactor::One,
                                 .alphaOp = BlendOp::Add};
  m_pipeline = m_pipelineCache.getOrCreate(pipelineDesc).get();
  m_pipelineFormat = outputFormat;
}

} // namespace chEngineSDK
