/************************************************************************/
/**
 * @file chProjectSettingsUI.cpp
 * @author AccelMR
 * @date 2026/10/22
 * @brief Window that edits the settings of the open project.
 */
/************************************************************************/
#include "chProjectSettingsUI.h"

#include "chConfigFile.h"
#include "chConsoleVariable.h"
#include "chEngineConfig.h"
#include "chEnginePaths.h"
#include "chInputManager.h"
#include "chLogger.h"
#include "chStringUtils.h"

#include "imgui.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"

namespace chEngineSDK {
CH_LOG_DECLARE_STATIC(ProjectSettingsLog, All);

namespace {
ConsoleVariable<bool> g_cvarShowProjectSettings("Editor.ShowProjectSettings",
                                                false,
                                                "Shows the Project Settings window.",
                                                "ShowProjectSettings");

// Size the window gets the first time it opens; ImGui remembers it after.
constexpr float kWindowWidth = 760.0f;
constexpr float kWindowHeight = 560.0f;
constexpr float kSectionListWidth = 150.0f;
constexpr float kFieldLabelWidth = 90.0f;
constexpr float kTargetColumnWidth = 70.0f;
constexpr float kScaleColumnWidth = 70.0f;

constexpr ImVec4 kWarningColor(1.0f, 0.75f, 0.3f, 1.0f);
constexpr ImVec4 kErrorColor(1.0f, 0.4f, 0.4f, 1.0f);

constexpr uint32 kNone = ~0u;
constexpr StringView kNewActionName = "NewAction";

enum class SettingsSection : uint8 { Input, COUNT };

const ANSICHAR* const kSectionNames[] = {ICON_FA_GAMEPAD "  Input"};
static_assert(sizeof(kSectionNames) / sizeof(kSectionNames[0])
              == static_cast<SIZE_T>(SettingsSection::COUNT));

struct BindingEntry
{
  InputBinding binding;
  // The text of a key the file has but that does not parse, kept so saving does not drop it.
  String unreadText;
};

struct ActionEntry
{
  String name;
  InputActionType type = InputActionType::Button;
  String context;
  Vector<BindingEntry> bindings;
  // The same action in the InputManager, to show the keys the player changed.
  InputActionId id = InputManager::INVALID_ACTION;
};

struct SettingsState
{
  SettingsSection section = SettingsSection::Input;
  bool bFocus = false;
  bool bLoaded = false;

  // What <Project>/Config/Input.ini holds, in its order; written whole after every change.
  Vector<ActionEntry> actions;
  // Every context the actions name, Default first, for the context combo.
  Vector<String> contexts;

  // The name field shows the text being typed only for the action that has it.
  uint32 nameEditAction = kNone;
  ANSICHAR nameBuffer[64] = "";
  ANSICHAR newContextBuffer[64] = "";
  ANSICHAR keySearchBuffer[64] = "";

  // A capture in flight: the binding it goes to and the key it answered with. The serial
  // tells the answer of the last capture apart from one that was replaced.
  uint32 captureAction = kNone;
  uint32 captureBinding = kNone;
  uint32 captureSerial = 0;
  InputKey capturedKey = InputKey::None;
  bool bCaptureAnswered = false;

  // Changes to the list itself wait until it is drawn.
  uint32 deleteAction = kNone;
  uint32 removeAction = kNone;
  uint32 removeBinding = kNone;

