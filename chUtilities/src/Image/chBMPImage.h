/************************************************************************/
/**
 * @file chBMPImage.h
 * @author AccelMR
 * @date 2025/01/17
 * 
 *    BMP Image class that handles BMP image files, load/save and edit them.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * Includes
 */
/************************************************************************/
#include "chPrerequisitesUtilities.h"

#include "chColor.h"

namespace chEngineSDK {

#pragma pack(push, 1)
/*
 * BMPHeader struct
 * Represents the header of a BMP file
*/
struct BMPHeader
{
  char signature[2];
  int32 fileSize;
  int32 reserved;
  int32 dataOffset;
};
#pragma pack(pop)

/*
 * BMPInfoHeaderCore struct
 * Represents the core information header of a BMP file
*/
struct BMPInfoHeaderCore
{
  int32 headerSize;
  int32 width;
  int32 height;
  int16 planes;
  int16 bpp;
};

/*
 * BMPInfoHeader struct
 * Represents the information header of a BMP file
*/
struct BMPInfoHeader
{
  BMPInfoHeaderCore core;
  int32 compression;
  int32 imageSize;
  int32 xPixelsPerMeter;
  int32 yPixelsPerMeter;
  int32 colorsUsed;
  int32 importantColors;
};

/*
 * BPP enum class
 * Represents the bits per pixel of a BMP file
*/
enum class BPP: uint8
{
  //TODO: support for BPP_1, BPP_4, BPP_8
  // BPP_1 = 1,
  // BPP_4 = 4,
  // BPP_8 = 8,

  BPP_16 = 16,
  BPP_24 = 24,
  BPP_32 = 32
};

/*
 * TextureMode enum class
 * Represents the texture mode
*/
enum class BMP_TEXTURE_MODE
{
  NONE,
  REPEAT,
  CLAMP,
  MIRROR,
  STRETCH
};
namespace BMP_TextureModeString
{
String toString(BMP_TEXTURE_MODE mode) 
{
  switch (mode)
  {
  case BMP_TEXTURE_MODE::NONE:
    return "NONE";
  case BMP_TEXTURE_MODE::REPEAT:
    return "REPEAT";
  case BMP_TEXTURE_MODE::CLAMP:
    return "CLAMP";
  case BMP_TEXTURE_MODE::MIRROR:
    return "MIRROR";
  case BMP_TEXTURE_MODE::STRETCH:
    return "STRETCH";
  default:
    return "UNKNOWN";
  }
}
}

/* 
 * BitmapImage class
 * Represents a bitmap image
*/

class CH_UTILITY_EXPORT BMPImage
{
 public:
  BMPImage() : m_width(0), m_height(0), m_bpp(BPP::BPP_24) {};
  ~BMPImage();

  /*
   * Getters
  */
  FORCEINLINE uint32
  getWidth() const { return m_width; }

  FORCEINLINE uint32
  getHeight() const { return m_height; }

  FORCEINLINE BPP
  getBPP() const { return m_bpp; }

  /*
   * Create a new bitmap image
   * @param width: width of the image
   * @param height: height of the image
   * @param bpp: bits per pixel (BPP_16, BPP_24, BPP_32)
  */
  void
  create(uint32 width, uint32 height, BPP bpp = BPP::BPP_24);

  /*
   * Clear the image with a color
   * @param color: color to clear the image with
  */
  void
  clear(const Color& color);

  /*
   * Get the color of a pixel
   * @param x: x-coordinate of the pixel
   * @param y: y-coordinate of the pixel
   * @return: color of the pixel
  */
  Color 
  getPixel(uint32 x, uint32 y) const;

  /*
   * Set the color of a pixel
   * @param x: x-coordinate of the pixel
   * @param y: y-coordinate of the pixel
   * @param color: color to set the pixel with
  */
  void 
  setPixel(uint32 x, uint32 y, const Color& color);

  /*
   * Get the color of a pixel at a given texture coordinate
   * @param u: u-coordinate of the texture
   * @param v: v-coordinate of the texture
   * @return: color of the pixel
  */
  Color
  getColor(float u, float v) const;

  /*
   * Set the color of a pixel at a given texture coordinate
   * @param u: u-coordinate of the texture
   * @param v: v-coordinate of the texture
   * @param color: color to set the pixel with
  */
  void
  setColor(float u, float v, const Color& color);

