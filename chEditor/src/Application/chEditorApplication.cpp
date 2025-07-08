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

#include "chEventDispatcherManager.h"
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
EditorApplication::onPostInitialize() {
  // Call super since there is when the window and render context are initialized.
  WindowedApplication::onPostInitialize();

  CH_LOG_INFO(EditorApp, "Post-initialization of EditorApplication.");
  // Initialize editor-specific components here
  initializeEditorComponents();

  bindEvents();

  CH_LOG_INFO(EditorApp, "Post-initialization completed successfully.");

}

/*
*/
void
EditorApplication::initializeEditorComponents() {
  CH_LOG_INFO(EditorApp, "Initializing editor components.");

  CH_ASSERT(EventDispatcherManager::isStarted());

  EventDispatcherManager& eventDispatcher = EventDispatcherManager::instance();

  eventDispatcher.OnKeyDown.connect([this](const KeyBoardData& data) {
    // Handle key down events specific to the editor
    if (data.hasModifier(KeyBoardModifier::LCTRL) ||
        data.hasModifier(KeyBoardModifier::RCTRL)) {
      if (data.key == chKeyBoard::Key::S) {
        CH_LOG_DEBUG(EditorApp, "Ctrl+S pressed, saving the current document.");
        // Handle Ctrl+S for saving the current document
      }
      else if (data.key == chKeyBoard::Key::O) {
        CH_LOG_DEBUG(EditorApp, "Ctrl+O pressed, opening a document.");
        // Handle Ctrl+O for opening a document
      }
    }
  });

  eventDispatcher.OnKeyUp.connect(
    [this](const KeyBoardData& /*data*/) {
      // Handle key up events specific to the editor
    });

  eventDispatcher.OnKeyPressed.connect(
    [this](const KeyBoardData& data) {
      CH_LOG_DEBUG(EditorApp, "Key pressed event: {0}, modifiers: {1}",
                   static_cast<uint32>(data.key), data.modifiers);
      // Handle key pressed events specific to the editor
    });

  eventDispatcher.OnMouseButtonDown.connect(
    [this](const MouseButtonData& data) {
      CH_LOG_DEBUG(EditorApp, "Mouse button down event: {0}", static_cast<uint32>(data.button));
      // Handle mouse button down events specific to the editor
    });
  eventDispatcher.OnMouseButtonUp.connect(
    [this](const MouseButtonData& data) {
      CH_LOG_DEBUG(EditorApp, "Mouse button up event: {0}", static_cast<uint32>(data.button));
      // Handle mouse button up events specific to the editor
    });
  eventDispatcher.OnMouseButtonPressed.connect(
    [this](const MouseButtonData& data) {
      CH_LOG_DEBUG(EditorApp, "Mouse button pressed event: {0}", static_cast<uint32>(data.button));
      // Handle mouse button pressed events specific to the editor
    });

  eventDispatcher.OnMouseMove.connect(
    [this](const MouseMoveData& /*data*/) {
      // Handle mouse move events specific to the editor
    });
  eventDispatcher.OnMouseWheel.connect(
    [this](const MouseWheelData& data) {
      CH_LOG_DEBUG(EditorApp, "Mouse wheel event: deltaY={0}, x={1}, y={2}",
                   data.deltaY, static_cast<uint32>(data.x), static_cast<uint32>(data.y));
      // Handle mouse wheel events specific to the editor
    });

  // Here you can initialize any editor-specific components, like UI, plugins, etc.
  // This is just a placeholder for now.
  // For example, you might want to initialize a UI framework or load editor plugins.
  // m_uiManager.initialize();
  // m_pluginManager.loadPlugins();
  CH_LOG_INFO(EditorApp, "Editor components initialized successfully.");
}

/*
*/
void
EditorApplication::bindEvents() {
  CH_LOG_INFO(EditorApp, "Binding editor events.");

}

} // namespace chEngineSDK