  // Reused for the keys the player changed, built only for actions that show them.
  String playerKeys;
};

NODISCARD SettingsState&
getState()
{
  static SettingsState state;
  return state;
}

NODISCARD Path
getProjectBindingsPath()
{
  return EnginePaths::getProjectConfigDirectory().join(
      Path(String(InputManager::BINDINGS_FILE_NAME)));
}

NODISCARD Path
getPlayerBindingsPath()
{
  return EnginePaths::getConfigDirectory().join(Path(String(InputManager::BINDINGS_FILE_NAME)));
}

/*
 * Action and context names are read by scripts and written as Input.ini section names, so
 * they keep to letters, digits and underscores.
 */
NODISCARD bool
isValidName(StringView name)
{
  if (name.empty()) {
    return false;
  }
  for (const ANSICHAR character : name) {
    const bool bLetter =
        (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
    const bool bDigit = character >= '0' && character <= '9';
    if (!bLetter && !bDigit && character != '_') {
      return false;
    }
  }
  return true;
}

NODISCARD uint32
findActionEntry(const SettingsState& state, StringView name, uint32 ignored = kNone)
{
  for (uint32 i = 0; i < state.actions.size(); ++i) {
    if (i != ignored && StringUtils::equalsIgnoreCase(state.actions[i].name, name)) {
      return i;
    }
  }
  return kNone;
}

NODISCARD String
makeUniqueActionName(const SettingsState& state)
{
  String name(kNewActionName);
  ANSICHAR number[StringUtils::MAX_INTEGER_CHARS];
  for (uint32 suffix = 1; findActionEntry(state, name) != kNone; ++suffix) {
    name = kNewActionName;
    name += StringUtils::toChars(number, suffix);
  }
  return name;
}

void
rebuildContexts(SettingsState& state)
{
  state.contexts.clear();
  state.contexts.emplace_back(InputManager::DEFAULT_CONTEXT);
  for (const ActionEntry& action : state.actions) {
    bool bKnown = false;
    for (const String& context : state.contexts) {
      bKnown = bKnown || StringUtils::equalsIgnoreCase(context, action.context);
    }
    if (!bKnown) {
      state.contexts.push_back(action.context);
    }
  }
}

void
refreshActionIds(SettingsState& state)
{
  const InputManager& input = InputManager::instance();
  for (ActionEntry& action : state.actions) {
    action.id = input.findAction(action.name);
  }
}

/*
 * Only the project's own file: the engine's layer stays out, so nothing of it is copied
 * into the project when it is saved.
 */
void
loadActions(SettingsState& state)
{
  state.bLoaded = true;
  state.actions.clear();
  state.nameEditAction = kNone;

  ConfigFile file;
  file.load(getProjectBindingsPath());
  for (const ConfigFile::Section& section : file.getSections()) {
    if (section.name.empty()) {
      continue;
    }
    ActionEntry action;
    action.name = section.name;
    const String* typeText = file.getValue(section.name, "Type");
    if (!InputManager::parseActionType(typeText ? StringView(*typeText) : StringView(),
                                       action.type)) {
      CH_LOG_WARNING(ProjectSettingsLog,
                     "Input action {0}: unknown type \"{1}\"; shown as Button.", section.name,
                     *typeText);
    }
    const String* contextText = file.getValue(section.name, "Context");
    action.context = contextText && !contextText->empty()
                         ? *contextText
                         : String(InputManager::DEFAULT_CONTEXT);

    if (const String* keysText = file.getValue(section.name, "Keys")) {
      StringView keys = *keysText;
      while (!keys.empty()) {
        const SIZE_T comma = keys.find(',');
        const StringView item = StringUtils::trimView(keys.substr(0, comma));
        keys = comma == StringView::npos ? StringView() : keys.substr(comma + 1);
        if (item.empty()) {
          continue;
        }
        BindingEntry entry;
        if (!InputManager::parseBinding(item, entry.binding)) {
          entry.unreadText = item;
        }
        action.bindings.push_back(std::move(entry));
      }
    }
    state.actions.push_back(std::move(action));
  }

  rebuildContexts(state);
  refreshActionIds(state);
}

/*
 * Writes the whole file again (its comments are lost) and loads the actions from every
 * layer, so the change reaches the game at once, also during Play.
 */
void
saveActions(SettingsState& state)
{
  ConfigFile file;
  String keys;
  for (const ActionEntry& action : state.actions) {
    keys.clear();
    for (const BindingEntry& entry : action.bindings) {
      // A row whose key was never picked has nothing to write yet.
      if (entry.unreadText.empty() && entry.binding.key == InputKey::None) {
        continue;
      }
      if (!keys.empty()) {
        keys += ", ";
      }
      keys += entry.unreadText.empty() ? InputManager::bindingToString(entry.binding, action.type)
                                       : entry.unreadText;
    }
    file.setValue(action.name, "Type", InputManager::getActionTypeName(action.type));
    file.setValue(action.name, "Keys", keys);
    if (!StringUtils::equalsIgnoreCase(action.context, InputManager::DEFAULT_CONTEXT)) {
      file.setValue(action.name, "Context", action.context);
    }
  }

  const Path path = getProjectBindingsPath();
  if (!file.save(path)) {
    CH_LOG_ERROR(ProjectSettingsLog, "Could not save the input actions to {0}.", path);
    return;
  }
  InputManager::instance().loadBindings();
  rebuildContexts(state);
  refreshActionIds(state);
}

/*
 * Removes the keys the player saved for one action, or for all of them when the name is
 * empty, from /Saved/Config/Input.ini.
 */
void
clearPlayerBindings(SettingsState& state, StringView actionName)
{
  ConfigFile kept;
  if (!actionName.empty()) {
    ConfigFile saved;
    saved.load(getPlayerBindingsPath());
    for (const ConfigFile::Section& section : saved.getSections()) {
      if (StringUtils::equalsIgnoreCase(section.name, actionName)) {
        continue;
      }
      for (const ConfigFile::Entry& entry : section.entries) {
        kept.setValue(section.name, entry.key, entry.value);
      }
    }
  }

  const Path path = getPlayerBindingsPath();
  if (!kept.save(path)) {
    CH_LOG_ERROR(ProjectSettingsLog, "Could not save the player's input bindings to {0}.",
                 path);
    return;
  }
  InputManager::instance().loadBindings();
  refreshActionIds(state);
}

NODISCARD bool
hasPlayerBindings(const InputManager& input, InputActionId id)
{
  if (id == InputManager::INVALID_ACTION) {
    return false;
  }
  const Span<const InputBinding> current = input.getBindings(id);
  const Span<const InputBinding> defaults = input.getDefaultBindings(id);
  if (current.size() != defaults.size()) {
    return true;
  }
  for (SIZE_T i = 0; i < current.size(); ++i) {
    if (current[i] != defaults[i]) {
      return true;
    }
  }
  return false;
}

/*
 * After a change of type or key, moves targets whose meaning is clear (a button needs
 * none, an Axis1D action only has X); the rest is left for the user and shown in red.
 */
void
fixTarget(InputBinding& binding, InputActionType type)
{
  const InputKeyType keyType = InputKeyUtils::getType(binding.key);
  if (type == InputActionType::Button
      || (type == InputActionType::Axis2D && keyType == InputKeyType::Axis2D)) {
    binding.target = InputAxisTarget::None;
  }
  else if (type == InputActionType::Axis1D) {
    if (binding.target == InputAxisTarget::PositiveY) {
      binding.target = InputAxisTarget::PositiveX;
    }
    else if (binding.target == InputAxisTarget::NegativeY) {
      binding.target = InputAxisTarget::NegativeX;
    }
  }
}

void
cancelCapture(SettingsState& state)
{
  if (state.captureAction != kNone && !state.bCaptureAnswered && InputManager::isStarted()) {
    InputManager::instance().cancelKeyCapture();
  }
  state.captureAction = kNone;
  state.captureBinding = kNone;
  state.bCaptureAnswered = false;
}

void
startCapture(SettingsState& state, uint32 actionIndex, uint32 bindingIndex)
{
  state.captureAction = actionIndex;
  state.captureBinding = bindingIndex;
  state.bCaptureAnswered = false;
  const uint32 serial = ++state.captureSerial;
  InputManager::instance().captureNextKey([serial](InputKey key) {
    SettingsState& current = getState();
    if (current.captureSerial == serial && current.captureAction != kNone) {
      current.capturedKey = key;
      current.bCaptureAnswered = true;
    }
  });
}

/*
 * Runs after the list is drawn, so the capture button stays disabled on the frame the key
 * arrives and that key cannot press it again.
 */
void
applyCapturedKey(SettingsState& state)
{
  if (state.captureAction == kNone) {
    return;
  }
  if (!state.bCaptureAnswered) {
    // Cancelled by someone else, such as Stop resetting the game state.
    if (!InputManager::instance().isCapturingKey()) {
      cancelCapture(state);
    }
    return;
  }

  const uint32 actionIndex = state.captureAction;
  const uint32 bindingIndex = state.captureBinding;
  const InputKey key = state.capturedKey;
  state.captureAction = kNone;
  state.captureBinding = kNone;
  state.bCaptureAnswered = false;
  if (key == InputKey::Escape || actionIndex >= state.actions.size()
      || bindingIndex >= state.actions[actionIndex].bindings.size()) {
    return;
  }
  ActionEntry& action = state.actions[actionIndex];
  BindingEntry& entry = action.bindings[bindingIndex];
  entry.unreadText.clear();
  entry.binding.key = key;
  fixTarget(entry.binding, action.type);
  saveActions(state);
}

void
renderKeyCombo(SettingsState& state, ActionEntry& action, BindingEntry& entry)
{
  const InputKey current = entry.binding.key;
  // Key names are views into a table, not terminated strings.
  ANSICHAR label[64];
  StringUtils::copyToBuffer(
      label, current == InputKey::None ? StringView("Pick a key") : InputKeyUtils::getName(current));

  ImGui::SetNextItemWidth(-FLT_MIN);
  if (!ImGui::BeginCombo("##key", label, ImGuiComboFlags_HeightLarge)) {
    return;
  }
  if (ImGui::IsWindowAppearing()) {
    state.keySearchBuffer[0] = '\0';
    ImGui::SetKeyboardFocusHere();
  }
  ImGui::SetNextItemWidth(-FLT_MIN);
  ImGui::InputTextWithHint("##search", ICON_FA_SEARCH "  Search", state.keySearchBuffer,
                           sizeof(state.keySearchBuffer));
  const StringView search(state.keySearchBuffer);

  InputDevice lastDevice = InputDevice::None;
  for (uint32 i = 1; i < InputKeyUtils::KEY_COUNT; ++i) {
    const InputKey key = static_cast<InputKey>(i);
    const StringView name = InputKeyUtils::getName(key);
    if (!search.empty() && !StringUtils::containsIgnoreCase(name, search)) {
      continue;
    }
    const InputDevice device = InputKeyUtils::getDevice(key);
    if (search.empty() && device != lastDevice) {
      lastDevice = device;
      ImGui::SeparatorText(device == InputDevice::Keyboard ? "Keyboard"
                           : device == InputDevice::Mouse  ? "Mouse"
                                                           : "Gamepad");
    }
    ImGui::PushID(static_cast<int32>(i));
    StringUtils::copyToBuffer(label, name);
    if (ImGui::Selectable(label, key == current)) {
      entry.unreadText.clear();
      entry.binding.key = key;
      fixTarget(entry.binding, action.type);
      saveActions(state);
    }
    ImGui::PopID();
  }
  ImGui::EndCombo();
}

void
renderTargetCombo(SettingsState& state, const ActionEntry& action, BindingEntry& entry)
{
  const InputKeyType keyType = InputKeyUtils::getType(entry.binding.key);
  if (action.type == InputActionType::Button
      || (action.type == InputActionType::Axis2D && keyType == InputKeyType::Axis2D)) {
    ImGui::TextDisabled("-");
    return;
  }

  struct TargetChoice
  {
    InputAxisTarget target;
    const ANSICHAR* label;
  };
  constexpr TargetChoice kAxis1DChoices[] = {{InputAxisTarget::PositiveX, "+"},
                                             {InputAxisTarget::NegativeX, "-"}};
  constexpr TargetChoice kAxis2DChoices[] = {{InputAxisTarget::PositiveX, "+X"},
                                             {InputAxisTarget::NegativeX, "-X"},
                                             {InputAxisTarget::PositiveY, "+Y"},
                                             {InputAxisTarget::NegativeY, "-Y"}};
  const bool bAxis1D = action.type == InputActionType::Axis1D;
  const Span<const TargetChoice> choices = bAxis1D ? Span<const TargetChoice>(kAxis1DChoices)
                                                   : Span<const TargetChoice>(kAxis2DChoices);

  // An Axis1D binding without a target goes up, like "+".
  const InputAxisTarget shown = bAxis1D && entry.binding.target == InputAxisTarget::None
                                    ? InputAxisTarget::PositiveX
                                    : entry.binding.target;
  const ANSICHAR* preview = "Pick";
  for (const TargetChoice& choice : choices) {
    if (choice.target == shown) {
      preview = choice.label;
    }
  }

  ImGui::SetNextItemWidth(-FLT_MIN);
  if (ImGui::BeginCombo("##target", preview)) {
    for (const TargetChoice& choice : choices) {
      if (ImGui::Selectable(choice.label, choice.target == shown)) {
        entry.binding.target = choice.target;
        saveActions(state);
      }
    }
    ImGui::EndCombo();
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Where the key puts its value in the action.");
  }
}

void
renderBindingRow(SettingsState& state, uint32 actionIndex, uint32 bindingIndex)
{
  ActionEntry& action = state.actions[actionIndex];
  BindingEntry& entry = action.bindings[bindingIndex];
  ImGui::PushID(static_cast<int32>(bindingIndex));
  ImGui::TableNextRow();

  ImGui::TableNextColumn();
  if (!entry.unreadText.empty()) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(kErrorColor, ICON_FA_EXCLAMATION_TRIANGLE "  %s",
                       entry.unreadText.c_str());
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("This key cannot be read; it is kept as written. Pick a key to "
                        "replace it.");
    }
    ImGui::TableNextColumn();
    ImGui::TableNextColumn();
  }
  else {
    const bool bValid = InputManager::isBindingValid(entry.binding, action.type);
    if (!bValid) {
      ImGui::PushStyleColor(ImGuiCol_Text, kErrorColor);
    }
    renderKeyCombo(state, action, entry);
    if (!bValid) {
      ImGui::PopStyleColor();
      if (ImGui::IsItemHovered() && entry.binding.key != InputKey::None) {
        ImGui::SetTooltip("This key does not fit the action's type: a button or one-axis key "
                          "in an Axis2D action needs a target, and a 2D key only fits an "
                          "Axis2D action. The game skips it.");
      }
    }

    ImGui::TableNextColumn();
    renderTargetCombo(state, action, entry);

    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::DragFloat("##scale", &entry.binding.scale, 0.01f, 0.0f, 0.0f, "x%.3g");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      saveActions(state);
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Multiplies the key's value.");
    }
  }

