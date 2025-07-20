/************************************************************************/
/**
 * @file chCompressionUtils.h
 * @author AccelMR
 * @date 2025/07/20
 * @brief Compression utilities for data compression and decompression
 *
 * Provides static utility functions for compressing and decompressing
 * byte data using various algorithms including RLE and LZ77.
 * Designed for texture data compression but can be used for any binary data.
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
 * @brief Supported compression algorithms
 */
enum class CompressionType : uint8 {
  None = 0,    ///< No compression applied
  RLE  = 1,    ///< Run-Length Encoding
  LZ77 = 2     ///< LZ77 compression algorithm
};

/**
 * @brief Result of compression operation
 */
struct CompressionResult {
  Vector<uint8> data;              ///< Compressed data
  CompressionType type;            ///< Algorithm used for compression
  uint32 originalSize;             ///< Size of original uncompressed data
  uint32 compressedSize;           ///< Size of compressed data

  /**
   * @brief Calculate compression ratio as percentage
   * @return Compression ratio (0.0 - 1.0, where 0.5 means 50% of original size)
   */
  NODISCARD FORCEINLINE float
  getCompressionRatio() const {
    if (originalSize == 0) return 1.0f;
    return static_cast<float>(compressedSize) / static_cast<float>(originalSize);
  }
};

/**
 * @brief Static utility class for data compression and decompression
 *
 * Sample usage:
 * ```cpp
 * Vector<uint8> imageData = loadImageData();
 * auto result = CompressionUtils::compressBest(imageData);
 * Vector<uint8> decompressed = CompressionUtils::decompress(result.data);
 * ```
 */
class CH_UTILITY_EXPORT CompressionUtils
{
 public:
  /**
   * @brief Compress data using specified algorithm
   *
   * @param data Raw data to compress
   * @param type Compression algorithm to use
   * @return CompressionResult containing compressed data and metadata
   */
  static CompressionResult
  compress(const Vector<uint8>& data, CompressionType type);

  /**
   * @brief Decompress data (algorithm auto-detected from header)
   *
   * @param compressedData Data compressed by this utility
   * @return Decompressed raw data
   * @note Returns empty vector if decompression fails
   */
  static Vector<uint8>
  decompress(const Vector<uint8>& compressedData);

  /**
   * @brief Try all compression algorithms and return best result
   *
   * @param data Raw data to compress
   * @return CompressionResult with the most efficient compression found
   */
  static CompressionResult
  compressBest(const Vector<uint8>& data);

  /**
   * @brief Check if data appears to be compressed by this utility
   *
   * @param data Data to check
   * @return True if data has valid compression header
   */
  static bool
  isCompressed(const Vector<uint8>& data);

  /**
   * @brief Get compression type from compressed data
   *
   * @param compressedData Data to analyze
   * @return CompressionType used, or None if not recognized
   */
  static CompressionType
  getCompressionType(const Vector<uint8>& compressedData);

 private:
  // Magic bytes for identifying compressed data
  static constexpr uint32 COMPRESSION_MAGIC = 0x43485A50; // 'CHZP' (Chimera Zip)

  // Header structure for compressed data
  struct CompressionHeader {
    uint32 magic;              ///< Magic bytes for identification
    CompressionType type;      ///< Compression algorithm used
    uint32 originalSize;       ///< Size of uncompressed data
    uint32 compressedSize;     ///< Size of compressed payload
  };

  /**
   * @brief Compress data using Run-Length Encoding
   *
   * @param data Raw data to compress
   * @return Compressed data without header
   */
  static Vector<uint8>
  compressRLE(const Vector<uint8>& data);

  /**
   * @brief Compress data using LZ77 algorithm
   *
   * @param data Raw data to compress
   * @return Compressed data without header
   */
  static Vector<uint8>
  compressLZ77(const Vector<uint8>& data);

  /**
   * @brief Decompress RLE encoded data
   *
   * @param data Compressed data without header
   * @param originalSize Expected size of decompressed data
   * @return Decompressed raw data
   */
  static Vector<uint8>
  decompressRLE(const Vector<uint8>& data, uint32 originalSize);

  /**
   * @brief Decompress LZ77 encoded data
   *
   * @param data Compressed data without header
   * @param originalSize Expected size of decompressed data
   * @return Decompressed raw data
   */
  static Vector<uint8>
  decompressLZ77(const Vector<uint8>& data, uint32 originalSize);

  /**
   * @brief Write compression header to data stream
   *
   * @param output Vector to write header to
   * @param header Header data to write
   */
  static void
  writeHeader(Vector<uint8>& output, const CompressionHeader& header);

  /**
   * @brief Read compression header from data stream
   *
   * @param data Data to read header from
   * @param header Output header structure
   * @return True if header was successfully read and validated
   */
  static bool
  readHeader(const Vector<uint8>& data, CompressionHeader& header);
};

} // namespace chEngineSDK
