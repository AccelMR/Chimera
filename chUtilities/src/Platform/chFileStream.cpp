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

#include "chDebug.h"
#include "chStringUtils.h"
#include "chUnicode.h"

namespace chEngineSDK{

/**
 * @brief	Checks does the provided buffer has an UTF32 byte order mark in
 *        little endian order.
 */
bool
isUTF32LE( const char *buffer ) {
  return (0xFF == static_cast<uint8>(buffer[0]) &&
    0xFE == static_cast<uint8>(buffer[1]) &&
    0x00 == static_cast<uint8>(buffer[2]) &&
    0x00 == static_cast<uint8>(buffer[3]));
}

/**
 * @brief Checks does the provided buffer has an UTF32 byte order mark in
 *        big endian order.
 */
bool
isUTF32BE(const char* buffer) {
  return (0x00 == static_cast<uint8>(buffer[0]) &&
          0x00 == static_cast<uint8>(buffer[1]) &&
          0xFE == static_cast<uint8>(buffer[2]) &&
          0xFF == static_cast<uint8>(buffer[3]));
}

/**
 * @brief Checks does the provided buffer has an UTF16 byte order mark in
 *        little endian order.
 */
bool
isUTF16LE(const char* buffer) {
  return (0xFF == static_cast<uint8>(buffer[0]) &&
          0xFE == static_cast<uint8>(buffer[1]));
}

/**
 * @brief Checks does the provided buffer has an UTF16 byte order mark in
 *        big endian order.
 */
bool
isUTF16BE(const char* buffer) {
  return (0xFE == static_cast<uint8>(buffer[0]) &&
          0xFF == static_cast<uint8>(buffer[1]));
}

/**
 * @brief Checks does the provided buffer has an UTF8 byte order mark.
 */
bool
isUTF8(const char* buffer) {
  return (0xEF == static_cast<uint8>(buffer[0]) &&
          0xBB == static_cast<uint8>(buffer[1]) &&
          0xBF == static_cast<uint8>(buffer[2]));
}

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
  m_end = m_data + sourceStream->read(m_data, m_size);
  m_freeOnClose = true;

#if USING(CH_DEBUG_MODE)
  memset(m_data, 0, m_size);
#endif

  CH_ASSERT( m_end >= m_currPos );
}

/*
*/
MemoryDataStream::~MemoryDataStream() {
  close();
}

/*
*/
SIZE_T
MemoryDataStream::read(void* buf, SIZE_T size) {
  SIZE_T cnt = size;

  if (m_currPos + cnt > m_end) {
    cnt = m_end - m_currPos;
  }
  if (0 == cnt) {
    return 0;
  }

  CH_ASSERT(cnt <= size);

#if USING (CH_PLATFORM_WIN32)
  memcpy_s(buf, size, m_currPos, cnt);
#elif USING (CH_PLATFORM_LINUX)
  memcpy(buf, m_currPos, size);
#endif
  m_currPos += cnt;

  return cnt;
}

