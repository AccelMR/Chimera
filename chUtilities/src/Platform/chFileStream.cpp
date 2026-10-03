/************************************************************************/
/**
 * @file chFileStream.cpp
 * @author AccelMR
 * @date 2022/08/26
 *
 * @brief Handles read and write from files steams.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chFileStream.h"

#include <cstring>
#include <fstream>

#include "chLogger.h"
#include "chMath.h"
#include "chStringUtils.h"
#include "chUnicode.h"

namespace chEngineSDK{

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

/**
 * Text without a mark is read as UTF-8. The UTF-32 LE mark starts with the UTF-16 LE one,
 * so it is checked first.
 */
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

/**
 * Builds each unit from its bytes in the file's order, so it works on any machine
 * whatever its own byte order is. A trailing incomplete unit is dropped.
 */
template<typename UnitString>
NODISCARD UnitString
unitsFromBytes(const String& bytes, bool bigEndian)
{
  using Unit = typename UnitString::value_type;
  constexpr SIZE_T unitSize = sizeof(Unit);

  UnitString units;
  units.resize(bytes.size() / unitSize);
  for (SIZE_T i = 0; i < units.size(); ++i) {
    uint32 value = 0;
    for (SIZE_T b = 0; b < unitSize; ++b) {
      const uint32 byte = static_cast<uint8>(bytes[i * unitSize + b]);
      const SIZE_T shift = (bigEndian ? unitSize - 1 - b : b) * 8;
      value |= byte << shift;
    }
    units[i] = static_cast<Unit>(value);
  }
  return units;
}

} // namespace

/*
*/
MemoryDataStream::MemoryDataStream(SIZE_T _size)
  : DataStream(ACCESS_MODE::kREAD | ACCESS_MODE::kWRITE),
    m_data(nullptr),
    m_freeOnClose(true) {
  m_data = m_currPos = reinterpret_cast<uint8*>(malloc(_size));
  m_size = _size;
  m_end = m_data + m_size;

  CH_ASSERT(m_end >= m_currPos);
}

/*
*/
MemoryDataStream::MemoryDataStream(void* memory, SIZE_T _size, bool _freeOnClose /*= true*/)
  : DataStream(ACCESS_MODE::kREAD | ACCESS_MODE::kWRITE),
    m_data(nullptr),
    m_freeOnClose(_freeOnClose)
{
  m_data = m_currPos = static_cast<uint8*>(memory);
  m_size = _size;
  m_end = m_data + m_size;

  CH_ASSERT(m_end >= m_currPos);
}

/*
*/
MemoryDataStream::MemoryDataStream(const SPtr<DataStream>& sourceStream)
  : DataStream(ACCESS_MODE::kREAD | ACCESS_MODE::kWRITE),
    m_data(nullptr) {
  //Copy data from incoming stream
  m_size = sourceStream->size();

  m_data = reinterpret_cast<uint8*>(malloc(m_size));
  m_currPos = m_data;
  m_size = sourceStream->read(m_data, m_size);
  m_end = m_data + m_size;
  m_freeOnClose = true;
}

/*
*/
MemoryDataStream::~MemoryDataStream() {
  close();
}

/*
*/
SIZE_T
MemoryDataStream::read(void* buf, SIZE_T size)
{
  const SIZE_T count = Math::min(size, static_cast<SIZE_T>(m_end - m_currPos));
  if (0 == count) {
    return 0;
  }

  memcpy(buf, m_currPos, count);
  m_currPos += count;
  return count;
}

/*
*/
SIZE_T
MemoryDataStream::write(const void* buf, SIZE_T size)
{
  if (!isWriteable()) {
    return 0;
  }

  const SIZE_T count = Math::min(size, static_cast<SIZE_T>(m_end - m_currPos));
  if (0 == count) {
    return 0;
  }

  memcpy(m_currPos, buf, count);
  m_currPos += count;
  return count;
}

/*
*/
void
MemoryDataStream::skip(SIZE_T count) {
  SIZE_T newpos = static_cast<SIZE_T>((m_currPos - m_data) + count);
  CH_ASSERT( m_data + newpos <= m_end );
  m_currPos = m_data + newpos;
}

/*
*/
void
MemoryDataStream::seek(SIZE_T pos) {
  CH_ASSERT(m_data + pos <= m_end);
  m_currPos = m_data + pos;
}

/*
*/
SIZE_T
MemoryDataStream::tell() const {
  return m_currPos - m_data;
}

/*
*/
bool
MemoryDataStream::isAtEnd() const {
  return m_currPos >= m_end;
}

/*
*/
void
MemoryDataStream::close()
{
  if (nullptr != m_data) {
    if (m_freeOnClose) {
      free(m_data);
    }
    m_data = m_currPos = m_end = nullptr;
  }
}

/*
*/
SPtr<DataStream>
MemoryDataStream::clone() const
{
  // The copy owns its data, so it stays valid after this stream is closed.
  auto* copy = static_cast<uint8*>(malloc(m_size));
  if (m_size > 0) {
    memcpy(copy, m_data, m_size);
  }
  return chMakeShared<MemoryDataStream>(copy, m_size, true);
}

/*
*/
FileDataStream::FileDataStream(const Path&_path,
                               AccesModeFlag _accessMode /*= AccesModeFlag(ACCESS_MODE::kREAD)*/,
                               bool _freeOnClose /*= true*/ )
  : DataStream(_accessMode),
    m_path(_path),
    m_freeOnClose(_freeOnClose) {
  init();
}

