/************************************************************************/
/**
 * @file chInputManager.h
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  Game input: named actions bound to keys, players, rebinding, rumble and cursor.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chDisplayEvent.h"
#include "chEventSystem.h"
#include "chInputState.h"
#include "chLogger.h"
#include "chModule.h"

namespace chEngineSDK {
class ConfigFile;

CH_LOG_DECLARE_EXTERN(CH_CORE_EXPORT, InputSystem);

enum class InputActionType : uint8 {
  // Down or up.
  Button,
  // One value, such as throttle or zoom.
  Axis1D,
  // Two values, such as move or look.
  Axis2D
};

/**
 * Where a binding puts its value in an axis action. A button in an Axis1D action goes to
 * PositiveX (also None) or NegativeX; in an Axis2D action a button or a one-axis key must
 * name its direction, while a 2D key fills both axes and has no target.
 */
enum class InputAxisTarget : uint8 { None, PositiveX, NegativeX, PositiveY, NegativeY };

/**
 * One key of an action. Written in Input.ini as Key[:Target][*Scale], e.g. "W:+Y",
 * "GamepadRightStick*4", "S:-" (an Axis1D action going negative).
 */
struct InputBinding {
  InputKey key = InputKey::None;
  InputAxisTarget target = InputAxisTarget::None;
  float scale = 1.0f;

  NODISCARD bool
  operator==(const InputBinding& other) const noexcept = default;
};

enum class CursorMode : uint8 {
  Normal,
  // Not drawn over the window, still free to leave it.
  Hidden,
  // Hidden and held in place, so only its movement is read (first person look).
  Locked
};

using InputActionId = uint16;

/**
 * Exists so game code reads what the player wants ("Jump", "Move") instead of keys, and
 * the player can change which keys do it while the game runs.
 *
 * Actions come from Input.ini, read from /Engine/Config, then the project's Config folder
 * (the defaults), then /Saved/Config (the player's own bindings, written by
 * saveUserBindings), one section per action:
 *   [Move]
 *   Type=Axis2D                      Button (default), Axis1D or Axis2D
 *   Keys=W:+Y, S:-Y, D:+X, A:-X, GamepadLeftStick
 *   Context=Gameplay                 optional, "Default" when missing
 *
 * Each frame, after the events, updateActions() evaluates every action for every player.
 * Buttons held together in an axis action do not add up past their largest scale, so a
 * diagonal on the keyboard is as fast as a single key; analog keys add as they are. An
 * action reads as up when its context is off, while game input is off (the editor outside
 * Play) and while a key is being captured for rebinding.
 *
 * Players: player 0 reads the keyboard, the mouse and every gamepad; more players are
 * given their devices with setPlayerDevices. Main thread only.
 */
class CH_CORE_EXPORT InputManager : public Module<InputManager>
{
 public:
  InputManager();

  ~InputManager() override;

  /**
   * Starts a frame: forgets last frame's changes and reads the Input.* console variables.
   */
  void
  beginFrame();

  /**
   * Gives one platform event to the device state; events that are not input are ignored.
   */
  void
  processEvent(const DisplayEvent& event);

  /**
   * Evaluates every action once the frame's events are in, and gives a captured key to
   * the callback waiting for it.
   */
  void
  updateActions();

  void
  beginFixedStep();

  void
  endFixedStep();

  NODISCARD FORCEINLINE const InputState&
  getState() const noexcept
  {
    return m_state;
  }

  /**
   * Reads the actions from the Input.ini layers (see the class comment). Needs
   * EnginePaths.
   */
  void
  loadBindings();

  /**
   * Builds the actions from defaults and replaces their keys with the ones user has.
   * Bad entries are logged and skipped.
   */
  void
  loadBindings(const ConfigFile& defaults, const ConfigFile& user);

  /**
   * Writes the keys of every action that differs from its defaults into
   * /Saved/Config/Input.ini.
   */
  bool
  saveUserBindings() const;

