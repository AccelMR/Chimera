/************************************************************************/
/**
 * @file chProjectBrowserApplication.cpp
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  The editor's run without a project.
 */
/************************************************************************/
#include "chProjectBrowserApplication.h"

#include "chConsoleVariable.h"
#include "chICommandList.h"
#include "chIGraphicsAPI.h"
#include "chISwapChain.h"
#include "chITexture.h"
#include "chImGuiRenderer.h"
#include "chLogger.h"
#include "chProjectBrowserUI.h"
#include "chUIHelpers.h"

#include "imgui.h"

namespace chEngineSDK {
using namespace chUIHelpers;

CH_LOG_DECLARE_STATIC(ProjectBrowserApp, All);

namespace {
// The browser is a small window, whatever size the editor saved for itself.
constexpr const ANSICHAR* kWindowSettings[][2] = {{"Window.Width", "1100"},
                                                  {"Window.Height", "720"},
                                                  {"Window.Maximized", "false"},
                                                  {"Window.Title", "Chimera Projects"}};
} // namespace

/*
 */
ProjectBrowserApplication::ProjectBrowserApplication() = default;

/*
 */
ProjectBrowserApplication::~ProjectBrowserApplication() = default;

/*
 */
void
ProjectBrowserApplication::initializeModules()
{
  WindowedApplication::initializeModules();
  for (const auto& setting : kWindowSettings) {
    if (IConsoleVariable* variable = ConsoleVariables::find(setting[0])) {
      variable->setFromString(setting[1], ConsoleVariableSource::Code);
    }
  }
}

/*
 */
void
ProjectBrowserApplication::onPostInitialize()
{
  CH_LOG_INFO(ProjectBrowserApp, "No project given; showing the project browser.");
  m_updateInjection = UIHelpers::initializeImGui(getDisplaySurface(), getEventHandler(), nullptr);
  // One window filling the main one: nothing to dock or take out.
  ImGui::GetIO().ConfigFlags &= ~(ImGuiConfigFlags_DockingEnable |
                                  ImGuiConfigFlags_ViewportsEnable);
  m_imguiRenderer = chMakeUnique<ImGuiRenderer>();
}

/*
 */
void
ProjectBrowserApplication::destroyModules()
{
  if (IGraphicsAPI::isStarted()) {
    IGraphicsAPI::instance().waitIdle();
  }
  m_updateInjection.disconnect();
  m_imguiRenderer.reset();
  UIHelpers::shutDownImGui();
  WindowedApplication::destroyModules();
}

/*
 */
void
ProjectBrowserApplication::update(const float deltaTime)
{
  CH_PARAMETER_UNUSED(deltaTime);
  UIHelpers::newFrame();
  ProjectBrowserUI::renderWindow();
  ImGui::Render();

  Path projectFile;
  if (ProjectBrowserUI::takeChosenProject(projectFile)) {
    requestRelaunch(ProjectBrowserUI::getLaunchArguments(projectFile));
    quit();
  }
}

/*
 */
void
ProjectBrowserApplication::onRender(ICommandList& commandList, const ISwapChain& swapChain)
{
  const ITexture& image = swapChain.getCurrentTexture();
  const Array<TextureBarrier, 1> toRendering = {
      TextureBarrier{.texture = &image,
                     .before = ResourceState::Undefined,
                     .after = ResourceState::RenderTarget}};
  commandList.barrier(toRendering);

  const uint32 width = swapChain.getWidth();
  const uint32 height = swapChain.getHeight();
  RenderingDesc renderingDesc{.colorAttachmentCount = 1, .width = width, .height = height};
  renderingDesc.colorAttachments[0] = {.view = &swapChain.getCurrentTextureView(),
                                       .loadOp = LoadOp::Clear};
  commandList.beginRendering(renderingDesc);
  m_imguiRenderer->render(commandList, *ImGui::GetDrawData(), swapChain.getFormat(), width,
                          height);
  commandList.endRendering();

  const Array<TextureBarrier, 1> toPresent = {
      TextureBarrier{.texture = &image,
                     .before = ResourceState::RenderTarget,
                     .after = ResourceState::Present}};
  commandList.barrier(toPresent);
}

} // namespace chEngineSDK
