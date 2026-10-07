/************************************************************************/
/**
 * @file chStringUtils.h
 * @author AccelMR
 * @date 2022/06/23
 * @brief String helpers and the format function used by the logger.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include <concepts>

namespace chEngineSDK {

/**
 * How StringUtils::format writes an argument. Decided at compile time from the argument
 * type, so FormatString can check that a spec fits its argument.
 */
enum class FormatArgType : uint8
{
  Text,
  Char,
  Bool,
  Signed,
  Unsigned,
  Float
};

template<typename T>
consteval FormatArgType
formatArgTypeOf()
{
  using Type = std::decay_t<T>;
  if constexpr (std::is_same_v<Type, bool>) {
    return FormatArgType::Bool;
  }
  else if constexpr (std::is_same_v<Type, ANSICHAR>) {
    return FormatArgType::Char;
  }
  else if constexpr (std::is_convertible_v<const Type&, StringView>) {
    return FormatArgType::Text;
  }
  else if constexpr (std::is_integral_v<Type>) {
    return std::is_signed_v<Type> ? FormatArgType::Signed : FormatArgType::Unsigned;
  }
  else if constexpr (std::is_floating_point_v<Type>) {
    return FormatArgType::Float;
  }
  else if constexpr (std::is_enum_v<Type>) {
    return std::is_signed_v<std::underlying_type_t<Type>> ? FormatArgType::Signed
                                                          : FormatArgType::Unsigned;
  }
  else {
    return FormatArgType::Text;
  }
}

/**
 * The part of a placeholder after ':', a subset of std::format:
 * [[fill]align][0][width][.precision][type]
 *   align      '<' left, '>' right, '^' center. Numbers go right by default, the rest left.
 *   0          pads numbers with zeros after the sign.
 *   precision  digits after the point (f, e) or significant digits (g, none). Floats only.
 *   type       d x X b o for integers, f e g for floats.
 */
struct FormatSpec
{
  static constexpr uint32 MAX_WIDTH = 999;
  static constexpr int32 MAX_PRECISION = 99;

  ANSICHAR fill = ' ';
  ANSICHAR align = '\0';
  bool zeroPad = false;
  uint32 width = 0;
  int32 precision = -1;
  ANSICHAR type = '\0';

  /**
   * @return false if the text is not a valid spec.
   */
  NODISCARD static constexpr bool
  parse(StringView text, FormatSpec& spec) noexcept;

  NODISCARD constexpr bool
  fits(FormatArgType argType) const noexcept;
};

/**
 * Format string for StringUtils::format, checked at compile time against the arguments so a
 * wrong placeholder stops the build instead of failing at runtime.
 *
 * Placeholders: "{}" takes the arguments in order, "{0}" picks one by index (it can be
 * repeated or reordered). Both kinds cannot be mixed. "{{" and "}}" write a brace. A spec
 * can follow a ':' ("{:.2f}", "{0:>8}", "{:08x}"), see FormatSpec.
 */
template<typename... Args>
class FormatString
{
 public:
  template<typename T>
    requires std::convertible_to<const T&, StringView>
  consteval FormatString(const T& text)
   : m_text(text)
  {
    check();
  }

  NODISCARD constexpr StringView
  get() const noexcept
  {
    return m_text;
  }

 private:
  consteval void
  check() const;

  // Not constexpr on purpose: reaching one of them during the compile time check stops
  // the build, and its name tells what is wrong.
  static void
  errorIndexOutOfRange()
  {}

  static void
  errorUnclosedBrace()
  {}

  static void
  errorUnmatchedClosingBrace()
  {}

  static void
  errorInvalidPlaceholder()
  {}

  static void
  errorMixedAutomaticAndManualIndex()
  {}

  static void
  errorInvalidFormatSpec()
  {}

  static void
  errorFormatSpecDoesNotFitArgumentType()
  {}

  StringView m_text;
};

/**
 * Helpers for String that the standard library does not have.
 */
class CH_UTILITY_EXPORT StringUtils
{
 public:
  NODISCARD static bool
  equals(StringView str1, StringView str2) noexcept
  {
    return str1 == str2;
  }

