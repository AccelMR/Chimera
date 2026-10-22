/************************************************************************/
/**
 * @file chScriptApi.cpp
 * @author AccelMR
 * @date 2026/10/22
 * @brief
 *  A description of everything the engine gives scripts, for code editors.
 */
/************************************************************************/
#include "chScriptApi.h"

#include "chScriptManager.h"
#include "chStringUtils.h"

#include <cstring>

#include "lua.h"

namespace chEngineSDK {
namespace {
const ANSICHAR* const kLuaKeywords[] = {"and",   "break", "do",     "else", "elseif", "end",
                                        "false", "for",   "function", "goto", "if",   "in",
                                        "local", "nil",   "not",    "or",   "repeat", "return",
                                        "then",  "true",  "until",  "while"};

/*
 * Splits at the commas that are not inside (), <>, {} or [], so "fun(a, b)" and
 * "table<string, number>" stay one piece.
 */
Vector<StringView>
splitTopLevel(const StringView text)
{
  Vector<StringView> pieces;
  int32 depth = 0;
  SIZE_T start = 0;
  for (SIZE_T i = 0; i < text.size(); ++i) {
    const ANSICHAR c = text[i];
    if (c == '(' || c == '<' || c == '{' || c == '[') {
      ++depth;
    }
    else if (c == ')' || c == '>' || c == '}' || c == ']') {
      --depth;
    }
    else if (c == ',' && depth == 0) {
      pieces.push_back(StringUtils::trimView(text.substr(start, i - start)));
      start = i + 1;
    }
  }
  const StringView last = StringUtils::trimView(text.substr(start));
  if (!last.empty() || !pieces.empty()) {
    pieces.push_back(last);
  }
  return pieces;
}

struct Parameter
{
  // "out?", "x", "..."
  StringView name;
  StringView type;
};

/*
 * "name: type"; a name alone has any type.
 */
Parameter
readParameter(const StringView text)
{
  const SIZE_T colon = text.find(':');
  if (colon == StringView::npos) {
    return {text, "any"};
  }
  return {StringUtils::trimView(text.substr(0, colon)),
          StringUtils::trimView(text.substr(colon + 1))};
}

/*
 * The name a function declaration lists, without the ? of an optional parameter.
 */
StringView
getArgumentName(const StringView name)
{
  return !name.empty() && name.back() == '?' ? name.substr(0, name.size() - 1) : name;
}

bool
isIdentifier(const StringView text)
{
  if (text.empty() || (text[0] >= '0' && text[0] <= '9')) {
    return false;
  }
  for (const ANSICHAR c : text) {
    const bool bLetter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    if (!bLetter && !(c >= '0' && c <= '9') && c != '_') {
      return false;
    }
  }
  for (const ANSICHAR* keyword : kLuaKeywords) {
    if (text == keyword) {
      return false;
    }
  }
  return true;
}

void
appendLine(String& out, const StringView text)
{
  out += text;
  out += '\n';
}

/*
 * Each line of text as a --- comment, which LuaLS shows with the next declaration.
 */
void
appendDescription(String& out, const StringView text)
{
  SIZE_T start = 0;
  while (start < text.size()) {
    SIZE_T end = text.find('\n', start);
    if (end == StringView::npos) {
      end = text.size();
    }
    out += "---";
    appendLine(out, text.substr(start, end - start));
    start = end + 1;
  }
}

/*
 * "fun(self: Type, parameters): results" for ---@overload.
 */
void
appendFunctionType(String& out, const ScriptApiSignature& signature, const StringView selfType)
{
  out += "fun(";
  if (!selfType.empty()) {
    out += "self: ";
    out += selfType;
    if (!signature.parameters.empty()) {
      out += ", ";
    }
  }
  out += signature.parameters;
  out += ')';
  if (!signature.results.empty()) {
    out += ": ";
    out += signature.results;
  }
}

void
appendFunction(String& out, const ScriptApiType& type, const ScriptApiFunction& function)
{
  if (function.signatures.empty()) {
    return;
  }
  const ScriptApiSignature& main = function.signatures[0];
  const Vector<StringView> parameters = splitTopLevel(main.parameters);
  for (const StringView text : parameters) {
    const Parameter parameter = readParameter(text);
    out += "---@param ";
    out += parameter.name;
    out += ' ';
    appendLine(out, parameter.type);
  }
  for (const StringView result : splitTopLevel(main.results)) {
    out += "---@return ";
    appendLine(out, result);
  }
  for (SIZE_T i = 1; i < function.signatures.size(); ++i) {
    out += "---@overload ";
    appendFunctionType(out, function.signatures[i],
                       function.bMethod ? StringView(type.name) : StringView());
    out += '\n';
  }

  out += "function ";
  out += type.name;
  out += function.bMethod ? ':' : '.';
  out += function.name;
  out += '(';
  for (SIZE_T i = 0; i < parameters.size(); ++i) {
    if (i > 0) {
      out += ", ";
    }
    out += getArgumentName(readParameter(parameters[i]).name);
  }
  out += ") end\n\n";
}

void
appendType(String& out, const ScriptApiType& type)
{
  appendDescription(out, type.description);
  out += "---@class ";
  out += type.name;
  if (!type.base.empty()) {
    out += " : ";
    out += type.base;
  }
  out += '\n';
  // A class keeps its statics and constants in the same table its values read methods from,
  // so LuaLS sees them as one class.
  for (const Vector<ScriptApiField>* fields : {&type.fields, &type.constants}) {
    for (const ScriptApiField& field : *fields) {
      out += "---@field ";
      out += field.name;
      out += ' ';
      appendLine(out, field.type);
    }
  }
  for (const String& op : type.operators) {
    out += "---@operator ";
    appendLine(out, op);
  }
  for (const ScriptApiSignature& constructor : type.constructors) {
    out += "---@overload ";
    appendFunctionType(out, constructor, StringView());
    out += '\n';
  }
  if (type.kind != ScriptApiTypeKind::Shape) {
    out += type.name;
    out += " = {}\n";
  }
  out += '\n';

  for (const ScriptApiFunction& function : type.functions) {
    appendFunction(out, type, function);
  }
}

void
appendEnum(String& out, const ScriptApiEnum& scriptEnum)
{
  out += "---@enum ";
  out += scriptEnum.name;
  out += '\n';
  out += scriptEnum.name;
  out += " = {\n";
  ANSICHAR number[StringUtils::MAX_INTEGER_CHARS];
  for (const ScriptApiEnumValue& value : scriptEnum.values) {
    out += "  ";
    if (isIdentifier(value.name)) {
      out += value.name;
    }
    else {
      out += "[\"";
      out += value.name;
      out += "\"]";
    }
    out += " = ";
    out += StringUtils::toChars(number, value.value);
    out += ",\n";
  }
  out += "}\n\n";
}
} // namespace

/*
 */
void
ScriptApiDescription::attach(lua_State* state, ScriptApiDescription* description) noexcept
{
  std::memcpy(lua_getextraspace(state), &description, sizeof(description));
}

/*
 */
ScriptApiDescription*
ScriptApiDescription::find(lua_State* state) noexcept
{
  ScriptApiDescription* description = nullptr;
  std::memcpy(&description, lua_getextraspace(state), sizeof(description));
  return description;
}

/*
 */
ScriptApiType&
ScriptApiDescription::addType(const StringView name, const ScriptApiTypeKind kind)
{
  for (const UniquePtr<ScriptApiType>& type : m_types) {
    if (type->name == name) {
      return *type;
    }
  }
  UniquePtr<ScriptApiType>& type = m_types.emplace_back(chMakeUnique<ScriptApiType>());
  type->name = String(name);
  type->kind = kind;
  return *type;
}

/*
 */
ScriptApiEnum&
ScriptApiDescription::addEnum(const StringView name)
{
  for (ScriptApiEnum& scriptEnum : m_enums) {
    if (scriptEnum.name == name) {
      return scriptEnum;
    }
  }
  ScriptApiEnum& scriptEnum = m_enums.emplace_back();
  scriptEnum.name = String(name);
  return scriptEnum;
}

/*
 */
void
ScriptApiDescription::addAlias(const StringView name, const StringView type)
{
  m_aliases.push_back({String(name), String(type)});
}

/*
 */
bool
ScriptApiDescription::parseSignature(const StringView text, ScriptApiSignature& out)
{
  const StringView trimmed = StringUtils::trimView(text);
  constexpr StringView kPrefix = "fun(";
  if (!trimmed.starts_with(kPrefix)) {
    return false;
  }
  int32 depth = 1;
  SIZE_T close = kPrefix.size();
  for (; close < trimmed.size(); ++close) {
    const ANSICHAR c = trimmed[close];
    if (c == '(') {
      ++depth;
    }
    else if (c == ')' && --depth == 0) {
      break;
    }
  }
  if (depth != 0) {
    return false;
  }
  const StringView rest = StringUtils::trimView(trimmed.substr(close + 1));
  if (!rest.empty() && rest[0] != ':') {
    return false;
  }
  out.parameters = String(
      StringUtils::trimView(trimmed.substr(kPrefix.size(), close - kPrefix.size())));
  out.results = rest.empty() ? String() : String(StringUtils::trimView(rest.substr(1)));
  return true;
}

/*
 */
void
ScriptApiDescription::addSignatures(ScriptApiFunction& function,
                                    const Span<const ANSICHAR* const> signatures)
{
  for (const ANSICHAR* text : signatures) {
    ScriptApiSignature signature;
    if (!parseSignature(text, signature)) {
      CH_LOG_WARNING(ScriptLog, "The script function {0} has a signature that does not read "
                     "as fun(...): {1}", function.name, text);
      continue;
    }
    function.signatures.push_back(std::move(signature));
  }
}

/*
 */
String
ScriptApiDescription::writeDefinitions() const
{
  String out;
  out.reserve(64 * 1024);
  appendLine(out, "---@meta");
  appendLine(out, "-- Written by the Chimera editor from the engine's script bindings every time "
                  "the project opens.");
  appendLine(out, "-- Changes made here are lost.");
  out += '\n';
  for (const ScriptApiField& alias : m_aliases) {
    out += "---@alias ";
    out += alias.name;
    out += ' ';
    appendLine(out, alias.type);
  }
  if (!m_aliases.empty()) {
    out += '\n';
  }
  for (const ScriptApiEnum& scriptEnum : m_enums) {
    appendEnum(out, scriptEnum);
  }
  for (const UniquePtr<ScriptApiType>& type : m_types) {
    appendType(out, *type);
  }
  return out;
}

/*
 */
String
ScriptApiDescription::writeLuaLsConfig(const StringView definitionFolder)
{
  // Scripts get no io, os, package or debug (see ScriptManager), so LuaLS should not offer
  // them either.
  return StringUtils::format("{{\n"
                             "  \"runtime.version\": \"Lua 5.4\",\n"
                             "  \"runtime.builtin\": {{\n"
                             "    \"io\": \"disable\",\n"
                             "    \"os\": \"disable\",\n"
                             "    \"package\": \"disable\",\n"
                             "    \"debug\": \"disable\"\n"
                             "  }},\n"
                             "  \"workspace.library\": [\"{0}\"],\n"
                             "  \"workspace.checkThirdParty\": false\n"
                             "}}\n",
                             definitionFolder);
}

} // namespace chEngineSDK
