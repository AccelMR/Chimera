#include "chBMPImage.h"
#include "chBox2D.h"
#include "chDebug.h"
#include "chFileSystem.h"
#include "chPath.h"

using namespace chEngineSDK;

#define RUN_BMP_TESTS
#ifdef RUN_BMP_TESTS

int main(int argc, char* argv[])
{
  LOG_INFO("BMPImage Test");
  String resourcePath = "resources/";
  String savePath = "resources/save/tests/";
  String fileName = "test";

  BMPImage source;
  source.decode(resourcePath + fileName + ".bmp");

  BMPImage dest;
  uint32 destWidth = source.getWidth() * 2;
  uint32 destHeight = source.getHeight() * 2;
  dest.create(destWidth, destHeight, BPP::BPP_24);

  Box2D srcRect(Vector2::ZERO, {static_cast<float>(source.getWidth()),
                                static_cast<float>(source.getHeight()) });
  Box2D dstRect(Vector2::ZERO, {static_cast<float>(destWidth), 
                                static_cast<float>(destHeight) });

  dest.bitBlt(source, srcRect, dstRect, BMP_TEXTURE_MODE::NONE);
  dest.encode(resourcePath + fileName + BMP_TextureModeString::toString(BMP_TEXTURE_MODE::NONE));
  dest.clear(Color::Transparent);

  dest.bitBlt(source, srcRect, dstRect, BMP_TEXTURE_MODE::REPEAT);
  dest.encode(resourcePath + fileName + BMP_TextureModeString::toString(BMP_TEXTURE_MODE::REPEAT));
  dest.clear(Color::Transparent);

  dest.bitBlt(source, srcRect, dstRect, BMP_TEXTURE_MODE::CLAMP);
  dest.encode(resourcePath + fileName + BMP_TextureModeString::toString(BMP_TEXTURE_MODE::CLAMP));
  dest.clear(Color::Transparent);

  dest.bitBlt(source, srcRect, dstRect, BMP_TEXTURE_MODE::MIRROR);
  dest.encode(resourcePath + fileName + BMP_TextureModeString::toString(BMP_TEXTURE_MODE::MIRROR));
  dest.clear(Color::Transparent);

  dest.bitBlt(source, srcRect, dstRect, BMP_TEXTURE_MODE::STRETCH);
  dest.encode(resourcePath + fileName + BMP_TextureModeString::toString(BMP_TEXTURE_MODE::STRETCH));
  dest.clear(Color::Transparent);

  FileSystem::createDirectory(savePath);
  g_Debug().saveLog(savePath + "BMPImageTest.txt");

  return 0;
}

#endif //RUN_BMP_TESTS