  /**
   * Copies src into a fixed size char array, cutting it if it does not fit. The result
   * always ends with '\0'.
   *
   * @return false if src was cut.
   */
  template<SIZE_T N>
  static bool
  copyToBuffer(ANSICHAR (&dest)[N], StringView src) noexcept
  {
    static_assert(N > 0, "The destination buffer needs room for the '\\0'.");
    const SIZE_T count = src.size() < N ? src.size() : N - 1;
    src.copy(dest, count);
    dest[count] = '\0';
    return count == src.size();
  }

  NODISCARD static String
  replaceAllChars(const String& toReplace, ANSICHAR from, ANSICHAR to);

  NODISCARD static String
  replaceAllSubStr(const String& toReplace, const String& from, const String& to);

  /**
   * Splits the string at every separator. Empty pieces are skipped.
   */
  NODISCARD static Vector<String>
  splitString(const String& toSplit, ANSICHAR separator);

  /**
   * Splits the string at every separator. Empty pieces are skipped.
   */
  NODISCARD static Vector<String>
  splitString(const String& toSplit, const String& separator);

  NODISCARD static String
  join(const Vector<String>& toJoin, const String& separator);

  /**
   * Text without arguments is returned as it is, without looking for placeholders, so it
   * can come from anywhere (e.g. messages from a library).
   */
  NODISCARD static String
  format(StringView text)
  {
    return String(text);
  }

  /**
   * Replaces the placeholders of the format with the arguments. See FormatString.
   * Arguments can be strings, numbers, enums, anything convertible to String, or any
   * type with a toString() method.
   */
  template<typename... Args>
    requires(sizeof...(Args) > 0)
  NODISCARD static String
  format(FormatString<std::type_identity_t<Args>...> format, Args&&... args);

  template<typename T>
  NODISCARD static String
  toString(T&& value);

  /**
   * ASCII only, other bytes are left as they are.
   */
  NODISCARD static String
  toLower(const String& str);

  /**
   * ASCII only, other bytes are left as they are.
   */
  NODISCARD static String
  toUpper(const String& str);

  /**
   * True if search appears in text, ignoring ASCII case. Does not allocate. An empty
   * search is always found.
   */
  NODISCARD static bool
  containsIgnoreCase(StringView text, StringView search) noexcept;

  /**
   * Compares ignoring ASCII case. Does not allocate.
   */
  NODISCARD static bool
  equalsIgnoreCase(StringView str1, StringView str2) noexcept;

  /**
   * Writes a number in decimal into buffer without allocating; floats get the shortest
   * text that reads back as the same value. No '\0' is added.
   *
   * @return The written text, which points into buffer.
   */
  template<SIZE_T N, typename T>
    requires(std::is_arithmetic_v<T> && !std::is_same_v<T, bool> &&
             !std::is_same_v<T, ANSICHAR>)
  NODISCARD static StringView
  toChars(ANSICHAR (&buffer)[N], T value) noexcept
  {
    if constexpr (std::is_floating_point_v<T>) {
      static_assert(N >= MAX_FLOAT_CHARS, "The buffer is too small for a float.");
      return floatToChars(buffer, N, static_cast<double>(value));
    }
    else if constexpr (std::is_signed_v<T>) {
      static_assert(N >= MAX_INTEGER_CHARS, "The buffer is too small for an integer.");
      return signedToChars(buffer, N, static_cast<int64>(value));
    }
    else {
      static_assert(N >= MAX_INTEGER_CHARS, "The buffer is too small for an integer.");
      return unsignedToChars(buffer, N, static_cast<uint64>(value));
    }
  }

  NODISCARD static String
  lTrim(const String& str);

  NODISCARD static String
  rTrim(const String& str);

  NODISCARD static String
  trim(const String& str);

  /**
   * Same as trim, without allocating: the result points into text.
   */
  NODISCARD static StringView
  trimView(StringView text) noexcept;

  /**
   * Buffer sizes that fit any number written by toChars: "-9223372036854775808" and
   * "-2.2250738585072014e-308".
   */
  static constexpr SIZE_T MAX_INTEGER_CHARS = 20;
  static constexpr SIZE_T MAX_FLOAT_CHARS = 24;

