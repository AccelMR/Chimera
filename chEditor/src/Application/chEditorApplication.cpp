/************************************************************************/
/**
 * @file chEditorApplication.cpp
 * @author AccelMR
 * @date 2025/07/07
 * @details
 *  Chimera Editor application class.
 */
/************************************************************************/

#include "chEditorApplication.h"

#include "chLogger.h"

CH_LOG_DECLARE_STATIC(EditorApp, All);

namespace chEngineSDK {

  /*
*/
EditorApplication::EditorApplication() {
  CH_LOG_INFO(EditorApp, "Creating EditorApplication instance.");
}

/*
*/
EditorApplication::~EditorApplication() {
  CH_LOG_INFO(EditorApp, "Destroying EditorApplication instance.");
}

/*
*/
void
EditorApplication::initializeEditorComponents() {
  CH_LOG_INFO(EditorApp, "Initializing editor components.");
  // Here you can initialize any editor-specific components, like UI, plugins, etc.
  // This is just a placeholder for now.
  // For example, you might want to initialize a UI framework or load editor plugins.
  // m_uiManager.initialize();
  // m_pluginManager.loadPlugins();
  CH_LOG_INFO(EditorApp, "Editor components initialized successfully.");
}

} // namespace chEngineSDK
