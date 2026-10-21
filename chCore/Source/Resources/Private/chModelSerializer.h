/************************************************************************/
/**
 * @file chModelSerializer.h
 * @author AccelMR
 * @date 2026/10/20
 * @brief
 *  Writes and reads the data of a Model.
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesCore.h"

namespace chEngineSDK {
class DataStream;
class Mesh;
class Model;

/**
 * Exists so the bytes of a model live in one place apart from the asset, which only adds
 * the file start and loads the materials, and so tests can write and read models without
 * an AssetManager. Reading checks every count and size against the data left, so a broken
 * file fails instead of reading garbage.
 */
class CH_CORE_EXPORT ModelSerializer
{
 public:
  static void
  write(DataStream& stream, const Model& model);

  /**
   * Material slots keep only their UUID; the caller loads the assets. Null when the data
   * is broken.
   */
  NODISCARD static SPtr<Model>
  read(DataStream& stream);

 private:
  static void
  writeMesh(DataStream& stream, const Mesh& mesh);

  NODISCARD static SPtr<Mesh>
  readMesh(DataStream& stream);
};

} // namespace chEngineSDK
