/************************************************************************/
/**
 * @file chFileStream.cpp
 * @author AccelMR
 * @date 2022/08/26
 * @brief Streams that read and write bytes from memory or from a file.
 */
/************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chFileStream.h"

#include <cstdlib>
#include <cstring>
#include <fstream>

#include "chMath.h"
#include "chUnicode.h"

namespace chEngineSDK {

namespace {

enum class TextEncoding : uint8
{
  UTF8,
  UTF16LE,
  UTF16BE,
  UTF32LE,
  UTF32BE
};

struct ByteOrderMark
{
  TextEncoding encoding = TextEncoding::UTF8;
  SIZE_T size = 0;
};

constexpr uint8 kUTF8Mark[] = {0xEF, 0xBB, 0xBF};
constexpr uint8 kUTF16LEMark[] = {0xFF, 0xFE};
constexpr uint8 kUTF16BEMark[] = {0xFE, 0xFF};
constexpr uint8 kUTF32LEMark[] = {0xFF, 0xFE, 0x00, 0x00};
constexpr uint8 kUTF32BEMark[] = {0x00, 0x00, 0xFE, 0xFF};

// First allocation of a growing memory stream.
constexpr SIZE_T kMinGrowCapacity = 256;

// Buffer used to copy a stream into a file.
constexpr SIZE_T kCopyChunkSize = 64 * 1024;

template<SIZE_T N>
NODISCARD bool
startsWith(const uint8* bytes, SIZE_T count, const uint8 (&mark)[N]) noexcept
{
  if (count < N) {
    return false;
  }
  for (SIZE_T i = 0; i < N; ++i) {
    if (bytes[i] != mark[i]) {
      return false;
    }
  }
  return true;
}

// The UTF-32 LE mark starts with the UTF-16 LE one, so it is checked first.
NODISCARD ByteOrderMark
detectByteOrderMark(const uint8* bytes, SIZE_T count) noexcept
{
  if (startsWith(bytes, count, kUTF32LEMark)) {
    return {TextEncoding::UTF32LE, sizeof(kUTF32LEMark)};
  }
  if (startsWith(bytes, count, kUTF32BEMark)) {
    return {TextEncoding::UTF32BE, sizeof(kUTF32BEMark)};
  }
  if (startsWith(bytes, count, kUTF8Mark)) {
    return {TextEncoding::UTF8, sizeof(kUTF8Mark)};
  }
  if (startsWith(bytes, count, kUTF16LEMark)) {
    return {TextEncoding::UTF16LE, sizeof(kUTF16LEMark)};
  }
  if (startsWith(bytes, count, kUTF16BEMark)) {
    return {TextEncoding::UTF16BE, sizeof(kUTF16BEMark)};
  }
  return {};
}

// Builds each unit from its bytes in the file's order, so it works whatever the byte
// order of the machine is. A trailing incomplete unit is dropped.
template<typename UnitString>
NODISCARD UnitString
unitsFromBytes(const String& bytes, bool bBigEndian)
{
  using Unit = typename UnitString::value_type;
  constexpr SIZE_T unitSize = sizeof(Unit);

  UnitString units;
  units.resize(bytes.size() / unitSize);
  for (SIZE_T i = 0; i < units.size(); ++i) {
    uint32 value = 0;
    for (SIZE_T b = 0; b < unitSize; ++b) {
      const uint32 byte = static_cast<uint8>(bytes[i * unitSize + b]);
      const SIZE_T shift = (bBigEndian ? unitSize - 1 - b : b) * 8;
      value |= byte << shift;
    }
    units[i] = static_cast<Unit>(value);
  }
  return units;
}

} // namespace

String
DataStream::getAsString()
{
  seek(0);
  uint8 header[sizeof(kUTF32LEMark)];
  const ByteOrderMark mark = detectByteOrderMark(header, read(header, sizeof(header)));
  seek(mark.size);

  // UTF-8 text is read straight into the result, so it is copied only once.
  String bytes;
  if (m_size > 0) {
    if (m_size > mark.size) {
      bytes.resize(m_size - mark.size);
      bytes.resize(read(bytes.data(), bytes.size()));
    }
  }
  else {
    // A stream that does not know its size is read in chunks.
    ANSICHAR chunk[4096];
    while (true) {
      const SIZE_T readBytes = read(chunk, sizeof(chunk));
      if (0 == readBytes) {
        break;
      }
      bytes.append(chunk, readBytes);
    }
  }

  switch (mark.encoding) {
  case TextEncoding::UTF16LE:
  case TextEncoding::UTF16BE:
    return UTF8::fromUTF16(
        unitsFromBytes<U16String>(bytes, TextEncoding::UTF16BE == mark.encoding));
  case TextEncoding::UTF32LE:
  case TextEncoding::UTF32BE:
    return UTF8::fromUTF32(
        unitsFromBytes<U32String>(bytes, TextEncoding::UTF32BE == mark.encoding));
  case TextEncoding::UTF8:
  default:
    return bytes;
  }
}

MemoryDataStream::MemoryDataStream() noexcept
 : DataStream(AccessMode::Read | AccessMode::Write),
   m_bGrowable(true)
{}

MemoryDataStream::MemoryDataStream(SIZE_T size)
 : DataStream(AccessMode::Read | AccessMode::Write)
{
  m_data = static_cast<uint8*>(malloc(size));
  if (m_data) {
    m_size = size;
  }
  m_currPos = m_data;
  m_end = m_data + m_size;
}

MemoryDataStream::MemoryDataStream(void* memory, SIZE_T size, bool bFreeOnClose) noexcept
 : DataStream(AccessMode::Read | AccessMode::Write),
   m_data(static_cast<uint8*>(memory)),
   m_currPos(m_data),
   m_end(m_data + size),
   m_bFreeOnClose(bFreeOnClose)
{
  m_size = size;
}

MemoryDataStream::MemoryDataStream(const SPtr<DataStream>& sourceStream)
 : DataStream(AccessMode::Read | AccessMode::Write)
{
  const SIZE_T sourceLeft = sourceStream->size() - Math::min(sourceStream->tell(),
                                                             sourceStream->size());
  m_data = static_cast<uint8*>(malloc(sourceLeft));
  if (m_data) {
    m_size = sourceStream->read(m_data, sourceLeft);
  }
  m_currPos = m_data;
  m_end = m_data + m_size;
}

MemoryDataStream::MemoryDataStream(MemoryDataStream&& other) noexcept
 : DataStream(other.m_accessMode),
   m_data(other.m_data),
   m_currPos(other.m_currPos),
   m_end(other.m_end),
   m_capacity(other.m_capacity),
   m_bFreeOnClose(other.m_bFreeOnClose),
   m_bGrowable(other.m_bGrowable)
{
  m_size = other.m_size;
  other.m_data = other.m_currPos = other.m_end = nullptr;
  other.m_capacity = 0;
  other.m_size = 0;
}

MemoryDataStream::~MemoryDataStream()
{
  close();
}

MemoryDataStream&
MemoryDataStream::operator=(MemoryDataStream&& other) noexcept
{
  if (this != &other) {
    close();
    m_accessMode = other.m_accessMode;
    m_size = other.m_size;
    m_data = other.m_data;
    m_currPos = other.m_currPos;
    m_end = other.m_end;
    m_capacity = other.m_capacity;
    m_bFreeOnClose = other.m_bFreeOnClose;
    m_bGrowable = other.m_bGrowable;
    other.m_data = other.m_currPos = other.m_end = nullptr;
    other.m_capacity = 0;
    other.m_size = 0;
  }
  return *this;
}

SIZE_T
MemoryDataStream::read(void* buffer, SIZE_T size)
{
  const SIZE_T count = Math::min(size, static_cast<SIZE_T>(m_end - m_currPos));
  if (0 == count) {
    return 0;
  }

  memcpy(buffer, m_currPos, count);
  m_currPos += count;
  return count;
}

SIZE_T
MemoryDataStream::write(const void* buffer, SIZE_T size)
{
  if (!isWriteable()) {
    return 0;
  }

  if (m_bGrowable) {
    return writeGrowing(buffer, size);
  }

  const SIZE_T count = Math::min(size, static_cast<SIZE_T>(m_end - m_currPos));
  if (0 == count) {
    return 0;
  }

  memcpy(m_currPos, buffer, count);
  m_currPos += count;
  return count;
}

SIZE_T
MemoryDataStream::writeGrowing(const void* buffer, SIZE_T size)
{
  if (0 == size) {
    return 0;
  }

  const SIZE_T position = static_cast<SIZE_T>(m_currPos - m_data);
  const SIZE_T needed = position + size;
  if (needed > m_capacity) {
    // Doubling keeps the number of copies small for data written in many small pieces.
    const SIZE_T capacity = Math::max(needed, Math::max(m_capacity * 2, kMinGrowCapacity));
    uint8* data = static_cast<uint8*>(realloc(m_data, capacity));
    if (!data) {
      return 0;
    }
    const SIZE_T written = static_cast<SIZE_T>(m_end - m_data);
    m_data = data;
    m_currPos = m_data + position;
    m_end = m_data + written;
    m_capacity = capacity;
  }

  memcpy(m_currPos, buffer, size);
  m_currPos += size;
  if (m_currPos > m_end) {
    m_end = m_currPos;
    m_size = static_cast<SIZE_T>(m_end - m_data);
  }
  return size;
}

void
MemoryDataStream::seek(SIZE_T pos)
{
  // Clamped, because a read point past the end would make the bytes left negative.
  CH_ASSERT(pos <= m_size);
  m_currPos = m_data + Math::min(pos, m_size);
}

SIZE_T
MemoryDataStream::tell() const
{
  return static_cast<SIZE_T>(m_currPos - m_data);
}

bool
MemoryDataStream::isAtEnd() const
{
  return m_currPos >= m_end;
}

void
MemoryDataStream::close()
{
  if (m_data && m_bFreeOnClose) {
    free(m_data);
  }
  m_data = m_currPos = m_end = nullptr;
  m_capacity = 0;
  m_size = 0;
}

SPtr<DataStream>
MemoryDataStream::clone() const
{
  auto* copy = static_cast<uint8*>(malloc(m_size));
  if (!copy) {
    return chMakeShared<MemoryDataStream>(nullptr, 0, false);
  }
  memcpy(copy, m_data, m_size);
  return chMakeShared<MemoryDataStream>(copy, m_size, true);
}

FileDataStream::FileDataStream(const Path& path, AccessModeFlags accessMode)
 : DataStream(accessMode),
   m_path(path),
   m_stream(new std::fstream())
{
  std::ios::openmode mode = std::ios::binary;
  if (isReadable()) {
    mode |= std::ios::in;
  }
  if (isWriteable()) {
    mode |= std::ios::out;
  }

  // The wide form on Windows, so paths with characters outside ASCII open the right file.
  m_stream->open(m_path.getPlatformString().c_str(), mode);

  // Nothing is logged here because the Logger opens its own file through this class.
  if (!m_stream->is_open()) {
    m_stream.reset();
    return;
  }

  if (isReadable()) {
    m_stream->seekg(0, std::ios::end);
    const std::streamoff end = m_stream->tellg();
    m_size = end > 0 ? static_cast<SIZE_T>(end) : 0;
    m_stream->seekg(0, std::ios::beg);
  }
}

FileDataStream::FileDataStream(const Path& path, const SPtr<DataStream>& sourceDataStream)
 : FileDataStream(path, AccessMode::Write)
{
  CH_ASSERT(sourceDataStream->isReadable());
  if (!isOpen()) {
    return;
  }

  Vector<uint8> chunk(kCopyChunkSize);
  sourceDataStream->seek(0);
  while (true) {
    const SIZE_T readBytes = sourceDataStream->read(chunk.data(), kCopyChunkSize);
    if (0 == readBytes) {
      break;
    }
    if (write(chunk.data(), readBytes) != readBytes) {
      close();
      return;
    }
  }
}

FileDataStream::~FileDataStream()
{
  close();
}

SIZE_T
FileDataStream::read(void* buffer, SIZE_T size)
{
  if (!m_stream || !isReadable()) {
    return 0;
  }
  m_stream->read(static_cast<ANSICHAR*>(buffer), static_cast<std::streamsize>(size));
  return static_cast<SIZE_T>(m_stream->gcount());
}

SIZE_T
FileDataStream::write(const void* buffer, SIZE_T size)
{
  if (!m_stream || !isWriteable()) {
    return 0;
  }

  // A read that reached the end leaves the stream failed, which would refuse every write.
  m_stream->clear();
  m_stream->write(static_cast<const ANSICHAR*>(buffer), static_cast<std::streamsize>(size));
  return m_stream->fail() ? 0 : size;
}

void
FileDataStream::seek(SIZE_T pos)
{
  if (!m_stream) {
    return;
  }
  m_stream->clear();
  m_stream->seekg(static_cast<std::streamoff>(pos), std::ios::beg);
}

SIZE_T
FileDataStream::tell() const
{
  if (!m_stream) {
    return 0;
  }
  m_stream->clear();
  const std::streamoff pos = m_stream->tellg();
  return pos > 0 ? static_cast<SIZE_T>(pos) : 0;
}

bool
FileDataStream::isAtEnd() const
{
  return !m_stream || tell() >= m_size;
}

SPtr<DataStream>
FileDataStream::clone() const
{
  if (m_stream) {
    m_stream->flush();
  }
  return chMakeShared<FileDataStream>(m_path, AccessMode::Read);
}

void
FileDataStream::close()
{
  // std::fstream::close writes what is still buffered.
  if (m_stream) {
    m_stream->close();
    m_stream.reset();
  }
}

void
FileDataStream::flush()
{
  if (m_stream) {
    m_stream->flush();
  }
}

bool
FileDataStream::isOpen() const
{
  return m_stream && m_stream->is_open();
}

} // namespace chEngineSDK
