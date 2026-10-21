/************************************************************************/
/**
 * @file chConfigFile.h
 * @author AccelMR
 * @date 2026/10/06
 * @brief Reads and writes .ini files.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

namespace chEngineSDK {

/**
 * Holds the text of an .ini file as sections of Key=Value entries, so settings can be
 * read from files the user edits by hand and written back.
 *
 * Format, one item per line:
 *   [Section]        starts a section; entries before the first one have no section ("")
 *   Key=Value        spaces around the key and the value are dropped
 *   Key="  Value  "  quotes keep the spaces and are removed
 *   ; comment        lines that start with ';' or '#' are skipped
 *
 * Section and key names ignore case. A key given twice keeps the last value. Bad lines
 * are logged with their file and line number and skipped. Comments are not kept, so
 * save() writes only sections and entries.
 */
class CH_UTILITY_EXPORT ConfigFile
{
 public:
  struct Entry
  {
    String key;
    String value;
  };

  struct Section
  {
    String name;
    Vector<Entry> entries;
  };

  /**
   * Adds the entries of the file to the ones already read, replacing equal keys.
   *
   * @return false if the file does not exist or cannot be read. Nothing is logged for a
   *         missing file, because most config layers are optional.
   */
  bool
  load(const Path& path);

  /**
   * Adds the entries of text to the ones already read, replacing equal keys. sourceName
   * only names the text in the log.
   */
  void
  parse(StringView text, StringView sourceName = "");

  /**
   * Adds every entry of other, replacing equal keys.
   */
  void
  merge(const ConfigFile& other);

  /**
   * Writes the file again from the sections, creating its folder if needed.
   */
  bool
  save(const Path& path) const;

  NODISCARD String
  toText() const;

  /**
   * Returns nullptr when the section or the key is missing.
   */
  NODISCARD const String*
  getValue(StringView section, StringView key) const noexcept;

  void
  setValue(StringView section, StringView key, StringView value);

  NODISCARD FORCEINLINE const Vector<Section>&
  getSections() const noexcept
  {
    return m_sections;
  }

  NODISCARD FORCEINLINE bool
  empty() const noexcept
  {
    return m_sections.empty();
  }

  void
  clear() noexcept
  {
    m_sections.clear();
  }

 private:
  NODISCARD Section&
  findOrAddSection(StringView name);

  Vector<Section> m_sections;
};

} // namespace chEngineSDK