  /*
   * Decode a BMP file
   * @param bmpPath: path to the BMP file
   * @return: true if successful, false otherwise
  */
  bool
  decode(const Path& bmpPath);

  /*
   * Encode the image to a BMP file
   * @param filename: name of the BMP file
  */
  void
  encode(const Path& filename) const;

  /*
   * Copy a portion of the source image to the destination image
   * @param src: source image
   * @param srcRect: source rectangle
   * @param dstRect: destination rectangle
   * @param mode: texture mode (NONE, REPEAT, CLAMP, MIRROR, STRETCH)
   * @param colorKey: color key for transparency
  */
  void 
  bitBlt(const BMPImage& src,
         const Box2D& srcRect,
         const Box2D& dstRect,
         const BMP_TEXTURE_MODE mode = BMP_TEXTURE_MODE::NONE,
         const Color& colorKey = Color::Black);

  /*
   * Resize the image
   * @param width: new width of the image
   * @param height: new height of the image
   */
  void
  resize(uint32 width, uint32 height);

 private:

  /*
   * Calculate the source coordinates for a given destination coordinate
   * @param x: x-coordinate of the destination
   * @param y: y-coordinate of the destination
   * @param srcRect: source rectangle
   * @param dstRect: destination rectangle
   * @param mode: texture mode
   * @param srcX: x-coordinate of the source
   * @param srcY: y-coordinate of the source
   * @return: true if the source coordinates are valid, false otherwise
  */
  bool calculateSourceCoordinates(int32 x, int32 y,
                                  const Box2D& srcRect, 
                                  const Box2D& dstRect,
                                  BMP_TEXTURE_MODE mode,
                                  int32& srcX, int32& srcY);

  /*
   * Calculate the source coordinates for a given destination coordinate in REPEAT mode
   * @param x: x-coordinate of the destination
   * @param y: y-coordinate of the destination
   * @param srcRect: source rectangle
   * @param srcX: x-coordinate of the source
   * @param srcY: y-coordinate of the source
   * @return: true if the source coordinates are valid, false otherwise
  */
  bool calculateRepeatCoordinates(int32 x, int32 y, 
                                  const Box2D& srcRect,
                                  int32& srcX, int32& srcY);

  /*
   * Calculate the source coordinates for a given destination coordinate in CLAMP mode
   * @param x: x-coordinate of the destination
   * @param y: y-coordinate of the destination
   * @param srcRect: source rectangle
   * @param srcX: x-coordinate of the source
   * @param srcY: y-coordinate of the source
   * @return: true if the source coordinates are valid, false otherwise
  */
  bool calculateClampCoordinates(int32 x, int32 y, 
                                 const Box2D& srcRect,
                                 int32& srcX, int32& srcY);

  /*
   * Calculate the source coordinates for a given destination coordinate in MIRROR mode
   * @param x: x-coordinate of the destination
   * @param y: y-coordinate of the destination
   * @param srcRect: source rectangle
   * @param srcX: x-coordinate of the source
   * @param srcY: y-coordinate of the source
   * @return: true if the source coordinates are valid, false otherwise
  */
  bool calculateMirrorCoordinates(int32 x, int32 y, 
                                  const Box2D& srcRect,
                                  int32& srcX, int32& srcY);

  /*
   * Calculate the source coordinates for a given destination coordinate in STRETCH mode
   * @param x: x-coordinate of the destination
   * @param y: y-coordinate of the destination
   * @param srcRect: source rectangle
   * @param dstRect: destination rectangle
   * @param srcX: x-coordinate of the source
   * @param srcY: y-coordinate of the source
   * @return: true if the source coordinates are valid, false otherwise
  */
  bool calculateStretchCoordinates(int32 x, int32 y, 
                                   const Box2D& srcRect, 
                                   const Box2D& dstRect,
                                   int32& srcX, int32& srcY);


 private:
  uint32 m_width;
  uint32 m_height;
  uint16 m_pitch;
  BPP m_bpp; //bits per pixel
  uint8 m_bytesPerPixel; //bytes per pixel
  SPtr<MemoryDataStream> m_data;
};
} // namespace chEngineSDKs