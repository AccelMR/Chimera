/************************************************************************/
/**
 * @file chBMPImage.cpp
 * @author AccelMR
 * @date 2025/01/17
 * 
 *    BMP Image class that handles BMP image files, load/save and edit them.
 */
 /************************************************************************/

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chBMPImage.h"

#include "chBox2D.h"
#include "chColor.h"
#include "chDebug.h"
#include "chFileSystem.h"
#include "chStringUtils.h"

namespace chEngineSDK {
namespace ImageHelpers
{
/*
 */
void 
writePixel(uint8 *buffer, const Color &color, const BPP m_bpp)
{
  switch (m_bpp)
  {
  case BPP::BPP_16:
  {
    uint16 pixel = color.to16Bit(true);
    buffer[0] = pixel & 0xFF;
    buffer[1] = (pixel >> 8) & 0xFF;
    break;
  }
  case BPP::BPP_24:
    buffer[0] = color.b;
    buffer[1] = color.g;
    buffer[2] = color.r;
    break;
  case BPP::BPP_32:
    buffer[0] = color.b;
    buffer[1] = color.g;
    buffer[2] = color.r;
    buffer[3] = color.a;
    break;
  default:
    std::cerr << "BMPImage::writePixel() " << "Error: Unsupported BPP format in writePixel." << std::endl;
    break;
  }
}

/*
 */
Color 
readPixel(const uint8 *buffer, const BPP m_bpp)
{
  switch (m_bpp)
  {
  case BPP::BPP_16:
    return Color::from16Bit(*reinterpret_cast<const uint16 *>(buffer), true);
    break;
  case BPP::BPP_24:
  {
    Color color;
    color.b = buffer[0];
    color.g = buffer[1];
    color.r = buffer[2];
    return color;
    break;
  }
  case BPP::BPP_32:
  {
    Color color;
    color.b = buffer[0];
    color.g = buffer[1];
    color.r = buffer[2];
    color.a = buffer[3];
    return color;
    break;
  }
  default:
    LOG_ERROR("Error: Unsupported BPP format in readPixel.");
    return Color();
    break;
  }
}
}

/*
 */
BMPImage::~BMPImage() {}

/*
 */
void
BMPImage::create(uint32 width, uint32 height, BPP bpp)
{
  if (width <= 0 || height <= 0)
  {
    LOG_ERROR(StringUtils::format("Error: Invalid image size ({0}, {1})", width, height));
    return;
  }

  m_data.reset();

  m_width = width;
  m_height = height;
  m_bpp = bpp;

  m_bytesPerPixel = static_cast<int32>(m_bpp) / 8;
  m_pitch = m_width * m_bytesPerPixel;

  m_data = ch_shared_ptr_new<MemoryDataStream>(m_pitch * m_height);
}

/*
 */
void 
BMPImage::clear(const Color &color)
{
  Vector<uint8> buffer(m_bytesPerPixel);
  ImageHelpers::writePixel(buffer.data(), color, m_bpp);

  m_data->seek(0);
  uint8* pixelInit = m_data->getCurrentPtr();
  CH_ASSERT(pixelInit);

  for (int32 y = 0; y < m_height; ++y)
  {
    uint8 *row = pixelInit + y * m_pitch;
    for (int32 x = 0; x < m_width; ++x)
    {
      std::memcpy(row + x * m_bytesPerPixel, buffer.data(), m_bytesPerPixel);
    }
  }
}

/*
 */
Color 
BMPImage::getPixel(uint32 x, uint32 y) const
{
  if (x >= m_width || y >= m_height)
  {
    LOG_ERROR(StringUtils::format("Error: Invalid pixel coordinates ({0}, {1})", x, y));
    return Color();
  }

  CH_ASSERT(m_data);
  m_data->seek(y * m_pitch + x * m_bytesPerPixel);
  uint8 *buffer = m_data->getCurrentPtr();
  return ImageHelpers::readPixel(buffer, m_bpp);
}

/*
 */
void 
BMPImage::setPixel(uint32 x, uint32 y, const Color &color)
{
  if (x >= m_width || y >= m_height)
  {
    LOG_ERROR(StringUtils::format("Error: Invalid pixel coordinates ({0}, {1})", x, y));
    return;
  }

  CH_ASSERT(m_data);
  m_data->seek(y * m_pitch + x * m_bytesPerPixel);
  uint8 *buffer = m_data->getCurrentPtr();
  ImageHelpers::writePixel(buffer, color, m_bpp);
}

/*
 */
Color 
BMPImage::getColor(float u, float v) const
{
  int32 x = static_cast<int32>(u * (m_width - 1));
  int32 y = static_cast<int32>(v * (m_height - 1));

  return getPixel(x, y);
}

/*
 */
void 
BMPImage::setColor(float u, float v, const Color &color)
{
  int32 x = static_cast<int32>(u * (m_width - 1));
  int32 y = static_cast<int32>(v * (m_height - 1));

  setPixel(x, y, color);
}

/*
 */
bool 
BMPImage::decode(const Path &bmpPath)
{
  Vector<uint8> buffer = FileSystem::fastReadFile(bmpPath);
  if (buffer.empty())
  {
    LOG_ERROR(StringUtils::format("Error: Unable to read file {0}", bmpPath.toString()));
    return false;
  }

  BMPHeader header;
  std::memcpy(&header, buffer.data(), sizeof(BMPHeader));

  if (header.signature[0] != 'B' || header.signature[1] != 'M')
  {
    LOG_ERROR("Error: Invalid BMP file format.");
    return false;
  }

  BMPInfoHeaderCore infoHeader;
  std::memcpy(&infoHeader, buffer.data() + sizeof(BMPHeader), sizeof(BMPInfoHeaderCore));

  create(infoHeader.width, infoHeader.height, static_cast<BPP>(infoHeader.bpp));

  const int32 lineMemoryWidth = m_pitch + (m_pitch % 4 ? 4 - m_pitch % 4 : 0);

  if (buffer.size() < header.dataOffset + lineMemoryWidth * m_height)
  {
    LOG_ERROR("Error: File too small to contain BMP image data.");
    return false;
  }

  uint8* pixelInit = m_data->getStartPtr();

  for (int32 y = m_height - 1; y >= 0; --y)
  {
    uint8* imageData = buffer.data() + header.dataOffset + y * lineMemoryWidth;
    std::memcpy(pixelInit + m_pitch * (m_height - 1 - y), imageData, m_pitch);
  }

  return true;
}

/*
 */
void 
BMPImage::encode(const Path& filename) const
{
  SPtr<DataStream> file = FileSystem::createAndOpenFile(filename + ".bmp");
  if (!file)
  {
    LOG_ERROR(StringUtils::format("Error: Unable to create file {0}", filename.toString()));
    return;
  }

  int32 padding = m_pitch % 4;
  const int32 lineMemoryWidth = m_pitch + (m_pitch % 4 ? 4 - m_pitch % 4 : 0);
  const SIZE_T wholeHeaderSize = sizeof(BMPHeader) + sizeof(BMPInfoHeader);

  BMPHeader header;
  header.signature[0] = 'B';
  header.signature[1] = 'M';
  header.fileSize = wholeHeaderSize + lineMemoryWidth * m_height;
  header.reserved = 0;
  header.dataOffset = wholeHeaderSize;

  file->write(reinterpret_cast<const char *>(&header), sizeof(BMPHeader));

  BMPInfoHeader infoHeader;
  infoHeader.core.headerSize = sizeof(BMPInfoHeader);
  infoHeader.core.width = m_width;
  infoHeader.core.height = m_height;
  infoHeader.core.planes = 1;
  infoHeader.core.bpp = static_cast<int>(m_bpp);

  infoHeader.compression = 0;
  infoHeader.imageSize = 0;
  infoHeader.xPixelsPerMeter = 3780; // 96 dpi
  infoHeader.yPixelsPerMeter = 3780;
  infoHeader.colorsUsed = 0;
  infoHeader.importantColors = 0;

  file->write(reinterpret_cast<const char *>(&infoHeader), sizeof(BMPInfoHeader));

  uint8* imageData = m_data->getStartPtr();

  const char paddBuffer[3] = {0, 0, 0};
  for (int y = m_height - 1; y >= 0; --y)
  {
    file->write(reinterpret_cast<const char *>(imageData + y * m_pitch), m_pitch);

    if (padding != 0)
    {
      file->write(paddBuffer, padding);
    }
  }

  file->close();
}

/*
 */
void 
BMPImage::bitBlt(const BMPImage& src,
                 const Box2D& srcRect,
                 const Box2D& dstRect,
                 const BMP_TEXTURE_MODE mode,
                 const Color& colorKey)
{
  Box2D srcRectClamped = srcRect;
  srcRectClamped.clamp(Box2D(Vector2::ZERO, { static_cast<float>(src.getWidth()),
                                              static_cast<float>(src.getHeight()) } ));

  Box2D dstRectClamped = dstRect;
  dstRectClamped.clamp(Box2D(Vector2::ZERO, { static_cast<float>(m_width), 
                                              static_cast<float>(m_height) }));

  Vector2 widthHeight = dstRectClamped.getSize();

  for (int32 y = 0; y < widthHeight.y; ++y)
  {
    for (int32 x = 0; x < widthHeight.x; ++x)
    {
      int32 srcX = 0;
      int32 srcY = 0;
      if (!calculateSourceCoordinates(x, y, 
                                      srcRectClamped, dstRectClamped, 
                                      mode, srcX, srcY))
      {
        continue;
      }

      Color color = src.getPixel(srcX, srcY);
      if (color == colorKey)
      {
        continue;
      }

      //LOG_DBG(StringUtils::format("Copying pixel ({0}, {1}) <- ({2}, {3})", x, y, srcX, srcY));
      setPixel(dstRectClamped.minPoint.x + x, dstRectClamped.minPoint.y + y, color);
    }
  }
}

/*
 */
bool 
BMPImage::calculateSourceCoordinates(int32 x, int32 y,
                                     const Box2D &srcRect, 
                                     const Box2D &dstRect,
                                     BMP_TEXTURE_MODE mode,
                                     int32 &srcX, int32 &srcY)
{
  Vector2 dstWidthHeight = dstRect.getSize();
  Vector2 srcWidthHeight = srcRect.getSize();
  switch (mode)
  {
  case BMP_TEXTURE_MODE::NONE:
    srcX = srcRect.minPoint.x + x;
    srcY = srcRect.minPoint.y + y;

    return srcX >= 0 && srcX < static_cast<int32>(srcWidthHeight.x) &&
          srcY >= 0 && srcY < static_cast<int32>(srcWidthHeight.y);

  case BMP_TEXTURE_MODE::REPEAT:
    return calculateRepeatCoordinates(x, y, srcRect, srcX, srcY);

  case BMP_TEXTURE_MODE::CLAMP:
    return calculateClampCoordinates(x, y, srcRect, srcX, srcY);

  case BMP_TEXTURE_MODE::MIRROR:
    return calculateMirrorCoordinates(x, y, srcRect, srcX, srcY);

  case BMP_TEXTURE_MODE::STRETCH:
    return calculateStretchCoordinates(x, y, srcRect, dstRect, srcX, srcY);

  default:
    LOG_ERROR("Error: Unsupported BMP_TEXTURE_MODE");
    return false;
  }
}

/*
*/
bool 
BMPImage::calculateRepeatCoordinates(int32 x, int32 y, 
                                    const Box2D &srcRect,
                                    int32& srcX, int32& srcY)
{
  Vector2 widthHeight = srcRect.getSize();
  srcX = srcRect.minPoint.x + x % static_cast<int32>(widthHeight.x);
  if (srcX < 0)
  {
    srcX += widthHeight.x;
  }

  srcY = srcRect.minPoint.y + y % static_cast<int32>(widthHeight.y);
  if (srcY < 0)
  {
    srcY += widthHeight.y;
  }

  return true;
}

/*
*/
bool 
BMPImage::calculateClampCoordinates(int32 x, int32 y, 
                                    const Box2D &srcRect,
                                    int32 &srcX, int32 &srcY)
{
  Vector2 widthHeight = srcRect.getSize();
  srcX = Math::max(0, std::min(static_cast<int32>(srcRect.minPoint.x + x), 
                               static_cast<int32>(widthHeight.x - 1)));
  srcY = Math::max(0, std::min(static_cast<int32>(srcRect.minPoint.y + y), 
                               static_cast<int32>(widthHeight.y - 1)));

  return true;
}

/*
*/
bool 
BMPImage::calculateMirrorCoordinates(int32 x, int32 y, 
                                     const Box2D &srcRect,
                                     int32 &srcX, int32 &srcY)
{
  Vector2 widthHeight = srcRect.getSize();
  srcX = static_cast<int32>(srcRect.minPoint.x) + x;
  srcY = static_cast<int32>(srcRect.minPoint.y) + y;

  if (srcX < 0)
  {
      srcX = -srcX;
  }
  if (srcX >= static_cast<int32>(widthHeight.x))
  {
      srcX %= (2 * static_cast<int32>(widthHeight.x));
      if (srcX >= static_cast<int32>(widthHeight.x))
      {
          srcX = 2 * static_cast<int32>(widthHeight.x) - srcX - 1;
      }
  }

  if (srcY < 0)
  {
      srcY = -srcY;
  }
  if (srcY >= static_cast<int32>(widthHeight.y))
  {
      srcY %= (2 * static_cast<int32>(widthHeight.y));
      if (srcY >= static_cast<int32>(widthHeight.y))
      {
          srcY = 2 * static_cast<int32>(widthHeight.y) - srcY - 1;
      }
  }

  if (srcX < 0 || srcX >= static_cast<int32>(widthHeight.x) ||
      srcY < 0 || srcY >= static_cast<int32>(widthHeight.y))
  {
    LOG_ERROR("Error: Coordinates are out of range.");
    return false;
  }
  return true;
}

/*
*/
bool 
BMPImage::calculateStretchCoordinates(int32 x, int32 y, 
                                         const Box2D &srcRect, 
                                         const Box2D &dstRect,
                                         int32 &srcX, int32 &srcY)
{
  Vector2 widthHeight = srcRect.getSize();
  Vector2 widthHeightDst = dstRect.getSize();
  const float scaleX = static_cast<float>(widthHeight.x) / widthHeightDst.x;
  const float scaleY = static_cast<float>(widthHeight.y) / widthHeightDst.y;

  srcX = static_cast<int32>(x * scaleX) + srcRect.minPoint.x;
  srcY = static_cast<int32>(y * scaleY) + srcRect.minPoint.y;

  return (srcX >= 0 && srcX < widthHeight.x && srcY >= 0 && srcY < widthHeight.y);
}

/*
 */
void
BMPImage::resize(uint32 width, uint32 height)
{
  if ((width == m_width && height == m_height) || width <= 0 || height <= 0)
  {
    return;
  }

  const float scaleX = 1 / static_cast<float>(width - 1);
  const float scaleY = 1 / static_cast<float>(height - 1);

  BMPImage temp;
  temp.create(width, height, m_bpp);

  // #pragma omp parallel for
  for (int32 y = 0; y < height; ++y)
  {
    float v = y * scaleY;
    for (int32 x = 0; x < width; ++x)
    {
      float u = x * scaleX;
      temp.setPixel(x, y, getColor(u, v));
    }
  }

  std::swap(m_data, temp.m_data);
  std::swap(m_width, temp.m_width);
  std::swap(m_height, temp.m_height);
  std::swap(m_bpp, temp.m_bpp);
  std::swap(m_bytesPerPixel, temp.m_bytesPerPixel);
  std::swap(m_pitch, temp.m_pitch);
}

} // namespace chEngineSDK