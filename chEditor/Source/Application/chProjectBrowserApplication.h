/************************************************************************/
/**
 * @file chProjectBrowserApplication.h
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  The editor's run without a project.
 */
/************************************************************************/
#pragma once

#include "chWindowedApplication.h"

namespace chEngineSDK {
class ImGuiRenderer;

/**
 * Exists so the editor started without -project asks for one: a small window with only the
 * project browser, no assets or scene. The picked project opens in a new run of the editor,
 * because the project decides the mounts and config every system reads at start up.
 */
class ProjectBrowserApplication : public WindowedApplication
{
 public:
  ProjectBrowserApplication();

  ~ProjectBrowserApplication() override;

 protected:
  void
  initializeModules() override;

  void
  onPostInitialize() override;

  void
  destroyModules() override;

  void
  update(const float deltaTime) override;

  void
  onRender(ICommandList& commandList, const ISwapChain& swapChain) override;

  NODISCARD StringView
  getDefaultApplicationId() const override { return "chimera-editor"; }

 private:
  UniquePtr<ImGuiRenderer> m_imguiRenderer;
  HEvent m_updateInjection;
};

} // namespace chEngineSDK