  /**
   * Fills file with the actions whose keys differ from their defaults, as saveUserBindings
   * writes them.
   */
  void
  writeUserBindings(ConfigFile& file) const;

  /**
   * Ignores case. Look it up once and keep the id: actions are read by index.
   */
  NODISCARD InputActionId
  findAction(StringView name) const noexcept;

  NODISCARD FORCEINLINE uint32
  getActionCount() const noexcept
  {
    return static_cast<uint32>(m_actions.size());
  }

  NODISCARD const String&
  getActionName(InputActionId action) const;

  NODISCARD InputActionType
  getActionType(InputActionId action) const;

  NODISCARD const String&
  getActionContext(InputActionId action) const;

  NODISCARD Span<const InputBinding>
  getBindings(InputActionId action) const;

  NODISCARD Span<const InputBinding>
  getDefaultBindings(InputActionId action) const;

  /**
   * Replaces the binding at index, or adds one when index is the binding count. False when
   * the binding does not fit the action's type (see InputAxisTarget).
   */
  bool
  setBinding(InputActionId action, uint32 index, const InputBinding& binding);

  bool
  removeBinding(InputActionId action, uint32 index);

  void
  resetBindings(InputActionId action);

  void
  resetAllBindings();

  /**
   * Adds to outActions every action with a binding on key, to warn about a key used twice.
   */
  void
  findActionsUsingKey(InputKey key, Vector<InputActionId>& outActions) const;

  NODISCARD bool
  isActionDown(InputActionId action, uint32 player = 0) const noexcept;

  /**
   * In a fixed step, since the last fixed step; else this frame.
   */
  NODISCARD bool
  wasActionPressed(InputActionId action, uint32 player = 0) const noexcept;

  NODISCARD bool
  wasActionReleased(InputActionId action, uint32 player = 0) const noexcept;

  /**
   * Button: 0 or 1. Axis1D: its value. Axis2D: the x value.
   */
  NODISCARD float
  getActionValue(InputActionId action, uint32 player = 0) const noexcept;

  NODISCARD Vector2
  getActionValue2D(InputActionId action, uint32 player = 0) const noexcept;

  /**
   * Contexts start enabled. A context no action names is remembered for when one does.
   */
  void
  setContextEnabled(StringView context, bool bEnabled);

  NODISCARD bool
  isContextEnabled(StringView context) const noexcept;

  /**
   * Off in the editor outside Play: actions read as up and the cursor is shown, while the
   * device state (getState) keeps working for the editor's own controls.
   */
  void
  setGameInputEnabled(bool bEnabled);

  NODISCARD FORCEINLINE bool
  isGameInputEnabled() const noexcept
  {
    return m_bGameInputEnabled;
  }

  /**
   * What a game may have changed goes back to its start: contexts on, cursor normal, no
   * key capture, no rumble, actions up. Bindings stay; they belong to the player.
   */
  void
  resetGameState();

  void
  setPlayerCount(uint32 count);

  NODISCARD FORCEINLINE uint32
  getPlayerCount() const noexcept
  {
    return m_playerCount;
  }

  /**
   * gamepadMask has one bit per InputState gamepad slot.
   */
  void
  setPlayerDevices(uint32 player, bool bKeyboardMouse, uint32 gamepadMask);

  /**
   * The next key, button or axis past the button threshold that goes down is given to
   * callback once, instead of reaching the actions, which also ignore it until it is
   * released. For rebinding menus.
   */
  void
  captureNextKey(Function<void(InputKey)> callback);

  void
  cancelKeyCapture();

  NODISCARD FORCEINLINE bool
  isCapturingKey() const noexcept
  {
    return static_cast<bool>(m_keyCapture);
  }

  /**
   * Shakes the player's gamepad (the one used last of theirs), each motor 0 to 1, for
   * milliseconds; 0 stops it. False when the player has no gamepad or it cannot rumble.
   */
  bool
  rumble(uint32 player, float lowFrequency, float highFrequency, uint32 milliseconds);

