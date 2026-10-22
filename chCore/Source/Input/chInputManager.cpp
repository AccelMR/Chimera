/************************************************************************/
/**
 * @file chInputManager.cpp
 * @author AccelMR
 * @date 2026/10/21
 * @brief
 *  Game input: named actions bound to keys, players, rebinding, rumble and cursor.
 */
/************************************************************************/
#include "chInputManager.h"

#include <charconv>

#include "chConfigFile.h"
#include "chConsoleVariable.h"
#include "chEnginePaths.h"
#include "chInputPlatform.h"
#include "chMath.h"
#include "chPath.h"
#include "chStringUtils.h"

namespace chEngineSDK {

CH_LOG_DEFINE_CATEGORY_SHARED(InputSystem, Info);

namespace {
ConsoleVariable<float> g_cvarStickDeadZone("Input.GamepadStickDeadZone", 0.15f,
                                           "Part of a gamepad stick's travel, from its "
                                           "center, that reads as zero.");

ConsoleVariable<float> g_cvarTriggerDeadZone("Input.GamepadTriggerDeadZone", 0.05f,
                                             "Part of a gamepad trigger's travel, from "
                                             "rest, that reads as zero.");

ConsoleVariable<float> g_cvarAxisButtonThreshold("Input.AxisButtonThreshold", 0.5f,
                                                 "How far an axis must move to count as a "
                                                 "button that is down, in actions and "
                                                 "rebinding.");

constexpr StringView kDefaultContext = "Default";
constexpr StringView kInputFileName = "Input.ini";

NODISCARD Vector2
getTargetDirection(InputAxisTarget target) noexcept
{
  switch (target) {
  case InputAxisTarget::NegativeX:
    return Vector2(-1.0f, 0.0f);
  case InputAxisTarget::PositiveY:
    return Vector2(0.0f, 1.0f);
  case InputAxisTarget::NegativeY:
    return Vector2(0.0f, -1.0f);
  default:
    return Vector2(1.0f, 0.0f);
  }
}

NODISCARD bool
parseActionType(StringView text, InputActionType& outType) noexcept
{
  if (text.empty() || StringUtils::equalsIgnoreCase(text, "Button")) {
    outType = InputActionType::Button;
  }
  else if (StringUtils::equalsIgnoreCase(text, "Axis1D")) {
    outType = InputActionType::Axis1D;
  }
  else if (StringUtils::equalsIgnoreCase(text, "Axis2D")) {
    outType = InputActionType::Axis2D;
  }
  else {
    return false;
  }
  return true;
}

/*
 * Reads a comma list of bindings; the ones that do not parse or do not fit the action are
 * logged and left out.
 */
void
parseBindingList(StringView text, const String& actionName, InputActionType type,
                 Vector<InputBinding>& outBindings)
{
  outBindings.clear();
  while (!text.empty()) {
    const SIZE_T comma = text.find(',');
    const StringView item = StringUtils::trimView(text.substr(0, comma));
    text = comma == StringView::npos ? StringView() : text.substr(comma + 1);
    if (item.empty()) {
      continue;
    }

    InputBinding binding;
    if (!InputManager::parseBinding(item, binding)) {
      CH_LOG_WARNING(InputSystem, "Input action {0}: cannot read the key \"{1}\".", actionName,
                     item);
      continue;
    }
    if (!InputManager::isBindingValid(binding, type)) {
      CH_LOG_WARNING(InputSystem,
                     "Input action {0}: \"{1}\" does not fit its type (a button or one-axis "
                     "key in an Axis2D action needs :+X, :-X, :+Y or :-Y).",
                     actionName, item);
      continue;
    }
    outBindings.push_back(binding);
  }
}

/*
 * "GamepadLeftShoulder" -> "Left Shoulder", for keys the platform has no name for.
 */
NODISCARD String
makeReadableName(StringView name)
{
  constexpr StringView kGamepadPrefix = "Gamepad";
  if (name.starts_with(kGamepadPrefix)) {
    name.remove_prefix(kGamepadPrefix.size());
  }

  String result;
  result.reserve(name.size() + 4);
  for (SIZE_T i = 0; i < name.size(); ++i) {
    const ANSICHAR character = name[i];
    const bool bUpper = character >= 'A' && character <= 'Z';
    const bool bDigit = character >= '0' && character <= '9';
    const ANSICHAR previous = i > 0 ? name[i - 1] : ' ';
    const bool bPreviousLower = previous >= 'a' && previous <= 'z';
    if (i > 0 && (bUpper || bDigit) && bPreviousLower) {
      result += ' ';
    }
    result += character;
  }
  return result;
}

/*
 * Face, back and start buttons carry different marks on each family; the rest are named
 * by position.
 */
NODISCARD StringView
getGamepadButtonLabel(InputKey key, GamepadType type) noexcept
{
  struct Labels {
    StringView south;
    StringView east;
    StringView west;
    StringView north;
    StringView back;
    StringView start;
  };
  constexpr Labels kXbox{"A", "B", "X", "Y", "View", "Menu"};
  constexpr Labels kPlayStation{"Cross", "Circle", "Square", "Triangle", "Create", "Options"};
  constexpr Labels kNintendo{"B", "A", "Y", "X", "Minus", "Plus"};

  const Labels& labels = type == GamepadType::PlayStation ? kPlayStation
                         : type == GamepadType::Nintendo  ? kNintendo
                                                          : kXbox;
  switch (key) {
  case InputKey::GamepadSouth:
    return labels.south;
  case InputKey::GamepadEast:
    return labels.east;
  case InputKey::GamepadWest:
    return labels.west;
  case InputKey::GamepadNorth:
    return labels.north;
  case InputKey::GamepadBack:
    return labels.back;
  case InputKey::GamepadStart:
    return labels.start;
  default:
    return StringView();
  }
}
} // namespace

/*
 */
InputManager::InputManager()
{
  m_players[0].bKeyboardMouse = true;
  m_players[0].gamepadMask = InputState::ALL_GAMEPADS;
  m_contexts.push_back({.name = String(kDefaultContext), .bEnabled = true});
}

/*
 */
InputManager::~InputManager() = default;

/*
 */
void
InputManager::onShutDown()
{
  m_keyCapture = nullptr;
  if (m_window && m_appliedCursorMode != CursorMode::Normal) {
    InputPlatform::setCursorMode(*m_window, CursorMode::Normal);
  }
  m_window = nullptr;
  InputPlatform::shutDown();
}

/*
 */
void
InputManager::beginFrame()
{
  m_state.beginFrame();
  m_state.setAnalogSettings(
      {.stickDeadZone = Math::clamp(g_cvarStickDeadZone.get(), 0.0f, 0.95f),
       .triggerDeadZone = Math::clamp(g_cvarTriggerDeadZone.get(), 0.0f, 0.95f),
       .buttonThreshold = Math::clamp(g_cvarAxisButtonThreshold.get(), 0.05f, 1.0f)});
}

/*
 */
void
InputManager::processEvent(const DisplayEvent& event)
{
  if (const KeyEvent* key = std::get_if<KeyEvent>(&event)) {
    m_state.setKey(key->key, key->bDown);
    if (key->bDown) {
      OnKeyPressed(*key);
    }
    else {
      OnKeyReleased(*key);
    }
  }
  else if (const MouseMoveEvent* move = std::get_if<MouseMoveEvent>(&event)) {
    m_state.addMouseMove(move->x, move->y, move->deltaX, move->deltaY);
  }
  else if (const MouseButtonEvent* button = std::get_if<MouseButtonEvent>(&event)) {
    m_state.setKey(button->button, button->bDown);
  }
  else if (const MouseWheelEvent* wheel = std::get_if<MouseWheelEvent>(&event)) {
    m_state.addMouseWheel(wheel->deltaX, wheel->deltaY);
  }
  else if (const GamepadAxisEvent* axis = std::get_if<GamepadAxisEvent>(&event)) {
    m_state.setGamepadAxis(axis->slot, axis->axis, axis->value);
  }
  else if (const GamepadButtonEvent* pad = std::get_if<GamepadButtonEvent>(&event)) {
    m_state.setGamepadButton(pad->slot, pad->button, pad->bDown);
  }
  else if (const GamepadConnectionEvent* connection =
               std::get_if<GamepadConnectionEvent>(&event)) {
    m_state.setGamepadConnected(connection->slot, connection->bConnected, connection->type);
  }
  else if (const FocusEvent* focus = std::get_if<FocusEvent>(&event)) {
    if (!focus->bFocused) {
      m_state.releaseAll();
    }
  }
}

/*
 */
void
InputManager::updateActions()
{
  if (m_capturedKey != InputKey::None && !m_state.isDown(m_capturedKey)) {
    m_capturedKey = InputKey::None;
  }

  // The captured key goes to the menu only, also in the frame it is captured.
  const bool bCapturing = static_cast<bool>(m_keyCapture);
  if (bCapturing) {
    const InputKey key = m_state.getFirstPressedKey();
    if (key != InputKey::None) {
      // Still held on the next frames, it would trigger the action it was just bound to.
      m_capturedKey = key;
      // Moved out first: the callback may start another capture or change bindings.
      Function<void(InputKey)> callback = std::move(m_keyCapture);
      m_keyCapture = nullptr;
      callback(key);
    }
  }

  const bool bReading = m_bGameInputEnabled && !bCapturing && !m_keyCapture;
  for (uint32 playerIndex = 0; playerIndex < m_playerCount; ++playerIndex) {
    Player& player = m_players[playerIndex];
    for (SIZE_T i = 0; i < m_actions.size(); ++i) {
      const Action& action = m_actions[i];
      evaluateAction(action, player, bReading && m_contexts[action.context].bEnabled,
                     player.actions[i]);
    }
  }
}

/*
 */
void
InputManager::evaluateAction(const Action& action, const Player& player, bool bEnabled,
                             ActionState& state) const noexcept
{
  const float threshold = g_cvarAxisButtonThreshold.get();
  Vector2 value = Vector2::ZERO;
  Vector2 digital = Vector2::ZERO;
  float maxDigitalScale = 0.0f;
  bool bDown = false;
  bool bAnyPressed = false;

  if (bEnabled) {
    for (const InputBinding& binding : action.bindings) {
      const InputDevice device = InputKeyUtils::getDevice(binding.key);
      if ((device != InputDevice::Gamepad && !player.bKeyboardMouse) ||
          binding.key == m_capturedKey) {
        continue;
      }
      const uint32 mask = player.gamepadMask;

      switch (InputKeyUtils::getType(binding.key)) {
      case InputKeyType::Button: {
        bAnyPressed |= m_state.wasPressed(binding.key, mask);
        if (!m_state.isDown(binding.key, mask)) {
          break;
        }
        if (action.type == InputActionType::Button) {
          bDown = true;
        }
        else {
          digital += getTargetDirection(binding.target) * binding.scale;
          maxDigitalScale = Math::max(maxDigitalScale, Math::abs(binding.scale));
        }
      } break;

      case InputKeyType::Axis1D: {
        const float axis = m_state.getAxis(binding.key, mask) * binding.scale;
        if (action.type == InputActionType::Button) {
          bDown |= Math::abs(axis) >= threshold;
        }
        else {
          value += getTargetDirection(binding.target) * axis;
        }
      } break;

      case InputKeyType::Axis2D: {
        const Vector2 axis = m_state.getAxis2D(binding.key, mask) * binding.scale;
        if (action.type == InputActionType::Button) {
          bDown |= axis.magnitude() >= threshold;
        }
        else {
          value += axis;
        }
      } break;

      default:
        break;
      }
    }
  }

  if (action.type == InputActionType::Button) {
    value = Vector2(bDown ? 1.0f : 0.0f, 0.0f);
  }
  else {
    const float digitalLength = digital.magnitude();
    if (digitalLength > maxDigitalScale) {
      digital *= maxDigitalScale / digitalLength;
    }
    value += digital;
    if (action.type == InputActionType::Axis1D) {
      value.y = 0.0f;
    }
    bDown = value.x != 0.0f || value.y != 0.0f;
  }

  // A key pressed and released inside the frame never shows as down, but still counts.
  const bool bWasDown = state.bDown;
  const bool bTap = !bWasDown && !bDown && bAnyPressed;
  state.value = value;
  state.bDown = bDown;
  state.bPressed = (!bWasDown && bDown) || bTap;
  state.bReleased = (bWasDown && !bDown) || bTap;
  state.bPressedSinceFixed |= state.bPressed;
  state.bReleasedSinceFixed |= state.bReleased;
}

/*
 */
void
InputManager::beginFixedStep()
{
  m_bInFixedStep = true;
  m_state.beginFixedStep();
}

/*
 */
void
InputManager::endFixedStep()
{
  m_bInFixedStep = false;
  m_state.endFixedStep();
  for (uint32 playerIndex = 0; playerIndex < m_playerCount; ++playerIndex) {
    for (ActionState& state : m_players[playerIndex].actions) {
      state.bPressedSinceFixed = false;
      state.bReleasedSinceFixed = false;
    }
  }
}

/*
 */
void
InputManager::loadBindings()
{
  ConfigFile defaults;
  const Path engineFile = Path("/Engine/Config").join(Path(String(kInputFileName)));
  if (defaults.load(engineFile)) {
    CH_LOG_INFO(InputSystem, "Loaded {0}", engineFile);
  }
  const Path projectFile =
      EnginePaths::getProjectConfigDirectory().join(Path(String(kInputFileName)));
  if (defaults.load(projectFile)) {
    CH_LOG_INFO(InputSystem, "Loaded {0}", projectFile);
  }

  ConfigFile user;
  const Path userFile = EnginePaths::getConfigDirectory().join(Path(String(kInputFileName)));
  if (user.load(userFile)) {
    CH_LOG_INFO(InputSystem, "Loaded {0}", userFile);
  }
  loadBindings(defaults, user);
}

/*
 */
void
InputManager::loadBindings(const ConfigFile& defaults, const ConfigFile& user)
{
  m_actions.clear();
  for (const ConfigFile::Section& section : defaults.getSections()) {
    if (section.name.empty()) {
      continue;
    }
    if (findAction(section.name) != INVALID_ACTION) {
      CH_LOG_WARNING(InputSystem, "Input action {0} is defined twice; the first one is kept.",
                     section.name);
      continue;
    }
    if (m_actions.size() >= INVALID_ACTION) {
      CH_LOG_ERROR(InputSystem, "Too many input actions; {0} and the rest are skipped.",
                   section.name);
      break;
    }

    Action action;
    action.name = section.name;
    const String* typeText = defaults.getValue(section.name, "Type");
    if (!parseActionType(typeText ? StringView(*typeText) : StringView(), action.type)) {
      CH_LOG_WARNING(InputSystem,
                     "Input action {0}: unknown type \"{1}\" (Button, Axis1D or Axis2D); "
                     "skipped.",
                     section.name, *typeText);
      continue;
    }
    const String* contextText = defaults.getValue(section.name, "Context");
    action.context = findOrAddContext(
        contextText && !contextText->empty() ? StringView(*contextText) : kDefaultContext);
    if (const String* keys = defaults.getValue(section.name, "Keys")) {
      parseBindingList(*keys, action.name, action.type, action.defaultBindings);
    }
    action.bindings = action.defaultBindings;
    m_actions.push_back(std::move(action));
  }

  for (const ConfigFile::Section& section : user.getSections()) {
    const String* keys = user.getValue(section.name, "Keys");
    if (!keys) {
      continue;
    }
    const InputActionId id = findAction(section.name);
    if (id == INVALID_ACTION) {
      CH_LOG_DEBUG(InputSystem, "Saved keys for the unknown input action {0} are ignored.",
                   section.name);
      continue;
    }
    Action& action = m_actions[id];
    parseBindingList(*keys, action.name, action.type, action.bindings);
  }

  for (Player& player : m_players) {
    player.actions.assign(m_actions.size(), ActionState{});
  }
  CH_LOG_INFO(InputSystem, "{0} input actions, {1} contexts.", m_actions.size(),
              m_contexts.size());
}

/*
 */
void
InputManager::writeUserBindings(ConfigFile& file) const
{
  for (const Action& action : m_actions) {
    if (action.bindings == action.defaultBindings) {
      continue;
    }
    String keys;
    for (const InputBinding& binding : action.bindings) {
      if (!keys.empty()) {
        keys += ", ";
      }
      keys += bindingToString(binding, action.type);
    }
    file.setValue(action.name, "Keys", keys);
  }
}

/*
 */
bool
InputManager::saveUserBindings() const
{
  // Written whole every time: the file only holds what differs from the defaults, so a
  // binding put back to its default must disappear from it.
  ConfigFile file;
  writeUserBindings(file);
  const Path path = EnginePaths::getConfigDirectory().join(Path(String(kInputFileName)));
  if (!file.save(path)) {
    CH_LOG_ERROR(InputSystem, "Could not save the input bindings to {0}.", path);
    return false;
  }
  return true;
}

/*
 */
InputActionId
InputManager::findAction(StringView name) const noexcept
{
  for (SIZE_T i = 0; i < m_actions.size(); ++i) {
    if (StringUtils::equalsIgnoreCase(m_actions[i].name, name)) {
      return static_cast<InputActionId>(i);
    }
  }
  return INVALID_ACTION;
}

/*
 */
const String&
InputManager::getActionName(InputActionId action) const
{
  CH_ASSERT(action < m_actions.size());
  return m_actions[action].name;
}

/*
 */
InputActionType
InputManager::getActionType(InputActionId action) const
{
  CH_ASSERT(action < m_actions.size());
  return m_actions[action].type;
}

/*
 */
const String&
InputManager::getActionContext(InputActionId action) const
{
  CH_ASSERT(action < m_actions.size());
  return m_contexts[m_actions[action].context].name;
}

/*
 */
Span<const InputBinding>
InputManager::getBindings(InputActionId action) const
{
  if (action >= m_actions.size()) {
    return {};
  }
  return m_actions[action].bindings;
}

/*
 */
Span<const InputBinding>
InputManager::getDefaultBindings(InputActionId action) const
{
  if (action >= m_actions.size()) {
    return {};
  }
  return m_actions[action].defaultBindings;
}

/*
 */
bool
InputManager::setBinding(InputActionId action, uint32 index, const InputBinding& binding)
{
  if (action >= m_actions.size()) {
    return false;
  }
  Action& target = m_actions[action];
  if (index > target.bindings.size() || !isBindingValid(binding, target.type)) {
    return false;
  }
  if (index == target.bindings.size()) {
    target.bindings.push_back(binding);
  }
  else {
    target.bindings[index] = binding;
  }
  return true;
}

/*
 */
bool
InputManager::removeBinding(InputActionId action, uint32 index)
{
  if (action >= m_actions.size() || index >= m_actions[action].bindings.size()) {
    return false;
  }
  Vector<InputBinding>& bindings = m_actions[action].bindings;
  bindings.erase(bindings.begin() + index);
  return true;
}

/*
 */
void
InputManager::resetBindings(InputActionId action)
{
  if (action < m_actions.size()) {
    m_actions[action].bindings = m_actions[action].defaultBindings;
  }
}

/*
 */
void
InputManager::resetAllBindings()
{
  for (Action& action : m_actions) {
    action.bindings = action.defaultBindings;
  }
}

/*
 */
void
InputManager::findActionsUsingKey(InputKey key, Vector<InputActionId>& outActions) const
{
  for (SIZE_T i = 0; i < m_actions.size(); ++i) {
    for (const InputBinding& binding : m_actions[i].bindings) {
      if (binding.key == key) {
        outActions.push_back(static_cast<InputActionId>(i));
        break;
      }
    }
  }
}

/*
 */
const InputManager::ActionState*
InputManager::findState(InputActionId action, uint32 player) const noexcept
{
  if (player >= m_playerCount || action >= m_players[player].actions.size()) {
    return nullptr;
  }
  return &m_players[player].actions[action];
}

/*
 */
bool
InputManager::isActionDown(InputActionId action, uint32 player) const noexcept
{
  const ActionState* state = findState(action, player);
  return state && state->bDown;
}

/*
 */
bool
InputManager::wasActionPressed(InputActionId action, uint32 player) const noexcept
{
  const ActionState* state = findState(action, player);
  return state && (m_bInFixedStep ? state->bPressedSinceFixed : state->bPressed);
}

/*
 */
bool
InputManager::wasActionReleased(InputActionId action, uint32 player) const noexcept
{
  const ActionState* state = findState(action, player);
  return state && (m_bInFixedStep ? state->bReleasedSinceFixed : state->bReleased);
}

/*
 */
float
InputManager::getActionValue(InputActionId action, uint32 player) const noexcept
{
  const ActionState* state = findState(action, player);
  return state ? state->value.x : 0.0f;
}

/*
 */
Vector2
InputManager::getActionValue2D(InputActionId action, uint32 player) const noexcept
{
  const ActionState* state = findState(action, player);
  return state ? state->value : Vector2::ZERO;
}

/*
 */
uint16
InputManager::findOrAddContext(StringView name)
{
  for (SIZE_T i = 0; i < m_contexts.size(); ++i) {
    if (StringUtils::equalsIgnoreCase(m_contexts[i].name, name)) {
      return static_cast<uint16>(i);
    }
  }
  m_contexts.push_back({.name = String(name), .bEnabled = true});
  return static_cast<uint16>(m_contexts.size() - 1);
}

/*
 */
void
InputManager::setContextEnabled(StringView context, bool bEnabled)
{
  m_contexts[findOrAddContext(context)].bEnabled = bEnabled;
}

/*
 */
bool
InputManager::isContextEnabled(StringView context) const noexcept
{
  for (const Context& entry : m_contexts) {
    if (StringUtils::equalsIgnoreCase(entry.name, context)) {
      return entry.bEnabled;
    }
  }
  return true;
}

/*
 */
void
InputManager::setGameInputEnabled(bool bEnabled)
{
  if (m_bGameInputEnabled == bEnabled) {
    return;
  }
  m_bGameInputEnabled = bEnabled;
  applyCursorMode();
}

/*
 */
void
InputManager::clearActionStates()
{
  for (Player& player : m_players) {
    for (ActionState& state : player.actions) {
      state = ActionState{};
    }
  }
}

/*
 */
void
InputManager::resetGameState()
{
  for (Context& context : m_contexts) {
    context.bEnabled = true;
  }
  m_keyCapture = nullptr;
  m_capturedKey = InputKey::None;
  m_cursorMode = CursorMode::Normal;
  applyCursorMode();
  const uint32 connected = m_state.getConnectedGamepads();
  for (uint32 slot = 0; slot < InputState::MAX_GAMEPADS; ++slot) {
    if (connected & (1u << slot)) {
      InputPlatform::rumbleGamepad(slot, 0.0f, 0.0f, 0);
    }
  }
  clearActionStates();
}

/*
 */
void
InputManager::setPlayerCount(uint32 count)
{
  m_playerCount = Math::clamp(count, 1u, MAX_PLAYERS);
}

/*
 */
void
InputManager::setPlayerDevices(uint32 player, bool bKeyboardMouse, uint32 gamepadMask)
{
  if (player >= MAX_PLAYERS) {
    return;
  }
  m_players[player].bKeyboardMouse = bKeyboardMouse;
  m_players[player].gamepadMask = gamepadMask & InputState::ALL_GAMEPADS;
}

/*
 */
void
InputManager::captureNextKey(Function<void(InputKey)> callback)
{
  m_keyCapture = std::move(callback);
}

/*
 */
void
InputManager::cancelKeyCapture()
{
  m_keyCapture = nullptr;
}

/*
 */
int32
InputManager::getPlayerGamepad(uint32 player) const noexcept
{
  if (player >= m_playerCount) {
    return InputState::NO_GAMEPAD;
  }
  const uint32 slots = m_players[player].gamepadMask & m_state.getConnectedGamepads();
  const int32 last = m_state.getLastGamepad();
  if (last != InputState::NO_GAMEPAD && (slots & (1u << last))) {
    return last;
  }
  for (uint32 slot = 0; slot < InputState::MAX_GAMEPADS; ++slot) {
    if (slots & (1u << slot)) {
      return static_cast<int32>(slot);
    }
  }
  return InputState::NO_GAMEPAD;
}

/*
 */
bool
InputManager::rumble(uint32 player, float lowFrequency, float highFrequency,
                     uint32 milliseconds)
{
  const int32 slot = getPlayerGamepad(player);
  if (slot == InputState::NO_GAMEPAD) {
    return false;
  }
  return InputPlatform::rumbleGamepad(static_cast<uint32>(slot),
                                      Math::clamp(lowFrequency, 0.0f, 1.0f),
                                      Math::clamp(highFrequency, 0.0f, 1.0f), milliseconds);
}

/*
 */
void
InputManager::setCursorMode(CursorMode mode)
{
  m_cursorMode = mode;
  applyCursorMode();
}

/*
 */
void
InputManager::setWindow(DisplaySurface* window)
{
  m_window = window;
  m_appliedCursorMode = CursorMode::Normal;
  applyCursorMode();
}

/*
 */
void
InputManager::applyCursorMode()
{
  const CursorMode mode = m_bGameInputEnabled ? m_cursorMode : CursorMode::Normal;
  if (!m_window || mode == m_appliedCursorMode) {
    return;
  }
  InputPlatform::setCursorMode(*m_window, mode);
  m_appliedCursorMode = mode;
}

/*
 */
String
InputManager::getKeyDisplayName(InputKey key, uint32 player) const
{
  switch (InputKeyUtils::getDevice(key)) {
  case InputDevice::Keyboard: {
    String name = InputPlatform::getKeyboardKeyName(key);
    if (!name.empty()) {
      return name;
    }
  } break;

  case InputDevice::Gamepad: {
    const int32 slot = getPlayerGamepad(player);
    const GamepadType type = slot == InputState::NO_GAMEPAD
                                 ? GamepadType::Unknown
                                 : m_state.getGamepad(static_cast<uint32>(slot)).type;
    const StringView label = getGamepadButtonLabel(key, type);
    if (!label.empty()) {
      return String(label);
    }
  } break;

  default:
    break;
  }
  return makeReadableName(InputKeyUtils::getName(key));
}

/*
 */
bool
InputManager::parseBinding(StringView text, InputBinding& outBinding) noexcept
{
  InputBinding binding;
  text = StringUtils::trimView(text);

  const SIZE_T star = text.find('*');
  if (star != StringView::npos) {
    const StringView scaleText = StringUtils::trimView(text.substr(star + 1));
    const ANSICHAR* end = scaleText.data() + scaleText.size();
    const std::from_chars_result result =
        std::from_chars(scaleText.data(), end, binding.scale);
    if (scaleText.empty() || result.ec != std::errc() || result.ptr != end) {
      return false;
    }
    text = StringUtils::trimView(text.substr(0, star));
  }

  const SIZE_T colon = text.find(':');
  if (colon != StringView::npos) {
    const StringView targetText = StringUtils::trimView(text.substr(colon + 1));
    if (targetText == "+X" || targetText == "X" || targetText == "+") {
      binding.target = InputAxisTarget::PositiveX;
    }
    else if (targetText == "-X" || targetText == "-") {
      binding.target = InputAxisTarget::NegativeX;
    }
    else if (targetText == "+Y" || targetText == "Y") {
      binding.target = InputAxisTarget::PositiveY;
    }
    else if (targetText == "-Y") {
      binding.target = InputAxisTarget::NegativeY;
    }
    else {
      return false;
    }
    text = StringUtils::trimView(text.substr(0, colon));
  }

  binding.key = InputKeyUtils::fromName(text);
  if (binding.key == InputKey::None) {
    return false;
  }
  outBinding = binding;
  return true;
}

/*
 */
String
InputManager::bindingToString(const InputBinding& binding, InputActionType type)
{
  String text(InputKeyUtils::getName(binding.key));
  switch (binding.target) {
  case InputAxisTarget::PositiveX:
    text += type == InputActionType::Axis1D ? ":+" : ":+X";
    break;
  case InputAxisTarget::NegativeX:
    text += type == InputActionType::Axis1D ? ":-" : ":-X";
    break;
  case InputAxisTarget::PositiveY:
    text += ":+Y";
    break;
  case InputAxisTarget::NegativeY:
    text += ":-Y";
    break;
  default:
    break;
  }
  if (binding.scale != 1.0f) {
    ANSICHAR buffer[StringUtils::MAX_FLOAT_CHARS];
    text += '*';
    text += StringUtils::toChars(buffer, binding.scale);
  }
  return text;
}

/*
 */
bool
InputManager::isBindingValid(const InputBinding& binding, InputActionType type) noexcept
{
  const InputKeyType keyType = InputKeyUtils::getType(binding.key);
  if (keyType == InputKeyType::None) {
    return false;
  }

  switch (type) {
  case InputActionType::Button:
    return binding.target == InputAxisTarget::None;

  case InputActionType::Axis1D:
    return keyType != InputKeyType::Axis2D
           && (binding.target == InputAxisTarget::None
               || binding.target == InputAxisTarget::PositiveX
               || binding.target == InputAxisTarget::NegativeX);

  case InputActionType::Axis2D:
    if (keyType == InputKeyType::Axis2D) {
      return binding.target == InputAxisTarget::None;
    }
    return binding.target != InputAxisTarget::None;
  }
  return false;
}

} // namespace chEngineSDK
