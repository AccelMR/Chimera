/************************************************************************/
/**
 * @file chFileStream.h
 * @author AccelMR
 * @date 2022/08/26
 * @brief Streams that read and write bytes from memory or from a file.
 */
/************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include <iosfwd>
#include <type_traits>

#include "chFlags.h"
#include "chPath.h"

namespace chEngineSDK {

enum class AccessMode : uint16
{
  None = 0x00,
  Read = 0x01,
  Write = 0x02
};
CH_FLAGS_OPERATORS_EXT(AccessMode, uint16);
using AccessModeFlag = Flags<AccessMode, uint16>;

/**
 * Common interface for every byte stream, so serialization code works the same on memory
 * and on files. Streams never throw: failed reads and writes return fewer bytes.
 */
class CH_UTILITY_EXPORT DataStream
{
 public:
  explicit DataStream(AccessModeFlag accessMode = AccessMode::Read) noexcept
   : m_accessMode(accessMode)
  {}

  DataStream(const DataStream&) = delete;

  virtual ~DataStream() = default;

  DataStream&
  operator=(const DataStream&) = delete;

  NODISCARD FORCEINLINE AccessModeFlag
  getAccessMode() const noexcept
  {
    return m_accessMode;
  }

  NODISCARD FORCEINLINE bool
  isReadable() const noexcept
  {
    return m_accessMode.isSetAny(AccessMode::Read);
  }

  NODISCARD FORCEINLINE bool
  isWriteable() const noexcept
  {
    return m_accessMode.isSetAny(AccessMode::Write);
  }

  /**
   *   Reads the whole stream from its start as text. UTF-16 and UTF-32 with a byte order
   *   mark are converted; anything else is read as UTF-8.
   **/
  NODISCARD String
  getAsString();

  template<typename T>
  FORCEINLINE DataStream&
  operator>>(T& value)
  {
    static_assert(std::is_trivially_copyable_v<T>,
                  "Only trivially copyable types can be read as raw bytes");
    CH_ASSERT(isReadable());
    read(&value, sizeof(T));
    return *this;
  }

  template<typename T>
  FORCEINLINE DataStream&
  operator<<(const T& value)
  {
    static_assert(std::is_trivially_copyable_v<T>,
                  "Only trivially copyable types can be written as raw bytes");
    CH_ASSERT(isWriteable());
    write(&value, sizeof(T));
    return *this;
  }

  NODISCARD virtual bool
  isFile() const noexcept = 0;

  /**
   *   Returns the number of bytes read, less than size at the end of the stream.
   **/
  virtual SIZE_T
  read(void* buffer, SIZE_T size) = 0;

  /**
   *   Returns the number of bytes written, less than size when the stream is full or the
   *   write failed.
   **/
  virtual SIZE_T
  write(const void* buffer, SIZE_T size) = 0;

  /**
   *   Moves the read and write point to pos bytes from the start, never past the end.
   **/
  virtual void
  seek(SIZE_T pos) = 0;

  NODISCARD virtual SIZE_T
  tell() const = 0;

  NODISCARD virtual bool
  isAtEnd() const = 0;

  /**
   *   Releases the stream. Reads and writes after it return 0.
   **/
  virtual void
  close() = 0;

  virtual void
  flush()
  {}

  NODISCARD virtual SPtr<DataStream>
  clone() const = 0;

  NODISCARD FORCEINLINE SIZE_T
  size() const noexcept
  {
    return m_size;
  }

 protected:
  SIZE_T m_size = 0;
  AccessModeFlag m_accessMode;
};

/**
 * Stream over a block of memory, either a fixed block or one that grows as it is written.
 */
class CH_UTILITY_EXPORT MemoryDataStream : public DataStream
{
 public:
  /**
   *   An empty stream that grows as it is written, for data whose size is only known once
   *   it is written. It owns its memory.
   **/
  MemoryDataStream() noexcept;

  /**
   *   A fixed block of size bytes that the stream owns.
   **/
  explicit MemoryDataStream(SIZE_T size);

  /**
   *   A fixed block made elsewhere. With bFreeOnClose the stream frees it with free() on
   *   close, so it must come from malloc().
   **/
  MemoryDataStream(void* memory, SIZE_T size, bool bFreeOnClose = true) noexcept;

  /**
   *   Copies what is left to read in sourceStream into memory the stream owns.
   **/
  explicit MemoryDataStream(const SPtr<DataStream>& sourceStream);

  MemoryDataStream(MemoryDataStream&& other) noexcept;

  ~MemoryDataStream() override;

  MemoryDataStream&
  operator=(MemoryDataStream&& other) noexcept;

  NODISCARD bool
  isFile() const noexcept override
  {
    return false;
  }

  NODISCARD FORCEINLINE uint8*
  getStartPtr() const noexcept
  {
    return m_data;
  }

  NODISCARD FORCEINLINE uint8*
  getCurrentPtr() const noexcept
  {
    return m_currPos;
  }

  SIZE_T
  read(void* buffer, SIZE_T size) override;

  SIZE_T
  write(const void* buffer, SIZE_T size) override;

  void
  seek(SIZE_T pos) override;

  NODISCARD SIZE_T
  tell() const override;

  NODISCARD bool
  isAtEnd() const override;

  void
  close() override;

  /**
   *   The copy owns its data, so it stays valid after this stream is closed.
   **/
  NODISCARD SPtr<DataStream>
  clone() const override;

 private:
  SIZE_T
  writeGrowing(const void* buffer, SIZE_T size);

  uint8* m_data = nullptr;
  uint8* m_currPos = nullptr;
  uint8* m_end = nullptr;
  // Allocated bytes of a stream that grows; m_end marks what was written.
  SIZE_T m_capacity = 0;
  bool m_bFreeOnClose = true;
  bool m_bGrowable = false;
};

/**
 * Stream over a file on disk. It takes a real path; virtual paths go through
 * FileSystem::openFile.
 */
class CH_UTILITY_EXPORT FileDataStream : public DataStream
{
 public:
  /**
   *   Opens the file. Write alone creates it or empties it; Read and Write together open
   *   an existing file without emptying it. If the file cannot be opened, isOpen() is
   *   false and reads return 0.
   **/
  explicit FileDataStream(const Path& path, AccessModeFlag accessMode = AccessMode::Read);

  /**
   *   Creates the file and writes the whole source stream into it, from its start.
   **/
  FileDataStream(const Path& path, const SPtr<DataStream>& sourceDataStream);

  ~FileDataStream() override;

  NODISCARD bool
  isFile() const noexcept override
  {
    return true;
  }

  SIZE_T
  read(void* buffer, SIZE_T size) override;

  SIZE_T
  write(const void* buffer, SIZE_T size) override;

  void
  seek(SIZE_T pos) override;

  NODISCARD SIZE_T
  tell() const override;

  /**
   *   Compares the read point with the size the file had when it was opened.
   **/
  NODISCARD bool
  isAtEnd() const override;

  /**
   *   Opens the same file again for reading only, because opening it again for writing
   *   would empty it.
   **/
  NODISCARD SPtr<DataStream>
  clone() const override;

  void
  close() override;

  void
  flush() override;

  NODISCARD bool
  isOpen() const;

  NODISCARD FORCEINLINE const Path&
  getPath() const noexcept
  {
    return m_path;
  }

 private:
  Path m_path;
  UniquePtr<std::fstream> m_stream;
};

} // namespace chEngineSDK
