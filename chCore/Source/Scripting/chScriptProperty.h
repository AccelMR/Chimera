/************************************************************************/
/**
 * @file chScriptProperty.h
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  Values a script declares so they can be set per object in the editor.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

#include "chUUID.h"

namespace chEngineSDK {

/**
 * Stored in scene files, so the values are fixed; new ones go at the end.
 */
enum class ScriptPropertyType : uint8
{
  Number = 0,
  Integer = 1,
  Boolean = 2,
  String = 3,
  Vector2 = 4,
  Vector3 = 5,
  Vector4 = 6,
  Color = 7,
  GameObject = 8,
  Prefab = 9,
  COUNT
};

/**
 * Exists so a property value can be kept, saved and compared without the Lua state: the
 * component keeps its overrides in it and the class its defaults, and both are pushed into
 * self when an instance starts. Only the member of its type is used.
 */
struct CH_CORE_EXPORT ScriptPropertyValue
{
  /**
   * Zero, false, empty, a zero vector, white, or no object.
   */
  NODISCARD static ScriptPropertyValue
  makeDefault(ScriptPropertyType type);

  NODISCARD bool
  operator==(const ScriptPropertyValue& other) const;

  ScriptPropertyType type = ScriptPropertyType::Number;
  double number = 0.0;
  int64 integer = 0;
  bool boolean = false;
  // Vectors use their first components; colors are r, g, b, a.
  float components[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  String text;
  // GameObject: the UUID of an object in the same scene. Prefab: the UUID of a prefab asset.
  // Null for none.
  UUID object = UUID::null();
};

/**
 * Exists so the editor and the instances read what a script declared in its properties list
 * once, when the script is compiled, and never from Lua each frame. The editor-only limits and
 * tooltip are not kept in games.
 */
struct ScriptProperty
{
  String name;
  ScriptPropertyValue defaultValue;
#if USING(CH_EDITOR)
  // Only used when minimum < maximum; numbers with both show a slider.
  double minimum = 0.0;
  double maximum = 0.0;
  String tooltip;
#endif
};

/**
 * A value one component sets for a property, by its name.
 */
struct ScriptPropertyOverride
{
  String name;
  ScriptPropertyValue value;
};

/**
 * Exists so names of property types are written the same way in scripts, errors and the
 * editor.
 */
class CH_CORE_EXPORT ScriptPropertyUtils
{
 public:
  /**
   * The name scripts write in type = "...".
   */
  NODISCARD static const ANSICHAR*
  getTypeName(ScriptPropertyType type);

  /**
   * False for a name that is not a type.
   */
  NODISCARD static bool
  fromTypeName(StringView name, ScriptPropertyType& outType);

  /**
   * Writes value with its type. Strings longer than MAX_TEXT_LENGTH are cut.
   */
  static void
  writeValue(DataStream& stream, const ScriptPropertyValue& value);

  /**
   * False for broken data or an unknown type.
   */
  NODISCARD static bool
  readValue(DataStream& stream, ScriptPropertyValue& outValue);

  /**
   * Names a script cannot use: the fields every instance has and the functions the engine
   * calls.
   */
  NODISCARD static bool
  isReservedName(StringView name);

  static constexpr uint32 MAX_NAME_LENGTH = 128;
  static constexpr uint32 MAX_TEXT_LENGTH = 4096;
  static constexpr uint32 MAX_PROPERTIES = 128;
};

} // namespace chEngineSDK