  /**
   * The game's cursor. It only shows while game input is on; otherwise the cursor is
   * normal.
   */
  void
  setCursorMode(CursorMode mode);

  NODISCARD FORCEINLINE CursorMode
  getCursorMode() const noexcept
  {
    return m_cursorMode;
  }

  /**
   * The window whose cursor setCursorMode changes; null before it exists and after it
   * closes.
   */
  void
  setWindow(DisplaySurface* window);

  /**
   * What to show the player: keyboard keys as the current layout names them, gamepad
   * buttons as the player's gamepad family names them.
   */
  NODISCARD String
  getKeyDisplayName(InputKey key, uint32 player = 0) const;

  NODISCARD static bool
  parseBinding(StringView text, InputBinding& outBinding) noexcept;

  NODISCARD static String
  bindingToString(const InputBinding& binding, InputActionType type);

  /**
   * True when the binding fits an action of that type (see InputAxisTarget).
   */
  NODISCARD static bool
  isBindingValid(const InputBinding& binding, InputActionType type) noexcept;

  /**
   * The name Input.ini writes for the type: "Button", "Axis1D" or "Axis2D".
   */
  NODISCARD static StringView
  getActionTypeName(InputActionType type) noexcept;

  /**
   * Reads a type name, ignoring case; an empty text is Button.
   */
  NODISCARD static bool
  parseActionType(StringView text, InputActionType& outType) noexcept;

  static constexpr uint32 MAX_PLAYERS = 4;
  static constexpr InputActionId INVALID_ACTION = 0xFFFF;
  // Read from /Engine/Config, the project's Config folder and /Saved/Config.
  static constexpr StringView BINDINGS_FILE_NAME = "Input.ini";
  // Context of an action whose section names none.
  static constexpr StringView DEFAULT_CONTEXT = "Default";

  // A keyboard key went down or up, after the state took it. For shortcuts, where the
  // moment and the modifiers matter; held keys do not repeat it. Not filtered by game input.
  const Event<void(const KeyEvent&)> OnKeyPressed;
  const Event<void(const KeyEvent&)> OnKeyReleased;

 protected:
  void
  onShutDown() override;

 private:
  struct ActionState {
    Vector2 value = Vector2::ZERO;
    bool bDown = false;
    bool bPressed = false;
    bool bReleased = false;
    bool bPressedSinceFixed = false;
    bool bReleasedSinceFixed = false;
  };

  struct Action {
    String name;
    InputActionType type = InputActionType::Button;
    uint16 context = 0;
    Vector<InputBinding> defaultBindings;
    Vector<InputBinding> bindings;
  };

  struct Player {
    bool bKeyboardMouse = false;
    uint32 gamepadMask = 0;
    Vector<ActionState> actions;
  };

  struct Context {
    String name;
    bool bEnabled = true;
  };

  NODISCARD const ActionState*
  findState(InputActionId action, uint32 player) const noexcept;

  void
  evaluateAction(const Action& action, const Player& player, bool bEnabled,
                 ActionState& state) const noexcept;

  NODISCARD uint16
  findOrAddContext(StringView name);

  NODISCARD int32
  getPlayerGamepad(uint32 player) const noexcept;

  void
  applyCursorMode();

  void
  clearActionStates();

  InputState m_state;
  Vector<Action> m_actions;
  Vector<Context> m_contexts;
  Array<Player, MAX_PLAYERS> m_players;
  uint32 m_playerCount = 1;
  Function<void(InputKey)> m_keyCapture;
  // The last captured key; the actions ignore it until it is released.
  InputKey m_capturedKey = InputKey::None;
  DisplaySurface* m_window = nullptr;
  CursorMode m_cursorMode = CursorMode::Normal;
  CursorMode m_appliedCursorMode = CursorMode::Normal;
  bool m_bGameInputEnabled = true;
  bool m_bInFixedStep = false;
};

} // namespace chEngineSDK
