/************************************************************************/
/**
 * @file chCompressionUtils.cpp
 * @author AccelMR
 * @date 2025/07/20
 * @brief Implementation of compression utilities for data compression
 */
/************************************************************************/

#include "chCompressionUtils.h"

namespace chEngineSDK {
/*
 */
CompressionResult
CompressionUtils::compress(const Vector<uint8>& data, CompressionType type) {
  if (data.empty()) {
    return {Vector<uint8>(), CompressionType::None, 0, 0};
  }

  CompressionResult result;
  result.originalSize = static_cast<uint32>(data.size());
  result.type = type;

  Vector<uint8> compressed;

  switch (type) {
    case CompressionType::RLE:
      compressed = compressRLE(data);
      break;
    case CompressionType::LZ77:
      compressed = compressLZ77(data);
      break;
    case CompressionType::None:
    default:
      compressed = data;
      break;
  }

  // Create final output with header
  CompressionHeader header = {
    .magic = COMPRESSION_MAGIC,
    .type = type,
    .originalSize = result.originalSize,
    .compressedSize = static_cast<uint32>(compressed.size())
  };

  result.data.reserve(sizeof(CompressionHeader) + compressed.size());
  writeHeader(result.data, header);
  result.data.insert(result.data.end(), compressed.begin(), compressed.end());

  result.compressedSize = static_cast<uint32>(result.data.size());

  return result;
}

/*
 */
Vector<uint8>
CompressionUtils::decompress(const Vector<uint8>& compressedData) {
  if (compressedData.empty() || compressedData.size() < sizeof(CompressionHeader)) {
    return Vector<uint8>();
  }

  CompressionHeader header;
  if (!readHeader(compressedData, header)) {
    return Vector<uint8>();
  }

  // Extract payload data (skip header)
  const size_t headerSize = sizeof(CompressionHeader);
  if (compressedData.size() < headerSize + header.compressedSize) {
    return Vector<uint8>();
  }

  Vector<uint8> payload(compressedData.begin() + headerSize,
                        compressedData.begin() + headerSize + header.compressedSize);

  switch (header.type) {
    case CompressionType::RLE:
      return decompressRLE(payload, header.originalSize);
    case CompressionType::LZ77:
      return decompressLZ77(payload, header.originalSize);
    case CompressionType::None:
      return payload;
    default:
      return Vector<uint8>();
  }
}

/*
 */
CompressionResult
CompressionUtils::compressBest(const Vector<uint8>& data) {
  if (data.empty()) {
    return {Vector<uint8>(), CompressionType::None, 0, 0};
  }

  CompressionResult bestResult = compress(data, CompressionType::None);

  // Try RLE compression
  CompressionResult rleResult = compress(data, CompressionType::RLE);
  if (rleResult.compressedSize < bestResult.compressedSize) {
    bestResult = rleResult;
  }

  // Try LZ77 compression
  CompressionResult lz77Result = compress(data, CompressionType::LZ77);
  if (lz77Result.compressedSize < bestResult.compressedSize) {
    bestResult = lz77Result;
  }

  return bestResult;
}

/*
 */
bool
CompressionUtils::isCompressed(const Vector<uint8>& data) {
  CompressionHeader header;
  return readHeader(data, header);
}

/*
 */
CompressionType
CompressionUtils::getCompressionType(const Vector<uint8>& compressedData) {
  CompressionHeader header;
  if (readHeader(compressedData, header)) {
    return header.type;
  }
  return CompressionType::None;
}

/*
 */
Vector<uint8>
CompressionUtils::compressRLE(const Vector<uint8>& data) {
  Vector<uint8> compressed;
  compressed.reserve(data.size()); // Worst case scenario

  for (size_t i = 0; i < data.size();) {
    uint8 currentByte = data[i];
    uint8 count = 1;

    // Count consecutive identical bytes (max 255)
    while (i + count < data.size() &&
           data[i + count] == currentByte &&
           count < 255) {
      count++;
    }

    // Write count and byte value
    compressed.push_back(count);
    compressed.push_back(currentByte);
    i += count;
  }

  return compressed;
}

/*
 */
Vector<uint8>
CompressionUtils::compressLZ77(const Vector<uint8>& data) {
  Vector<uint8> compressed;
  compressed.reserve(data.size()); // Initial estimation

  const size_t windowSize = 256; // Sliding window size
  const size_t maxMatchLength = 255; // Maximum match length

  for (size_t i = 0; i < data.size();) {
    size_t bestMatchLength = 0;
    size_t bestMatchDistance = 0;

    // Search for the best match in the sliding window
    size_t windowStart = (i >= windowSize) ? i - windowSize : 0;

    for (size_t j = windowStart; j < i; j++) {
      size_t matchLength = 0;

      // Find match length
      while (i + matchLength < data.size() &&
             j + matchLength < i &&
             data[j + matchLength] == data[i + matchLength] &&
             matchLength < maxMatchLength) {
        matchLength++;
      }

      // Update best match if this one is better
      if (matchLength > bestMatchLength) {
        bestMatchLength = matchLength;
        bestMatchDistance = i - j;
      }
    }

    // Encode match or literal
    if (bestMatchLength >= 3) {
      // Write match: flag + distance + length
      compressed.push_back(0xFF); // Match flag
      compressed.push_back(static_cast<uint8>(bestMatchDistance));
      compressed.push_back(static_cast<uint8>(bestMatchLength));
      i += bestMatchLength;
    } else {
      // Write literal byte (anything but 0xFF is a literal)
      uint8 literal = data[i];
      if (literal == 0xFF) {
        // Escape 0xFF literals with double 0xFF
        compressed.push_back(0xFF);
        compressed.push_back(0x00); // Escape code for literal 0xFF
        compressed.push_back(0xFF);
      } else {
        compressed.push_back(literal);
      }
      i++;
    }
  }

  return compressed;
}

/*
 */
Vector<uint8>
CompressionUtils::decompressRLE(const Vector<uint8>& data, uint32 originalSize) {
  Vector<uint8> decompressed;
  decompressed.reserve(originalSize);

  for (size_t i = 0; i < data.size(); i += 2) {
    if (i + 1 >= data.size()) {
      break; // Incomplete data
    }

    uint8 count = data[i];
    uint8 value = data[i + 1];

    // Add 'count' copies of 'value'
    for (uint8 j = 0; j < count; j++) {
      decompressed.push_back(value);

      // Safety check to prevent buffer overflow
      if (decompressed.size() >= originalSize) {
        break;
      }
    }

    if (decompressed.size() >= originalSize) {
      break;
    }
  }

  return decompressed;
}

/*
 */
Vector<uint8>
CompressionUtils::decompressLZ77(const Vector<uint8>& data, uint32 originalSize) {
  Vector<uint8> decompressed;
  decompressed.reserve(originalSize);

  for (size_t i = 0; i < data.size();) {
    uint8 currentByte = data[i];

    if (currentByte == 0xFF) {
      // Check for match or escaped literal
      if (i + 2 < data.size()) {
        uint8 nextByte = data[i + 1];

        if (nextByte == 0x00) {
          // Escaped literal 0xFF
          if (i + 2 < data.size()) {
            decompressed.push_back(data[i + 2]);
            i += 3;
          } else {
            break; // Incomplete escape sequence
          }
        } else {
          // Match: distance and length
          uint8 distance = nextByte;
          uint8 length = data[i + 2];

          // Copy from sliding window
          size_t startPos = decompressed.size() - distance;
          for (uint8 j = 0; j < length; j++) {
            if (startPos + j < decompressed.size()) {
              decompressed.push_back(decompressed[startPos + j]);
            }

            // Safety check
            if (decompressed.size() >= originalSize) {
              break;
            }
          }
          i += 3;
        }
      } else {
        break; // Incomplete match data
      }
    } else {
      // Literal byte
      decompressed.push_back(currentByte);
      i++;
    }

    // Safety check to prevent infinite loops
    if (decompressed.size() >= originalSize) {
      break;
    }
  }

  return decompressed;
}

/*
 */
void
CompressionUtils::writeHeader(Vector<uint8>& output, const CompressionHeader& header) {
  const uint8* headerBytes = reinterpret_cast<const uint8*>(&header);
  output.insert(output.end(), headerBytes, headerBytes + sizeof(CompressionHeader));
}

/*
 */
bool
CompressionUtils::readHeader(const Vector<uint8>& data, CompressionHeader& header) {
  if (data.size() < sizeof(CompressionHeader)) {
    return false;
  }

  std::memcpy(&header, data.data(), sizeof(CompressionHeader));

  // Validate magic bytes
  if (header.magic != COMPRESSION_MAGIC) {
    return false;
  }

  // Validate compression type
  if (header.type != CompressionType::None &&
      header.type != CompressionType::RLE &&
      header.type != CompressionType::LZ77) {
    return false;
  }

  // Basic size validation
  if (header.originalSize == 0 && header.compressedSize > 0) {
    return false;
  }

  return true;
}

} // namespace chEngineSDK