/*
*/
FileDataStream::FileDataStream(const Path& _path, const SPtr<DataStream>& sourceDataStream)
  : DataStream(ACCESS_MODE::kWRITE),
    m_path(_path),
    m_freeOnClose(true) {
  CH_ASSERT(sourceDataStream->isReadable());
  init();
  if (!isOpen()) {
    return;
  }

  // Copies through the DataStream interface, so any kind of stream can be the source.
  constexpr SIZE_T kChunkSize = 64 * 1024;
  Vector<uint8> chunk(kChunkSize);
  sourceDataStream->seek(0);
  while (true) {
    const SIZE_T readBytes = sourceDataStream->read(chunk.data(), kChunkSize);
    if (0 == readBytes) {
      break;
    }
    write(chunk.data(), readBytes);
  }
}

FileDataStream::~FileDataStream() {
  close();
}

SIZE_T
FileDataStream::read(void* buf, SIZE_T count) {
  m_pInStream->read(static_cast<ANSICHAR *>(buf), static_cast<std::streamsize>(count));
  return static_cast<SIZE_T>(m_pInStream->gcount());
}

SIZE_T
FileDataStream::write(const void* buf, SIZE_T count) {
  SIZE_T written = 0;
  if (isWriteable() && m_pFStream && m_pFStream->is_open()) {
    m_pFStream->write(static_cast<const ANSICHAR*>(buf), static_cast<std::streamsize>(count));
    written = count;
  }
  return written;
}

/*
*/
void
FileDataStream::skip(SIZE_T count) {
    m_pInStream->clear(); //Clear fail status in case eof was set
    m_pInStream->seekg(static_cast<std::ifstream::pos_type>(count), std::ios::cur);
}

/*
*/
void
FileDataStream::seek(SIZE_T pos) {
  m_pInStream->clear();	//Clear fail status in case eof was set
  m_pInStream->seekg( static_cast<std::streamoff>(pos), std::ios::beg );
}

/*
*/
SIZE_T
FileDataStream::tell() const {
  m_pInStream->clear(); //Clear fail status in case eof was set
  return static_cast<SIZE_T>(m_pInStream->tellg());
}

/*
*/
bool
FileDataStream::isAtEnd() const {
  return m_pInStream->eof();
}

/*
*/
SPtr<DataStream>
FileDataStream::clone() const {
  return chMakeShared<FileDataStream>(m_path, getAccessMode(), true);
}

/*
*/
void
FileDataStream::flush()
{
  if (m_pFStream && m_pFStream->is_open()) {
    m_pFStream->flush();
  }
}

/*
*/
void
FileDataStream::close() {
  if (m_pInStream) {
    if (m_pFStreamRO) {
      m_pFStreamRO->close();
    }

    if (m_pFStream) {
      m_pFStream->flush();
      m_pFStream->close();
    }

    if (m_freeOnClose) {
      m_pInStream = nullptr;
      m_pFStreamRO = nullptr;
      m_pFStream = nullptr;
    }
  }
}

/*
*/
bool
FileDataStream::isOpen() const {
  const bool fStreamopen = (nullptr != m_pFStream) && m_pFStream->is_open();
  const bool oStreamopen = (nullptr != m_pFStreamRO) && m_pFStreamRO->is_open();
  return fStreamopen ||oStreamopen;
}

/*
*/
void
FileDataStream::init() {
  //Always open in binary mode. Also, always include reading
  std::ios::openmode mode = std::fstream::binary;

  if (m_accessMode.isSetAny(ACCESS_MODE::kREAD)) {
    mode |= std::fstream::in;
  }

  if (m_accessMode.isSetAny(ACCESS_MODE::kWRITE)) {
    mode |= std::fstream::out;
    m_pFStream = chMakeShared<std::fstream>();
    m_pFStream->open(m_path.m_path, mode);
    m_pInStream = m_pFStream;
  }
  else {
    m_pFStreamRO = chMakeShared<std::ifstream>();
    m_pFStreamRO->open(m_path.m_path, mode);
    m_pInStream = m_pFStreamRO;
  }

  // Nothing is logged here because the Logger opens its own file through this class.
  // The streams are kept when opening fails, so reads return 0 instead of crashing;
  // callers check isOpen().
  if (m_pInStream->fail()) {
    return;
  }

  m_pInStream->seekg(0, std::ios_base::end);
  m_size = static_cast<SIZE_T>(m_pInStream->tellg());
  m_pInStream->seekg(0, std::ios_base::beg);
}

/*
*/
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
    while (!isAtEnd()) {
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

/*
*/
void
DataStream::writeString(const String& str, STRING_ENCODER encoder)
{
  // UTF-16 is written with a little endian mark and the units as they are in memory,
  // which is little endian on every supported platform. UTF-8 is written without a mark.
  if (STRING_ENCODER::kUTF16 == encoder) {
    write(kUTF16LEMark, sizeof(kUTF16LEMark));
    const U16String u16string = UTF8::toUTF16(str);
    write(u16string.data(), u16string.length() * sizeof(WCHAR16));
  }
  else {
    write(str.data(), str.length());
  }
}

/*
*/
void
DataStream::writeString(const WString& wStr, STRING_ENCODER encoder)
{
  writeString(UTF8::fromWide(wStr), encoder);
}

}