 private:
  /**
   * An argument of format without converting it: numbers, chars and bools keep their
   * value and are written by formatArgs, text is only viewed.
   */
  struct FormatArg
  {
    FormatArgType type = FormatArgType::Text;
    union
    {
      int64 signedValue = 0;
      uint64 unsignedValue;
      double floatValue;
      ANSICHAR charValue;
      bool boolValue;
    };
    StringView text;
  };

  template<typename T>
  static FormatArg
  makeFormatArg(T&& value, String*& nextString);

  static String
  formatArgs(StringView format, const FormatArg* args, SIZE_T count);

  NODISCARD static StringView
  signedToChars(ANSICHAR* buffer, SIZE_T size, int64 value) noexcept;

  NODISCARD static StringView
  unsignedToChars(ANSICHAR* buffer, SIZE_T size, uint64 value) noexcept;

  NODISCARD static StringView
  floatToChars(ANSICHAR* buffer, SIZE_T size, double value) noexcept;

  template<typename T>
  static constexpr bool NEEDS_STRING = formatArgTypeOf<T>() == FormatArgType::Text &&
                                       !std::is_convertible_v<const std::decay_t<T>&,
                                                              StringView>;
};

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
constexpr bool
FormatSpec::parse(StringView text, FormatSpec& spec) noexcept
{
  const auto isAlign = [](ANSICHAR c) { return c == '<' || c == '>' || c == '^'; };
  const auto isDigit = [](ANSICHAR c) { return c >= '0' && c <= '9'; };

  SIZE_T i = 0;
  if (text.size() >= 2 && isAlign(text[1])) {
    if (text[0] == '{' || text[0] == '}') {
      return false;
    }
    spec.fill = text[0];
    spec.align = text[1];
    i = 2;
  }
  else if (!text.empty() && isAlign(text[0])) {
    spec.align = text[0];
    i = 1;
  }

  if (i < text.size() && text[i] == '0') {
    spec.zeroPad = true;
    ++i;
  }

  while (i < text.size() && isDigit(text[i])) {
    spec.width = spec.width * 10 + static_cast<uint32>(text[i] - '0');
    if (spec.width > MAX_WIDTH) {
      return false;
    }
    ++i;
  }

  if (i < text.size() && text[i] == '.') {
    ++i;
    if (i >= text.size() || !isDigit(text[i])) {
      return false;
    }
    spec.precision = 0;
    while (i < text.size() && isDigit(text[i])) {
      spec.precision = spec.precision * 10 + static_cast<int32>(text[i] - '0');
      if (spec.precision > MAX_PRECISION) {
        return false;
      }
      ++i;
    }
  }

  if (i < text.size()) {
    spec.type = text[i++];
    if (StringView("dxXbofeg").find(spec.type) == StringView::npos) {
      return false;
    }
  }

  return i == text.size();
}

/*
 */
constexpr bool
FormatSpec::fits(FormatArgType argType) const noexcept
{
  const bool isInteger = argType == FormatArgType::Signed ||
                         argType == FormatArgType::Unsigned;
  const bool isFloat = argType == FormatArgType::Float;

  if (precision >= 0 && !isFloat) {
    return false;
  }
  if (zeroPad && !isInteger && !isFloat) {
    return false;
  }

  switch (type) {
  case '\0':
    return true;
  case 'd':
  case 'x':
  case 'X':
  case 'b':
  case 'o':
    return isInteger;
  default:
    return isFloat;
  }
}

/*
 */