  ImGui::TableNextColumn();
  const bool bCapturing =
      state.captureAction == actionIndex && state.captureBinding == bindingIndex;
  ImGui::BeginDisabled(bCapturing);
  if (ImGui::SmallButton(bCapturing ? ICON_FA_KEYBOARD "  Press a key..." : ICON_FA_KEYBOARD)) {
    cancelCapture(state);
    startCapture(state, actionIndex, bindingIndex);
  }
  ImGui::EndDisabled();
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::SetTooltip(bCapturing ? "Press the key, mouse button or gamepad button to use. "
                                   "Esc cancels."
                                 : "Use the next key pressed.");
  }
  ImGui::SameLine();
  if (ImGui::SmallButton(ICON_FA_TRASH)) {
    state.removeAction = actionIndex;
    state.removeBinding = bindingIndex;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Remove this key.");
  }
  ImGui::PopID();
}

void
renderNameField(SettingsState& state, uint32 actionIndex)
{
  ActionEntry& action = state.actions[actionIndex];
  ANSICHAR shownName[64];
  ANSICHAR* buffer = shownName;
  if (state.nameEditAction == actionIndex) {
    buffer = state.nameBuffer;
  }
  else {
    StringUtils::copyToBuffer(shownName, action.name);
  }

  ImGui::SetNextItemWidth(-FLT_MIN);
  ImGui::InputText("##name", buffer, sizeof(shownName));
  if (ImGui::IsItemActivated()) {
    state.nameEditAction = actionIndex;
    StringUtils::copyToBuffer(state.nameBuffer, action.name);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Letters, digits and _. Scripts that look the action up by name must "
                      "use the new name.");
  }
  if (!ImGui::IsItemDeactivated()) {
    return;
  }

  state.nameEditAction = kNone;
  const StringView name = StringUtils::trimView(StringView(state.nameBuffer));
  if (name == action.name) {
    return;
  }
  if (!isValidName(name)) {
    CH_LOG_WARNING(ProjectSettingsLog,
                   "\"{0}\" cannot name an input action: use letters, digits and _.", name);
    return;
  }
  if (findActionEntry(state, name, actionIndex) != kNone) {
    CH_LOG_WARNING(ProjectSettingsLog, "There is already an input action named {0}.", name);
    return;
  }
  action.name = name;
  saveActions(state);
}

