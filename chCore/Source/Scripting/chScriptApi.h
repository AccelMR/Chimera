/************************************************************************/
/**
 * @file chScriptApi.h
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  A description of everything the engine gives scripts, for code editors.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

struct lua_State;

namespace chEngineSDK {

/**
 * One way to call a function, in the text LuaLS annotations use: parameters as
 * "name: type, name?: type" (a ? marks an optional one, ... any count) and the results as
 * "type, type".
 */
struct ScriptApiSignature
{
  String parameters;
  String results;
};

struct ScriptApiFunction
{
  String name;
  // Called as value:name(...) instead of Type.name(...).
  bool bMethod = false;
  // The first one is the function; the rest are the other ways to call it.
  Vector<ScriptApiSignature> signatures;
};

struct ScriptApiField
{
  String name;
  String type;
};

enum class ScriptApiTypeKind : uint8 {
  // A type whose global table makes values and holds its static functions (Vector3).
  Class,
  // A global table of functions and values (Input, Time).
  Library,
  // Only a shape for annotations, with no global (the self of a script).
  Shape
};

struct ScriptApiType
{
  String name;
  ScriptApiTypeKind kind = ScriptApiTypeKind::Class;
  String description;
  // The type this one extends, empty for none.
  String base;
  // Members of each value.
  Vector<ScriptApiField> fields;
  // Members of the global table (Vector3.UP, Time.deltaTime).
  Vector<ScriptApiField> constants;
  Vector<ScriptApiSignature> constructors;
  // As LuaLS writes them: "add(Vector3): Vector3", "unm: Vector3".
  Vector<String> operators;
  Vector<ScriptApiFunction> functions;
};

struct ScriptApiEnumValue
{
  String name;
  int64 value = 0;
};

struct ScriptApiEnum
{
  String name;
  Vector<ScriptApiEnumValue> values;
};

/**
 * Exists so code editors can complete and check what scripts call: the binder writes here
 * every type, function and value it registers, and writeDefinitions turns that into a LuaLS
 * definition file. Only the editor makes one; a state without it records nothing.
 */
class CH_CORE_EXPORT ScriptApiDescription
{
 public:
  ScriptApiDescription() = default;

  ScriptApiDescription(const ScriptApiDescription&) = delete;

  ScriptApiDescription&
  operator=(const ScriptApiDescription&) = delete;

  /**
   * Registrations into state are recorded here from now on (null stops recording). The state
   * must not outlive this.
   */
  static void
  attach(lua_State* state, ScriptApiDescription* description) noexcept;

  /**
   * The description registrations into state go to; null when nothing records them.
   */
  NODISCARD static ScriptApiDescription*
  find(lua_State* state) noexcept;

  /**
   * The type with that name, made the first time it is asked for, so a type registered in
   * two steps keeps one entry. The reference stays valid while this lives.
   */
  ScriptApiType&
  addType(StringView name, ScriptApiTypeKind kind);

  ScriptApiEnum&
  addEnum(StringView name);

  /**
   * A name for a type written out (InputAction for "integer|string").
   */
  void
  addAlias(StringView name, StringView type);

  /**
   * Reads "fun(name: type, ...): result, ..." into out. False when the text is not written
   * that way.
   */
  NODISCARD static bool
  parseSignature(StringView text, ScriptApiSignature& out);

  /**
   * Adds the signatures written as "fun(...)" to function, logging the ones that do not read.
   */
  static void
  addSignatures(ScriptApiFunction& function, Span<const ANSICHAR* const> signatures);

  /**
   * The whole description as a LuaLS definition file (---@meta).
   */
  NODISCARD String
  writeDefinitions() const;

  /**
   * A LuaLS configuration (.luarc.json) for a project whose definitions are in
   * definitionFolder, relative to it, with the libraries scripts lack turned off.
   */
  NODISCARD static String
  writeLuaLsConfig(StringView definitionFolder);

  NODISCARD const Vector<UniquePtr<ScriptApiType>>&
  getTypes() const noexcept { return m_types; }

  NODISCARD const Vector<ScriptApiEnum>&
  getEnums() const noexcept { return m_enums; }

 private:
  // Pointers, so the reference addType gives survives later types.
  Vector<UniquePtr<ScriptApiType>> m_types;
  Vector<ScriptApiEnum> m_enums;
  Vector<ScriptApiField> m_aliases;
};

} // namespace chEngineSDK
