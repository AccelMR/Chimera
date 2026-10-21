/************************************************************************/
/**
 * @file chConfigFile.cpp
 * @author AccelMR
 * @date 2026/10/06
 * @brief Reads and writes .ini files.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chConfigFile.h"

#include "chFileSystem.h"
#include "chLogger.h"
#include "chPath.h"
#include "chStringUtils.h"

namespace chEngineSDK {

CH_LOG_DECLARE_STATIC(ConfigFileLog, All);

namespace {

NODISCARD bool
needsQuotes(StringView value) noexcept
{
  if (value.empty()) {
    return false;
  }
  return StringUtils::trimView(value).size() != value.size() || '"' == value.front();
}

} // namespace

/*
 */
bool
ConfigFile::load(const Path& path)
{
  if (!FileSystem::isFile(path)) {
    return false;
  }

  SPtr<DataStream> stream = FileSystem::openFile(path);
  if (!stream) {
    return false;
  }
  const String text = stream->getAsString();
  stream->close();

  parse(text, path.toString());
  return true;
}

/*
 */
void
ConfigFile::parse(StringView text, StringView sourceName)
{
  StringView section;
  uint32 lineNumber = 0;
  while (!text.empty()) {
    const SIZE_T lineEnd = text.find('\n');
    StringView line = StringUtils::trimView(text.substr(0, lineEnd));
    text = StringView::npos == lineEnd ? StringView() : text.substr(lineEnd + 1);
    ++lineNumber;

    if (line.empty() || ';' == line.front() || '#' == line.front()) {
      continue;
    }

    if ('[' == line.front()) {
      if (']' != line.back()) {
        CH_LOG_WARNING(ConfigFileLog, "{0}({1}): missing ']' in '{2}'.", sourceName,
                       lineNumber, line);
        continue;
      }
      section = StringUtils::trimView(line.substr(1, line.size() - 2));
      continue;
    }

    const SIZE_T equals = line.find('=');
    const StringView key = StringView::npos == equals
                               ? StringView()
                               : StringUtils::trimView(line.substr(0, equals));
    if (key.empty()) {
      CH_LOG_WARNING(ConfigFileLog, "{0}({1}): expected Key=Value, found '{2}'.", sourceName,
                     lineNumber, line);
      continue;
    }

    StringView value = StringUtils::trimView(line.substr(equals + 1));
    if (value.size() >= 2 && '"' == value.front() && '"' == value.back()) {
      value = value.substr(1, value.size() - 2);
    }
    setValue(section, key, value);
  }
}

/*
 */
void
ConfigFile::merge(const ConfigFile& other)
{
  for (const Section& section : other.m_sections) {
    for (const Entry& entry : section.entries) {
      setValue(section.name, entry.key, entry.value);
    }
  }
}

/*
 */
bool
ConfigFile::save(const Path& path) const
{
  SPtr<DataStream> stream = FileSystem::createAndOpenFile(path);
  if (!stream) {
    return false;
  }

  const String text = toText();
  const SIZE_T written = stream->write(text.data(), text.size());
  stream->close();
  return written == text.size();
}

/*
 */
String
ConfigFile::toText() const
{
  String text;
  for (const Section& section : m_sections) {
    if (section.entries.empty()) {
      continue;
    }

    if (!section.name.empty()) {
      if (!text.empty()) {
        text += '\n';
      }
      text += '[';
      text += section.name;
      text += "]\n";
    }

    for (const Entry& entry : section.entries) {
      text += entry.key;
      text += '=';
      if (needsQuotes(entry.value)) {
        text += '"';
        text += entry.value;
        text += '"';
      }
      else {
        text += entry.value;
      }
      text += '\n';
    }
  }
  return text;
}

/*
 */
const String*
ConfigFile::getValue(StringView section, StringView key) const noexcept
{
  for (const Section& current : m_sections) {
    if (!StringUtils::equalsIgnoreCase(current.name, section)) {
      continue;
    }
    for (const Entry& entry : current.entries) {
      if (StringUtils::equalsIgnoreCase(entry.key, key)) {
        return &entry.value;
      }
    }
    return nullptr;
  }
  return nullptr;
}

/*
 */
void
ConfigFile::setValue(StringView section, StringView key, StringView value)
{
  Section& target = findOrAddSection(section);
  for (Entry& entry : target.entries) {
    if (StringUtils::equalsIgnoreCase(entry.key, key)) {
      entry.value = value;
      return;
    }
  }
  target.entries.push_back({.key = String(key), .value = String(value)});
}

/*
 */
ConfigFile::Section&
ConfigFile::findOrAddSection(StringView name)
{
  for (Section& section : m_sections) {
    if (StringUtils::equalsIgnoreCase(section.name, name)) {
      return section;
    }
  }
  // Entries without a section are written first, before any header.
  if (name.empty()) {
    m_sections.insert(m_sections.begin(), Section{.name = String(), .entries = {}});
    return m_sections.front();
  }
  return m_sections.emplace_back(Section{.name = String(name), .entries = {}});
}

} // namespace chEngineSDK