void
renderContextCombo(SettingsState& state, ActionEntry& action)
{
  ImGui::SetNextItemWidth(-FLT_MIN);
  if (!ImGui::BeginCombo("##context", action.context.c_str())) {
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Game code turns contexts on and off; an action reads as up while "
                        "its context is off.");
    }
    return;
  }
  if (ImGui::IsWindowAppearing()) {
    state.newContextBuffer[0] = '\0';
  }
  for (const String& context : state.contexts) {
    if (ImGui::Selectable(context.c_str(),
                          StringUtils::equalsIgnoreCase(context, action.context))) {
      action.context = context;
      saveActions(state);
      break;
    }
  }
  ImGui::Separator();
  ImGui::SetNextItemWidth(-FLT_MIN);
  if (ImGui::InputTextWithHint("##newContext", "New context (Enter)", state.newContextBuffer,
                               sizeof(state.newContextBuffer),
                               ImGuiInputTextFlags_EnterReturnsTrue)) {
    const StringView name = StringUtils::trimView(StringView(state.newContextBuffer));
    if (isValidName(name)) {
      action.context = name;
      saveActions(state);
      ImGui::CloseCurrentPopup();
    }
    else {
      CH_LOG_WARNING(ProjectSettingsLog,
                     "\"{0}\" cannot name an input context: use letters, digits and _.", name);
    }
  }
  ImGui::EndCombo();
}

