/************************************************************************/
/**
 * @file chScriptProperty.cpp
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  Values a script declares so they can be set per object in the editor.
 */
/************************************************************************/
#include "chScriptProperty.h"

#include "chAssetFile.h"
#include "chFileStream.h"

namespace chEngineSDK {
namespace {
// In ScriptPropertyType order.
constexpr const ANSICHAR* kTypeNames[] = {"number",  "integer", "boolean",
                                          "string",  "Vector2", "Vector3",
                                          "Vector4", "Color",   "GameObject",
                                          "Prefab",  "Scene"};
static_assert(sizeof(kTypeNames) / sizeof(kTypeNames[0]) ==
              static_cast<SIZE_T>(ScriptPropertyType::COUNT));

constexpr const ANSICHAR* kReservedNames[] = {"gameObject", "transform", "properties",
                                              "onStart",    "update",    "fixedUpdate",
                                              "onDestroy"};

NODISCARD uint32
getComponentCount(const ScriptPropertyType type)
{
  switch (type) {
    case ScriptPropertyType::Vector2:
      return 2;
    case ScriptPropertyType::Vector3:
      return 3;
    case ScriptPropertyType::Vector4:
    case ScriptPropertyType::Color:
      return 4;
    default:
      return 0;
  }
}
} // namespace

/*
 */
ScriptPropertyValue
ScriptPropertyValue::makeDefault(const ScriptPropertyType type)
{
  ScriptPropertyValue value;
  value.type = type;
  if (type == ScriptPropertyType::Color) {
    value.components[0] = value.components[1] = value.components[2] = 1.0f;
    value.components[3] = 1.0f;
  }
  return value;
}

/*
 */
bool
ScriptPropertyValue::operator==(const ScriptPropertyValue& other) const
{
  if (type != other.type) {
    return false;
  }
  switch (type) {
    case ScriptPropertyType::Number:
      return number == other.number;
    case ScriptPropertyType::Integer:
      return integer == other.integer;
    case ScriptPropertyType::Boolean:
      return boolean == other.boolean;
    case ScriptPropertyType::String:
      return text == other.text;
    case ScriptPropertyType::GameObject:
    case ScriptPropertyType::Prefab:
    case ScriptPropertyType::Scene:
      return object == other.object;
    default:
      for (uint32 i = 0; i < getComponentCount(type); ++i) {
        if (components[i] != other.components[i]) {
          return false;
        }
      }
      return true;
  }
}

/*
 */
const ANSICHAR*
ScriptPropertyUtils::getTypeName(const ScriptPropertyType type)
{
  const SIZE_T index = static_cast<SIZE_T>(type);
  return index < static_cast<SIZE_T>(ScriptPropertyType::COUNT) ? kTypeNames[index]
                                                                : "unknown";
}

/*
 */
bool
ScriptPropertyUtils::fromTypeName(const StringView name, ScriptPropertyType& outType)
{
  for (SIZE_T i = 0; i < static_cast<SIZE_T>(ScriptPropertyType::COUNT); ++i) {
    if (name == kTypeNames[i]) {
      outType = static_cast<ScriptPropertyType>(i);
      return true;
    }
  }
  return false;
}

/*
 */
void
ScriptPropertyUtils::writeValue(DataStream& stream, const ScriptPropertyValue& value)
{
  const uint8 type = static_cast<uint8>(value.type);
  stream.write(&type, sizeof(type));
  switch (value.type) {
    case ScriptPropertyType::Number:
      stream.write(&value.number, sizeof(value.number));
      break;
    case ScriptPropertyType::Integer:
      stream.write(&value.integer, sizeof(value.integer));
      break;
    case ScriptPropertyType::Boolean: {
      const uint8 boolean = value.boolean ? 1 : 0;
      stream.write(&boolean, sizeof(boolean));
      break;
    }
    case ScriptPropertyType::String:
      AssetFile::writeString(
          stream, StringView(value.text).substr(0, static_cast<SIZE_T>(MAX_TEXT_LENGTH)));
      break;
    case ScriptPropertyType::GameObject:
    case ScriptPropertyType::Prefab:
    case ScriptPropertyType::Scene:
      stream.write(&value.object, sizeof(value.object));
      break;
    default:
      stream.write(value.components, sizeof(float) * getComponentCount(value.type));
      break;
  }
}

/*
 */
bool
ScriptPropertyUtils::readValue(DataStream& stream, ScriptPropertyValue& outValue)
{
  uint8 type = 0;
  if (!AssetFile::readValue(stream, type) ||
      type >= static_cast<uint8>(ScriptPropertyType::COUNT)) {
    return false;
  }
  outValue = ScriptPropertyValue::makeDefault(static_cast<ScriptPropertyType>(type));
  switch (outValue.type) {
    case ScriptPropertyType::Number:
      return AssetFile::readValue(stream, outValue.number);
    case ScriptPropertyType::Integer:
      return AssetFile::readValue(stream, outValue.integer);
    case ScriptPropertyType::Boolean: {
      uint8 boolean = 0;
      if (!AssetFile::readValue(stream, boolean)) {
        return false;
      }
      outValue.boolean = boolean != 0;
      return true;
    }
    case ScriptPropertyType::String:
      return AssetFile::readString(stream, outValue.text, MAX_TEXT_LENGTH);
    case ScriptPropertyType::GameObject:
    case ScriptPropertyType::Prefab:
    case ScriptPropertyType::Scene:
      return AssetFile::readValue(stream, outValue.object);
    default:
      return AssetFile::readBytes(stream, outValue.components,
                                  sizeof(float) * getComponentCount(outValue.type));
  }
}

/*
 */
bool
ScriptPropertyUtils::isReservedName(const StringView name)
{
  for (const ANSICHAR* reserved : kReservedNames) {
    if (name == reserved) {
      return true;
    }
  }
  return false;
}

} // namespace chEngineSDK
