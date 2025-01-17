/************************************************************************/
/**
 * @file chFileStream.h
 * @author AccelMR
 * @date 2022/08/26
 *   Handles read and write from files steams.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */                                                                     
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chPath.h"
#include "chFlags.h"

namespace chEngineSDK{
/*
 * Description: 
 *     Enum class to be used for any stream data and how it is going to be accessed. 
 *
 *  NOTE: This must never be used alone. Use it through AccessModeFlag.
 * 
 * Sample usage:
 *  AccessModeFlag myFlag(ACCESS_MODE::kREAD | ACCESS_MODE::kWRITE);
 */
enum class ACCESS_MODE: uint16 {
  kNONE  = 0x00,
  kREAD  = 0x01,
  kWRITE = 0x02
};
CH_FLAGS_OPERATORS_EXT(ACCESS_MODE, uint16);
using AccesModeFlag = Flags<ACCESS_MODE, uint16>;

enum class STRING_ENCODER {
  kUTF8,
  kUTF16
};

/*
 * Description: 
 *     Data Stream interface class.
 *
 * Sample usage:
 *    Interface classes are not to be used as an object itself.
 */
class CH_UTILITY_EXPORT DataStream {
 public:
  /** 
   *   Base constructor.
   * 
   * @param _accessMode
   *    Flag access, how this stream is going to be treated.
   **/
  DataStream(AccesModeFlag _accessMode = AccesModeFlag(ACCESS_MODE::kREAD))
    : m_accessMode(_accessMode)
  {}

  /** 
   *   Default destructor.
   **/
  virtual ~DataStream() = default;

  /** 
   *   Returns access mode of this data stream.
   **/
  FORCEINLINE AccesModeFlag
  getAccessMode() const {
    return m_accessMode;
  }

  /** 
   *   True if this stream is readable.
   **/
  FORCEINLINE virtual bool
  isReadable() const {
    return m_accessMode.isSetAny(ACCESS_MODE::kREAD);
  }

  /** 
   *   True if this stream is writable.
   **/
  FORCEINLINE virtual bool
  isWriteable() const {
    return m_accessMode.isSetAny(ACCESS_MODE::kWRITE);
  }

  /** 
   *   Returns content as string.
   **/
  virtual String
  getAsString();
  
  /**
   * @brief Reads data from the buffer and copies it to the specified value.
   */
  template<typename T>
  FORCEINLINE const DataStream&
  operator>>(T& val) {
    CH_ASSERT(isReadable()); //Debug only
    read(static_cast<void*>(&val), sizeof(T));
    return *this;
  }

  /** 
   *   So we can use >> with Sptr
   **/
  template<typename T>
  friend const SPtr<DataStream>&
  operator>>(const SPtr<DataStream> &ds, T& val) {
    *ds >> val;
    return ds;
  }

  /**
   * @brief Reads data from the buffer and copies it to the specified value.
   */
  template<typename T>
  FORCEINLINE DataStream&
  operator<<(const T& val) {
    CH_ASSERT(isWriteable()); //Debug only
    write(static_cast<const void*>(&val), sizeof(T));
    return *this;
  }
  
  /** 
   *   So we can use << with Sptr
   **/
  template<typename T>
  friend const SPtr<DataStream>& 
  operator<<(const SPtr<DataStream>& ds, const T& val) {
    *ds << val;
    return ds;
  }

  /** 
   *   Specific for string behavior.
   **/
  friend SPtr<DataStream>& 
  operator<<(SPtr<DataStream>& ds, const String& val) {
    ds->writeString(val);
    return ds;
  }
  
  /** 
   *   Specific for string behavior.
   **/
  friend SPtr<DataStream>& 
  operator<<(SPtr<DataStream>& ds, const char* val) {
    ds->writeString(String(val));
    return ds;
  }

  /**
   * @brief Overload for const char* to handle string literals directly.
   */
  FORCEINLINE DataStream&
  operator<<(const char* val) {
      writeString(String(val));
      return *this;
  }