void
renderAction(SettingsState& state, const InputManager& input, uint32 actionIndex)
{
  ActionEntry& action = state.actions[actionIndex];
  ImGui::PushID(static_cast<int32>(actionIndex));

  const bool bOpen = ImGui::TreeNodeEx("##action",
                                       ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen
                                           | ImGuiTreeNodeFlags_SpanAvailWidth,
                                       "%s  (%s)", action.name.c_str(),
                                       InputManager::getActionTypeName(action.type).data());
  if (ImGui::BeginPopupContextItem("##actionMenu")) {
    if (ImGui::MenuItem(ICON_FA_TRASH "  Delete Action")) {
      state.deleteAction = actionIndex;
    }
    ImGui::EndPopup();
  }
  if (!bOpen) {
    ImGui::PopID();
    return;
  }

  if (ImGui::BeginTable("##fields", 2)) {
    ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed, kFieldLabelWidth);
    ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Name");
    ImGui::TableNextColumn();
    renderNameField(state, actionIndex);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Type");
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##type", InputManager::getActionTypeName(action.type).data())) {
      constexpr InputActionType kTypes[] = {InputActionType::Button, InputActionType::Axis1D,
                                            InputActionType::Axis2D};
      for (const InputActionType type : kTypes) {
        if (ImGui::Selectable(InputManager::getActionTypeName(type).data(),
                              type == action.type)) {
          action.type = type;
          for (BindingEntry& entry : action.bindings) {
            fixTarget(entry.binding, type);
          }
          saveActions(state);
        }
      }
      ImGui::EndCombo();
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Context");
    ImGui::TableNextColumn();
    renderContextCombo(state, action);
    ImGui::EndTable();
  }

  if (hasPlayerBindings(input, action.id)) {
    state.playerKeys.clear();
    for (const InputBinding& binding : input.getBindings(action.id)) {
      if (!state.playerKeys.empty()) {
        state.playerKeys += ", ";
      }
      state.playerKeys += InputManager::bindingToString(binding, action.type);
    }
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(kWarningColor, ICON_FA_EXCLAMATION_TRIANGLE "  Changed by the player to: %s",
                       state.playerKeys.empty() ? "(no keys)" : state.playerKeys.c_str());
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Saved in Saved/Config/Input.ini (a rebinding menu, or a script that "
                        "saved its bindings). These keys win over the ones below.");
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear")) {
      clearPlayerBindings(state, action.name);
    }
  }

  constexpr ImGuiTableFlags kKeyTableFlags =
      ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH;
  if (ImGui::BeginTable("##keys", 4, kKeyTableFlags)) {
    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Target", ImGuiTableColumnFlags_WidthFixed, kTargetColumnWidth);
    ImGui::TableSetupColumn("Scale", ImGuiTableColumnFlags_WidthFixed, kScaleColumnWidth);
    ImGui::TableSetupColumn("##buttons", ImGuiTableColumnFlags_WidthFixed);
    for (uint32 i = 0; i < action.bindings.size(); ++i) {
      renderBindingRow(state, actionIndex, i);
    }
    ImGui::EndTable();
  }
  if (action.bindings.empty()) {
    ImGui::TextDisabled("No keys.");
  }
  if (ImGui::Button(ICON_FA_PLUS "  Add Key")) {
    action.bindings.push_back(BindingEntry{});
    cancelCapture(state);
    startCapture(state, actionIndex, static_cast<uint32>(action.bindings.size() - 1));
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Adds a key and waits for it to be pressed (Esc to pick it from the "
                      "list instead).");
  }

  ImGui::TreePop();
  ImGui::PopID();
}

