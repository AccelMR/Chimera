/************************************************************************/
/**
 * @file chScriptPropertiesUI.cpp
 * @author AccelMR
 * @date 2026/10/22
 * @brief Inspector rows for the properties a script lists.
 */
/************************************************************************/
#include "chScriptPropertiesUI.h"

#include "chAssetDragDrop.h"
#include "chEditorScene.h"
#include "chEditorSelection.h"
#include "chGameObject.h"
#include "chScene.h"
#include "chScriptAsset.h"
#include "chScriptComponent.h"
#include "chStringUtils.h"

#include "imgui.h"

#include "../../Content/Fonts/IconsFontAwsome5.h"

namespace chEngineSDK {
namespace {
// Shared by every string row: only one text field is typed in at a time.
ANSICHAR g_textBuffer[ScriptPropertyUtils::MAX_TEXT_LENGTH + 1] = "";

/*
 * Objects a property points at, so a row does not walk the scene every frame. An entry is
 * looked up again when its object died or left the scene; a missing one is looked up again
 * when the scene's revision changes.
 */
struct ObjectCache
{
  const Scene* scene = nullptr;
  uint32 revision = 0;
  UnorderedMap<UUID, WeakPtr<GameObject>> objects;
};

ObjectCache&
getObjectCache()
{
  static ObjectCache cache;
  return cache;
}

SPtr<GameObject>
findObject(const Scene& scene, const UUID& id)
{
  ObjectCache& cache = getObjectCache();
  const uint32 revision = EditorScene::getRevision();
  if (cache.scene != &scene || cache.revision != revision) {
    cache.scene = &scene;
    cache.revision = revision;
    cache.objects.clear();
  }
  WeakPtr<GameObject>& entry = cache.objects[id];
  SPtr<GameObject> gameObject = entry.lock();
  if (gameObject && gameObject->getScene() == &scene) {
    return gameObject;
  }
  gameObject = scene.findGameObject(id);
  entry = gameObject;
  return gameObject;
}

/*
 * True when the value changed.
 */
bool
renderNumberRow(const ScriptProperty& property, ScriptPropertyValue& value)
{
  const ANSICHAR* label = property.name.c_str();
  const bool bRange = property.minimum < property.maximum;
  if (value.type == ScriptPropertyType::Integer) {
    const int64 minimum = static_cast<int64>(property.minimum);
    const int64 maximum = static_cast<int64>(property.maximum);
    if (bRange) {
      return ImGui::SliderScalar(label, ImGuiDataType_S64, &value.integer, &minimum, &maximum);
    }
    return ImGui::DragScalar(label, ImGuiDataType_S64, &value.integer, 0.2f);
  }
  if (bRange) {
    return ImGui::SliderScalar(label, ImGuiDataType_Double, &value.number, &property.minimum,
                               &property.maximum, "%.3f");
  }
  return ImGui::DragScalar(label, ImGuiDataType_Double, &value.number, 0.05f, nullptr, nullptr,
                           "%.3f");
}

bool
renderVectorRow(const ScriptProperty& property, ScriptPropertyValue& value)
{
  const ANSICHAR* label = property.name.c_str();
  const bool bRange = property.minimum < property.maximum;
  const float minimum = bRange ? static_cast<float>(property.minimum) : 0.0f;
  const float maximum = bRange ? static_cast<float>(property.maximum) : 0.0f;
  float* components = value.components;
  switch (value.type) {
    case ScriptPropertyType::Vector2:
      return ImGui::DragFloat2(label, components, 0.05f, minimum, maximum, "%.3f");
    case ScriptPropertyType::Vector3:
      return ImGui::DragFloat3(label, components, 0.05f, minimum, maximum, "%.3f");
    case ScriptPropertyType::Vector4:
      return ImGui::DragFloat4(label, components, 0.05f, minimum, maximum, "%.3f");
    default:
      return ImGui::ColorEdit4(label, components, ImGuiColorEditFlags_Float);
  }
}

/*
 * A button with the object's name that takes objects dragged from the Scene Graph, and a
 * button to clear it.
 */
bool
renderObjectRow(const ScriptProperty& property,
                ScriptPropertyValue& value,
                const GameObject& owner)
{
  const Scene* scene = owner.getScene();
  const SPtr<GameObject> target =
      scene && !value.object.isNull() ? findObject(*scene, value.object) : nullptr;
  const ANSICHAR* targetName = "None (drop an object)";
  if (target) {
    targetName = target->getName().c_str();
  }
  else if (!value.object.isNull()) {
    targetName = "Missing object";
  }

  bool bChanged = false;
  const float clearWidth = value.object.isNull() ? 0.0f : ImGui::GetFrameHeight();
  const float width = ImGui::CalcItemWidth() - clearWidth;
  ImGui::Button(targetName, ImVec2(width > 1.0f ? width : 1.0f, 0.0f));
  if (!value.object.isNull()) {
    ImGui::SetItemTooltip("%s\nDrop another object from the Scene Graph to replace it",
                          targetName);
  }
  if (ImGui::BeginDragDropTarget()) {
    // Objects inside a prefab instance get new UUIDs every time the scene loads, so they
    // cannot be pointed at from outside it.
    const SPtr<GameObject> dragged = AssetDragDrop::getDraggedGameObject();
    const bool bAllowed = dragged && dragged->getScene() == scene &&
                          (!dragged->getOutermostPrefabInstance() ||
                           dragged->getOutermostPrefabInstance() == dragged.get());
    if (bAllowed) {
      if (const SPtr<GameObject> dropped = AssetDragDrop::acceptGameObject()) {
        value.object = dropped->getUUID();
        bChanged = true;
        // The Scene Graph selected the dragged row as the drag started; the Inspector goes
        // back to the object whose field took it.
        if (SPtr<GameObject> self = findObject(*scene, owner.getUUID())) {
          EditorSelection::setSelectedGameObject(self);
        }
      }
    }
    ImGui::EndDragDropTarget();
  }
  if (!value.object.isNull()) {
    ImGui::SameLine(0.0f, 0.0f);
    if (ImGui::Button(ICON_FA_TIMES, ImVec2(clearWidth, 0.0f))) {
      value.object = UUID::null();
      bChanged = true;
    }
    ImGui::SetItemTooltip("Clear");
  }
  ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
  ImGui::TextUnformatted(property.name.c_str());
  return bChanged;
}

/*
 * The widget of the property's type, editing value. True when it changed.
 */
bool
renderValue(const ScriptProperty& property, ScriptPropertyValue& value, const GameObject& owner)
{
  switch (value.type) {
    case ScriptPropertyType::Number:
    case ScriptPropertyType::Integer:
      return renderNumberRow(property, value);
    case ScriptPropertyType::Boolean:
      return ImGui::Checkbox(property.name.c_str(), &value.boolean);
    case ScriptPropertyType::String:
      StringUtils::copyToBuffer(g_textBuffer, value.text);
      if (ImGui::InputText(property.name.c_str(), g_textBuffer, sizeof(g_textBuffer))) {
        value.text = g_textBuffer;
        return true;
      }
      return false;
    case ScriptPropertyType::GameObject:
      return renderObjectRow(property, value, owner);
    default:
      return renderVectorRow(property, value);
  }
}

/*
 * Values saved for properties the script does not list (any more), or lists with another
 * type. They are kept until removed, so a typo in the script loses nothing.
 */
void
renderUnusedOverrides(ScriptComponent& component, const ScriptClassInfo& scriptClass)
{
  const ScriptPropertyOverride* toRemove = nullptr;
  for (const ScriptPropertyOverride& entry : component.getOverrides()) {
    const ScriptProperty* declared = nullptr;
    for (const ScriptProperty& property : scriptClass.properties) {
      if (property.name == entry.name) {
        declared = &property;
        break;
      }
    }
    if (declared && declared->defaultValue.type == entry.value.type) {
      continue;
    }

    ImGui::PushID(entry.name.c_str());
    if (ImGui::SmallButton(ICON_FA_TRASH)) {
      toRemove = &entry;
    }
    ImGui::SetItemTooltip("Remove the saved value");
    ImGui::SameLine();
    ImGui::TextDisabled("%s (unused %s)", entry.name.c_str(),
                        ScriptPropertyUtils::getTypeName(entry.value.type));
    if (declared) {
      ImGui::SetItemTooltip("The script lists it now as a %s",
                            ScriptPropertyUtils::getTypeName(declared->defaultValue.type));
    }
    else {
      ImGui::SetItemTooltip("The script does not list it any more");
    }
    ImGui::PopID();
  }
  if (toRemove) {
    const String name = toRemove->name;
    EditorScene::recordChange(*component.getOwner(), "Remove Script Value");
    static_cast<void>(component.removeOverride(name));
  }
}
} // namespace

/*
 */
void
ScriptPropertiesUI::render(ScriptComponent& component)
{
  if (!component.getScript()) {
    return;
  }
  const ScriptClassInfo* scriptClass = component.getScriptClass();
  if (!scriptClass || !scriptClass->bValid) {
    ImGui::TextColored(ImVec4(0.95f, 0.4f, 0.4f, 1.0f),
                       "The script has an error (see the Output Log)");
    return;
  }
  if (scriptClass->properties.empty() && component.getOverrides().empty()) {
    return;
  }

  GameObject& owner = *component.getOwner();
  const bool bRunning = component.isRunning();
  ScriptManager& scriptManager = ScriptManager::instance();
  ImGui::Spacing();
  for (const ScriptProperty& property : scriptClass->properties) {
    ImGui::PushID(property.name.c_str());
    const ScriptPropertyType type = property.defaultValue.type;
    const ScriptPropertyValue* saved = component.findOverride(property.name);
    const bool bOverridden = saved && saved->type == type;

    // While the script runs, what self holds now; the script may have changed it.
    ScriptPropertyValue value;
    if (!bRunning ||
        !scriptManager.getInstanceProperty(component.getInstance(), property.name, type,
                                           value)) {
      value = bOverridden ? *saved : property.defaultValue;
    }

    // Kept the same width whether or not the button shows, so rows do not jump.
    const float resetWidth = ImGui::GetFrameHeight();
    bool bReset = false;
    if (bOverridden) {
      bReset = ImGui::Button(ICON_FA_UNDO, ImVec2(resetWidth, 0.0f));
      ImGui::SetItemTooltip("Back to the script's default");
    }
    else {
      ImGui::Dummy(ImVec2(resetWidth, ImGui::GetFrameHeight()));
    }
    ImGui::SameLine();

    if (renderValue(property, value, owner)) {
      EditorScene::recordChange(owner, "Edit Script Value");
      component.setOverride(property.name, value);
    }
    if (!property.tooltip.empty()) {
      ImGui::SetItemTooltip("%s", property.tooltip.c_str());
    }
    if (bReset) {
      EditorScene::recordChange(owner, "Reset Script Value");
      static_cast<void>(component.removeOverride(property.name));
    }
    ImGui::PopID();
  }
  renderUnusedOverrides(component, *scriptClass);
}

} // namespace chEngineSDK