/*
*/
SIZE_T
MemoryDataStream::write(const void* buf, SIZE_T size) {
  SIZE_T written = 0;
  if (isWriteable()) {
    written = size;

    if (m_currPos + written > m_end) {
      written = m_end - m_currPos;
    }
    if (0 == written) {
      return 0;
    }
    memcpy(m_currPos, buf, size);

    m_currPos += written;
  }

  return written;
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
MemoryDataStream::close() {
  if (nullptr != m_data) {
    if (m_freeOnClose) {
      delete m_data;
    }
    m_data = nullptr;
  }
}

/*
*/
SPtr<DataStream>
MemoryDataStream::clone() const {
  return ch_shared_ptr_new<MemoryDataStream>(m_data, m_size, false);
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

  auto memStream = std::reinterpret_pointer_cast<MemoryDataStream>(sourceDataStream);
  CH_ASSERT(memStream);
  write(memStream->m_data, memStream->size());
}

FileDataStream::~FileDataStream() {
  close();
}

SIZE_T
FileDataStream::read(void* buf, SIZE_T count) {
  m_pInStream->read( static_cast<char *>(buf), static_cast<std::streamsize>(count) );
  return static_cast<SIZE_T>(m_pInStream->gcount());
}

SIZE_T
FileDataStream::write(const void* buf, SIZE_T count) {
  SIZE_T written = 0;
  if (isWriteable() && m_pFStream && m_pFStream->is_open()) {
    m_pFStream->write(static_cast<const char*>(buf), static_cast<std::streamsize>(count));
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
  return ch_shared_ptr_new<FileDataStream>(m_path, getAccessMode(), true);
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
    m_pFStream = ch_shared_ptr_new<std::fstream>();
    m_pFStream->open(m_path.m_path, mode);
    m_pInStream = m_pFStream;
  }
  else {
    m_pFStreamRO = ch_shared_ptr_new<std::ifstream>();
    m_pFStreamRO->open(m_path.m_path, mode);
    m_pInStream = m_pFStreamRO;
  }

  //Should check ensure open succeeded, in case fail for some reason.
  if (m_pInStream->fail()) {
    LOG_ERROR(StringUtils::format("Failed to open file: {0}", m_path.toString()));
    return;
  }

  m_pInStream->seekg(0, std::ios_base::end);
  m_size = static_cast<SIZE_T>(m_pInStream->tellg());
  m_pInStream->seekg(0, std::ios_base::beg);
}

/*
*/
String
DataStream::getAsString() {
 //Ensure read from begin of stream
  seek(0);
  
  //Try reading header
  uint8 headerBytes[4];
  SIZE_T numHeaderBytes = read(headerBytes, 4);
  
  SIZE_T dataOffset = 0;
  if (4 <= numHeaderBytes) {
    if (isUTF32LE(reinterpret_cast<char*>(headerBytes))) {
      dataOffset = 4;
    }
    else if (isUTF32BE(reinterpret_cast<char*>(headerBytes))) {
      std::u8string s;
      return String(s.begin(), s.end());
    }
  }
  
  if (0 == dataOffset && 3 <= numHeaderBytes) {
    if (isUTF8(reinterpret_cast<char*>(headerBytes))) {
      dataOffset = 3;
    }
  }
  
  if (0 == dataOffset && 2 <= numHeaderBytes) {
    if (isUTF16LE(reinterpret_cast<char*>(headerBytes))) {
      dataOffset = 2;
    }
    else if (isUTF16BE(reinterpret_cast<char*>(headerBytes))) {
      std::u8string s;
      return String(s.begin(), s.end());
    }
  }
  
  seek(dataOffset);
  
  //Read the entire buffer - ideally in one read, but if the size of the
  //buffer is unknown, do multiple fixed size reads.
  SIZE_T bufSize = (m_size > 0 ? m_size : 4096);
 
  auto* tempBuffer = static_cast<std::stringstream::char_type*>(malloc(bufSize));
  
  std::stringstream result;
  while (!isAtEnd()) {
    SIZE_T numReadBytes = read(tempBuffer, bufSize);
    result.write(tempBuffer, numReadBytes);
  }
 
  free(tempBuffer);
  
  std::string string = result.str();
  
  switch (dataOffset)
  {
    default:
    case 0: //No BOM = assumed UTF-8
    case 3: //UTF-8
      return String(string.data(), string.length());
    case 2: //UTF-16
      {
        SIZE_T numElems = string.length() / 2;
        return UTF8::fromUTF16(U16String(reinterpret_cast<char16_t*>(
                                          const_cast<char*>(string.data())),
                                         numElems));
      }
    case 4: //UTF-32
      {
        SIZE_T numElems = string.length() / 4;
        return UTF8::fromUTF32(U32String(reinterpret_cast<char32_t*>(
                                          const_cast<char*>(string.data())),
                                         numElems));
      }
  }
}

/*
*/
void
DataStream::writeString(const String& str, STRING_ENCODER encoder /*= STRING_ENCODER(STRING_ENCODER::kUTF8)*/) {
  if (encoder == STRING_ENCODER::kUTF16) {
    //Write BOM
    uint8 bom[2] = { 0xFF, 0xFE };
    write(bom, sizeof(bom));

    U16String u16string = UTF8::toUTF16(str);
    write(u16string.data(), u16string.length() * sizeof(char16_t));
  }
  else {
    // Write BOM
    //uint8 bom[3] = { 0xEF, 0xBB, 0xBF };
    //write(bom, sizeof(bom));
    write(str.data(), str.length());
  }
}

/*
*/
void
DataStream::writeString(const WString& wStr, STRING_ENCODER encoder /*= STRING_ENCODER(STRING_ENCODER::kUTF16)*/) {
  if ( STRING_ENCODER::kUTF16 == encoder ) {
    //Write BOM
    uint8 bom[2] = { 0xFF, 0xFE };
    write(bom, sizeof(bom));

    String u8string = UTF8::fromWide(wStr);
    U16String u16string = UTF8::toUTF16(u8string);
    write(u16string.data(), u16string.length() * sizeof(char16_t));
  }
  else {
    // Write BOM
    // uint8 bom[3] = { 0xEF, 0xBB, 0xBF };
    // write(bom, sizeof(bom));

    String u8string = UTF8::fromWide(wStr);
    write(u8string.data(), u8string.length());
  }
}

}