/*
 * Structural changes run after the list is drawn; they move indices, so a capture in flight
 * is dropped.
 */
void
applyPendingChanges(SettingsState& state)
{
  if (state.deleteAction != kNone) {
    const uint32 index = state.deleteAction;
    state.deleteAction = kNone;
    if (index < state.actions.size()) {
      cancelCapture(state);
      state.nameEditAction = kNone;
      state.actions.erase(state.actions.begin() + index);
      saveActions(state);
    }
  }
  if (state.removeAction != kNone) {
    const uint32 actionIndex = state.removeAction;
    const uint32 bindingIndex = state.removeBinding;
    state.removeAction = kNone;
    state.removeBinding = kNone;
    if (actionIndex < state.actions.size()
        && bindingIndex < state.actions[actionIndex].bindings.size()) {
      cancelCapture(state);
      Vector<BindingEntry>& bindings = state.actions[actionIndex].bindings;
      bindings.erase(bindings.begin() + bindingIndex);
      saveActions(state);
    }
  }
}

void
renderInputSection(SettingsState& state)
{
  const InputManager& input = InputManager::instance();

  ImGui::TextWrapped("The actions of this project and their keys, saved in Config/Input.ini "
                     "at every change. Scripts read them with Input.findAction(name).");
  if (ImGui::Button(ICON_FA_PLUS "  Add Action")) {
    ActionEntry action;
    action.name = makeUniqueActionName(state);
    action.context = InputManager::DEFAULT_CONTEXT;
    state.actions.push_back(std::move(action));
    saveActions(state);
  }
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_SYNC "  Reload")) {
    cancelCapture(state);
    loadActions(state);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Reads Config/Input.ini again, after it was changed outside the "
                      "editor.");
  }

  uint32 playerChanges = 0;
  for (const ActionEntry& action : state.actions) {
    playerChanges += hasPlayerBindings(input, action.id) ? 1 : 0;
  }
  if (playerChanges > 0) {
    ImGui::TextColored(kWarningColor,
                       ICON_FA_EXCLAMATION_TRIANGLE "  The player changed the keys of %u "
                                                    "action(s); those win in Play.",
                       playerChanges);
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear All")) {
      clearPlayerBindings(state, StringView());
    }
  }
  ImGui::Separator();

  if (state.actions.empty()) {
    ImGui::TextDisabled("No input actions yet.");
  }
  for (uint32 i = 0; i < state.actions.size(); ++i) {
    renderAction(state, input, i);
  }

  applyPendingChanges(state);
  applyCapturedKey(state);
}
} // namespace

