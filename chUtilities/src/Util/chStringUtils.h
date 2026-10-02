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
 * Format string for chString::format, checked at compile time against the number of
 * arguments so a wrong placeholder stops the build instead of failing at runtime.
 *
 * Placeholders: "{}" takes the arguments in order, "{0}" picks one by index (it can be
 * repeated or reordered). Both kinds cannot be mixed. "{{" and "}}" write a brace.
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

  StringView m_text;
};

/**
 * Helpers for String that the standard library does not have.
 */
class CH_UTILITY_EXPORT chString
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

  NODISCARD static String
  lTrim(const String& str);

  NODISCARD static String
  rTrim(const String& str);

  NODISCARD static String
  trim(const String& str);

 private:
  /**
   * An argument of format seen as text. Text arguments are only viewed, the rest are
   * converted and kept in 'owned' until the format ends.
   */
  struct FormatArg
  {
    String owned;
    StringView view;
    bool isOwned = false;
  };

  template<typename T>
  static FormatArg
  makeFormatArg(T&& value);

  static String
  formatArgs(StringView format, const StringView* args, SIZE_T count);
};

/************************************************************************/
/*
 * Implementation
 */
/************************************************************************/

/*
 */
template<typename... Args>
consteval void
FormatString<Args...>::check() const
{
  constexpr SIZE_T argCount = sizeof...(Args);
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

    if (close == i + 1) {
      usesAutomatic = true;
      if (automaticCount >= argCount) {
        errorIndexOutOfRange();
      }
      ++automaticCount;
    }
    else {
      usesManual = true;
      SIZE_T index = 0;
      for (SIZE_T d = i + 1; d < close; ++d) {
        if (m_text[d] < '0' || m_text[d] > '9') {
          errorInvalidPlaceholder();
        }
        index = index * 10 + static_cast<SIZE_T>(m_text[d] - '0');
      }
      if (index >= argCount) {
        errorIndexOutOfRange();
      }
    }

    if (usesAutomatic && usesManual) {
      errorMixedAutomaticAndManualIndex();
    }

    i = close + 1;
  }
}

/*
 */
template<typename T>
String
chString::toString(T&& value)
{
  using Type = std::decay_t<T>;

  if constexpr (std::is_same_v<Type, String>) {
    return std::forward<T>(value);
  }
  else if constexpr (std::is_arithmetic_v<Type>) {
    return std::to_string(static_cast<Type>(value));
  }
  else if constexpr (std::is_enum_v<Type>) {
    // Enums print their number, which also covers C enums such as VkResult.
    return std::to_string(static_cast<std::underlying_type_t<Type>>(value));
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
                  "chString::toString: the type is not a string, number or enum, is not "
                  "convertible to String and has no toString() method.");
  }
}

/*
 */
template<typename T>
chString::FormatArg
chString::makeFormatArg(T&& value)
{
  using Type = std::decay_t<T>;
  FormatArg arg;

  if constexpr (std::is_pointer_v<Type> && std::is_convertible_v<Type, StringView>) {
    arg.view = value ? StringView(value) : StringView("(null)");
  }
  else if constexpr (std::is_convertible_v<const T&, StringView>) {
    arg.view = value;
  }
  else {
    arg.owned = toString(std::forward<T>(value));
    arg.isOwned = true;
  }

  return arg;
}

/*
 */
template<typename... Args>
  requires(sizeof...(Args) > 0)
String
chString::format(FormatString<std::type_identity_t<Args>...> format, Args&&... args)
{
  Array<FormatArg, sizeof...(Args)> converted{makeFormatArg(std::forward<Args>(args))...};

  // Views are taken only now, because moving a short String into the array moves its
  // characters too.
  Array<StringView, sizeof...(Args)> views;
  for (SIZE_T i = 0; i < converted.size(); ++i) {
    views[i] = converted[i].isOwned ? StringView(converted[i].owned)
                                    : converted[i].view;
  }

  return formatArgs(format.get(), views.data(), views.size());
}

} // namespace chEngineSDK