template<typename... Args>
consteval void
FormatString<Args...>::check() const
{
  constexpr SIZE_T argCount = sizeof...(Args);
  // The extra entry keeps the array valid when there are no arguments.
  constexpr FormatArgType argTypes[] = {formatArgTypeOf<Args>()..., FormatArgType::Text};
  bool usesAutomatic = false;
  bool usesManual = false;
  SIZE_T automaticCount = 0;

  SIZE_T i = 0;
  while (i < m_text.size()) {
    const ANSICHAR c = m_text[i];

    if (c == '}') {
      if (i + 1 >= m_text.size() || m_text[i + 1] != '}') {
        errorUnmatchedClosingBrace();
      }
      i += 2;
      continue;
    }

    if (c != '{') {
      ++i;
      continue;
    }

    if (i + 1 < m_text.size() && m_text[i + 1] == '{') {
      i += 2;
      continue;
    }

    const SIZE_T close = m_text.find('}', i + 1);
    if (close == StringView::npos) {
      errorUnclosedBrace();
    }

    const StringView content = m_text.substr(i + 1, close - i - 1);
    const SIZE_T colon = content.find(':');
    const StringView indexText = content.substr(0, colon);

    SIZE_T index = 0;
    if (indexText.empty()) {
      usesAutomatic = true;
      index = automaticCount++;
    }
    else {
      usesManual = true;
      for (const ANSICHAR digit : indexText) {
        if (digit < '0' || digit > '9') {
          errorInvalidPlaceholder();
        }
        index = index * 10 + static_cast<SIZE_T>(digit - '0');
      }
    }

    if (index >= argCount) {
      errorIndexOutOfRange();
    }

    if (usesAutomatic && usesManual) {
      errorMixedAutomaticAndManualIndex();
    }

    if (colon != StringView::npos) {
      FormatSpec spec;
      if (!FormatSpec::parse(content.substr(colon + 1), spec)) {
        errorInvalidFormatSpec();
      }
      if (!spec.fits(argTypes[index])) {
        errorFormatSpecDoesNotFitArgumentType();
      }
    }

    i = close + 1;
  }
}

/*
 */
template<typename T>
String
StringUtils::toString(T&& value)
{
  using Type = std::decay_t<T>;

  if constexpr (std::is_same_v<Type, String>) {
    return std::forward<T>(value);
  }
  else if constexpr (std::is_arithmetic_v<Type> || std::is_enum_v<Type>) {
    // Written by format so numbers look the same everywhere. Enums print their number,
    // which also covers C enums such as VkResult.
    return format("{}", value);
  }
  else if constexpr (std::is_convertible_v<T, String>) {
    return String(std::forward<T>(value));
  }
  else if constexpr (requires { { value.toString() } -> std::convertible_to<String>; }) {
    return value.toString();
  }
  else {
    // sizeof(Type*) == 0 is never true but depends on T, so it only fails when this
    // branch is used.
    static_assert(sizeof(Type*) == 0,
                  "StringUtils::toString: the type is not a string, number or enum, is not "
                  "convertible to String and has no toString() method.");
  }
}

/*
 */
template<typename T>
StringUtils::FormatArg
StringUtils::makeFormatArg(T&& value, String*& nextString)
{
  using Type = std::decay_t<T>;
  constexpr FormatArgType type = formatArgTypeOf<T>();
  FormatArg arg;
  arg.type = type;

  if constexpr (type == FormatArgType::Bool) {
    arg.boolValue = value;
  }
  else if constexpr (type == FormatArgType::Char) {
    arg.charValue = value;
  }
  else if constexpr (type == FormatArgType::Signed) {
    arg.signedValue = static_cast<int64>(value);
  }
  else if constexpr (type == FormatArgType::Unsigned) {
    arg.unsignedValue = static_cast<uint64>(value);
  }
  else if constexpr (type == FormatArgType::Float) {
    arg.floatValue = static_cast<double>(value);
  }
  else if constexpr (std::is_pointer_v<Type> && std::is_convertible_v<Type, StringView>) {
    arg.text = value ? StringView(value) : StringView("(null)");
  }
  else if constexpr (std::is_convertible_v<const Type&, StringView>) {
    arg.text = value;
  }
  else {
    *nextString = toString(std::forward<T>(value));
    arg.text = *nextString;
    ++nextString;
  }

  return arg;
}

/*
 */
template<typename... Args>
  requires(sizeof...(Args) > 0)
String
StringUtils::format(FormatString<std::type_identity_t<Args>...> format, Args&&... args)
{
  // Only types written through toString() need a String. They are filled in place and
  // never moved, so the views to them stay valid.
  Array<String, (static_cast<SIZE_T>(NEEDS_STRING<Args>) + ...)> strings;
  String* nextString = strings.data();

  // A braced list runs its elements in order, so nextString is used in order too.
  const Array<FormatArg, sizeof...(Args)> converted{
    makeFormatArg(std::forward<Args>(args), nextString)...};

  return formatArgs(format.get(), converted.data(), converted.size());
}

} // namespace chEngineSDK