/*
 */
void
ProjectSettingsUI::render()
{
  SettingsState& state = getState();
  if (!g_cvarShowProjectSettings.get()) {
    return;
  }
  if (!state.bLoaded) {
    loadActions(state);
  }

  ImGui::SetNextWindowSize(ImVec2(kWindowWidth, kWindowHeight), ImGuiCond_FirstUseEver);
  if (state.bFocus) {
    state.bFocus = false;
    ImGui::SetNextWindowFocus();
  }
  bool bVisible = true;
  if (ImGui::Begin(ICON_FA_COGS "  Project Settings###ProjectSettings", &bVisible)) {
    if (ImGui::BeginChild("##sections", ImVec2(kSectionListWidth, 0.0f),
                          ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX)) {
      for (uint32 i = 0; i < static_cast<uint32>(SettingsSection::COUNT); ++i) {
        const SettingsSection section = static_cast<SettingsSection>(i);
        if (ImGui::Selectable(kSectionNames[i], state.section == section)) {
          state.section = section;
        }
      }
    }
    ImGui::EndChild();

    ImGui::SameLine();
    if (ImGui::BeginChild("##section")) {
      switch (state.section) {
      case SettingsSection::Input:
        renderInputSection(state);
        break;
      default:
        break;
      }
    }
    ImGui::EndChild();
  }
  ImGui::End();

  if (!bVisible) {
    setWindowVisible(false);
  }
}

/*
 */
bool
ProjectSettingsUI::isWindowVisible()
{
  return g_cvarShowProjectSettings.get();
}

/*
 */
void
ProjectSettingsUI::setWindowVisible(bool bVisible)
{
  SettingsState& state = getState();
  if (bVisible) {
    state.bLoaded = false;
    state.bFocus = true;
  }
  else {
    cancelCapture(state);
  }
  g_cvarShowProjectSettings.set(bVisible, ConsoleVariableSource::Console);
  EngineConfig::setUserValue("Editor", "Editor", "ShowProjectSettings",
                             bVisible ? "true" : "false");
}

/*
 */
void
ProjectSettingsUI::shutDown()
{
  SettingsState& state = getState();
  cancelCapture(state);
  state.actions.clear();
  state.contexts.clear();
  state.bLoaded = false;
}

} // namespace chEngineSDK