  /** 
   *   True if this stream is file.
   **/
  virtual bool
  isFile() const = 0;

  /** 
   *   Reads the requested number of bytes from the stream. 
   * 
   * @param buf
   *    The buffer where the info is going to be written.
   * 
   * @param size
   *    Size of the buffer where this is going to write.
   * 
   * @return DataStream&
   *    Reference to this.
   **/
  virtual SIZE_T
  read(void* buf, SIZE_T size) = 0;

  /** 
   *   Writes the requested number of bytes into the stream.
   * 
   * @param buf
   *    The buffer to copy into the stream.
   * 
   * @param size
   *    Size of the buffer to be written.
   *
   * @return DataStream&
   *    Reference to this.
   **/
  virtual SIZE_T
  write(const void* buf, SIZE_T size) = 0;

  /** 
   *   Writes a String into the file, user proper formatting.
   * 
   * @param str
   *    String to be written.
   * 
   * @param encoder
   *  The encoder to be used, UTF8 by default.
   **/
  virtual void
  writeString(const String& str, STRING_ENCODER encoder = STRING_ENCODER(STRING_ENCODER::kUTF8));

  /** 
   *   Writes a Wide String into the file, user proper formatting.
   * 
   * @param wStr
   *    Wide String to be written.
   * 
   * @param encoder
   *  The encoder to be used, UTF16 by default.
   **/
  virtual void
  writeString(const WString& wStr, STRING_ENCODER encoder = STRING_ENCODER(STRING_ENCODER::kUTF16));

  /** 
   *  Skip a defined number of bytes. This can also be a negative value,
   *  in which case the file pointer rewinds a defined number of bytes.
   **/
  virtual void
  skip(SIZE_T count) = 0;

  /**
   *  Repositions the read point to a specified byte.
   */
  virtual void
  seek(SIZE_T pos) = 0;
  
  /** 
   *   Returns the current byte offset from beginning.
   **/
  virtual SIZE_T
  tell() const = 0;

  /** 
   *   True if the stream is at the end.
   **/
  virtual bool
  isAtEnd() const = 0;
  
  /**
   * @brief Close the stream. This makes further operations invalid.
   */
  virtual void
  close() = 0;

  /**
   *    Creates a copy of this stream.
   */
  virtual SPtr<DataStream>
  clone() const = 0;

  /** 
   *   Returns the size of this Stream.
   **/
  FORCEINLINE SIZE_T
  size() const { return m_size; }

 protected:
  SIZE_T m_size = 0;
  AccesModeFlag m_accessMode;
};

/*
 * Description: 
 *     Data stream for handling data from memory.
 *
 */
class CH_UTILITY_EXPORT MemoryDataStream: public DataStream
{
 public:
  /**
   *   Default constructor
   */
  MemoryDataStream(SIZE_T _size);

  /** 
   *   Constructor from a already created chunk of memory. 
   * 
   * @param memory
   *   Pointer to the chunk of memory to store.
   * 
   * @param _size
   *    Size of the chunk of memory.
   * 
   * @param  _freeOnClose
   *    If this Memory stream should delete memory pointer on close or 
   *    something else will take care of freeing memory.
   **/
  MemoryDataStream(void* memory, SIZE_T _size, bool _freeOnClose = true);

  
  /**
   *    Creates a stream which pre-buffers the contents of another stream.
   *        Data from the other buffer will be entirely read and stored in an
   *        internal buffer.
   * @param sourceStream    
   *   Stream to read data from.
   */
  explicit MemoryDataStream(const SPtr<DataStream>& sourceStream);

  /**
   *   Default destructor
   */
  ~MemoryDataStream();

  /** 
   *   Returns true if is file.
   **/
  FORCEINLINE bool
  isFile() const override {
    return false;
  }

  /** 
   *   Returns the pointer to the start data of this memory.
   **/
  FORCEINLINE uint8*
  getStartPtr() const {
    return m_data;
  }

  /** 
   *   Returns the pointer to the current data.
   **/
  FORCEINLINE uint8*
  getCurrentPtr() const {
    return m_currPos;
  }
 
  /** 
   *   Reads the requested number of bytes from the stream. 
   * 
   * @param buf
   *    The buffer where the info is going to be written.
   * 
   * @param size
   *    Size of the buffer where this is going to write.
   * 
   * @return DataStream&
   *    Reference to this.
   **/
  SIZE_T
  read(void* buf, SIZE_T size) override;

  /** 
   *   Writes the requested number of bytes into the stream.
   * 
   * @param buf
   *    The buffer to copy into the stream.
   * 
   * @param size
   *    Size of the buffer to be written.
   *
   * @return DataStream&
   *    Reference to this.
   **/
  SIZE_T
  write(const void* buf, SIZE_T size) override;

  /** 
   *  Skip a defined number of bytes. This can also be a negative value,
   *  in which case the file pointer rewinds a defined number of bytes.
   **/
  void
  skip(SIZE_T count) override;

  /**
   *  Repositions the read point to a specified byte.
   */
  void
  seek(SIZE_T pos) override;

  /** 
   *   Returns the current byte offset from beginning.
   **/
  NODISCARD SIZE_T
  tell() const override;

  /** 
   *   True if the stream is at the end.
   **/
  bool
  isAtEnd() const override;

  /**
   *   Close the stream. This makes further operations invalid.
   */
  void
  close() override;

  /**
   *    Creates a copy of this stream.
   */
  SPtr<DataStream>
  clone() const override;

 private:
 friend class FileDataStream;
  uint8* m_data;
  uint8* m_currPos;
  uint8* m_end;

  bool m_freeOnClose;
};

/*
 * Description: 
 *     Data stream specialized for handling data from standard streams. 
 *
 * Sample usage:
 *
 */
class CH_UTILITY_EXPORT FileDataStream: public DataStream
{
 public:
 /*
  *   Default constructor
  */
  FileDataStream(const Path& _path, 
                 AccesModeFlag _accessMode = AccesModeFlag(ACCESS_MODE::kREAD), 
                 bool _freeOnClose = true);

  /** 
   *   Constructor from Memory stream.
   * 
   **/
  explicit FileDataStream(const Path& _path, const SPtr<DataStream>& sourceDataStream);

 /*
  *   Default destructor
  */
  ~FileDataStream();

  bool
  isFile() const override {
    return true;
  }

  /**
   * @brief @copydoc DataStream::read
   */
  SIZE_T
  read(void* buf, SIZE_T count) override;
  
  /**
   * @brief @copydoc DataStream::write
   */
  SIZE_T
  write(const void* buf, SIZE_T count) override;
  
  /**
   * @brief @copydoc DataStream::skip
   */
  void
  skip(SIZE_T count) override;
  
  /**
   * @brief @copydoc DataStream::seek
   */
  void
  seek(SIZE_T pos) override;
  
  /**
   * @brief @copydoc DataStream::tell
   */
  SIZE_T
  tell() const override;
  
  /**
   * @brief @copydoc DataStream::isEOF
   */
  bool
  isAtEnd() const override;
  
  /**
   * @brief @copydoc DataStream::clone
   */
  SPtr<DataStream>
  clone() const override;
  
  /**
   * @brief @copydoc DataStream::close
   */
  void 
  close() override;

  /** 
   *   True if this file is open.
   * To open again a file, use FileSystem::openFile(path);
   **/
  bool
  isOpen() const;
  
  /**
   * @brief Returns the path of the file opened by the stream.
   */
  FORCEINLINE const Path& 
  getPath() const {
    return m_path;
  }

 private:

 /** 
  *   Initializes this class using.
  **/
  void
  init();


  protected:
   Path m_path;
   SPtr<std::istream> m_pInStream;
   SPtr<std::ifstream> m_pFStreamRO;
   SPtr<std::fstream> m_pFStream;
   bool m_freeOnClose;
};

}

