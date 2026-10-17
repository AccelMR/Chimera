/************************************************************************/
/**
 * @file chMeshCodec.cpp
 * @author AccelMR
 * @date 2025/04/19
 * @brief
 * Implementation of the MeshCodec class for importing model files.
 */
/************************************************************************/

#include "chMeshCodec.h"

#include <limits>

#if USING(CH_CODECS)

#include "chAlgorithm.h"
#include "chAssetCodecManager.h"
#include "chAssetManager.h"
#include "chFileSystem.h"
#include "chLogger.h"
#include "chMaterialAsset.h"
#include "chMath.h"
#include "chMatrix4.h"
#include "chMesh.h"
#include "chMeshSplit.h"
#include "chModelAsset.h"
#include "chTextureAsset.h"
#include "chTextureMips.h"

#include <assimp/GltfMaterial.h>
#include <assimp/Importer.hpp>
#include <assimp/ProgressHandler.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace chEngineSDK {
CH_LOG_DECLARE_STATIC(MeshSystem, All);

namespace MeshManagerHelpers {
// With aiProcess_MakeLeftHanded assimp gives X right, Y up, Z forward; the engine uses
// X forward, Y right, Z up. Engine axis i reads assimp axis kAssimpAxis[i]. This step is
// a rotation and keeps the winding, but MakeLeftHanded itself mirrors Z, which turns the
// front faces of the source (counter-clockwise) into back faces of the engine (clockwise,
// as in Direct3D); aiProcess_FlipWindingOrder turns them back.
constexpr uint32 kAssimpAxis[4] = {2, 0, 1, 3};

/*
 */
static Vector3
convertAssimpVector(const aiVector3D& vector)
{
  return Vector3(vector[kAssimpAxis[0]], vector[kAssimpAxis[1]], vector[kAssimpAxis[2]]);
}

/*
 */
static Matrix4
convertAssimpMatrix(const aiMatrix4x4& matrix)
{
  // Assimp uses column vectors, so the matrix is transposed while its axes are reordered.
  Matrix4 result;
  for (uint32 row = 0; row < 4; ++row) {
    for (uint32 column = 0; column < 4; ++column) {
      result[row][column] = matrix[kAssimpAxis[column]][kAssimpAxis[row]];
    }
  }
  return result;
}

/*
 * Any unit vector perpendicular to the normal, for vertices whose tangent cannot come from
 * texture coordinates. Only normal maps read it, and they need coordinates anyway.
 */
static Vector4
makeAnyTangent(const Vector3& normal)
{
  const Vector3& axis = Math::abs(normal.z) < 0.9f ? Vector3::UP : Vector3::FORWARD;
  Vector3 tangent = axis.cross(normal);
  if (!tangent.normalize()) {
    return Vector4(Vector3::RIGHT.x, Vector3::RIGHT.y, Vector3::RIGHT.z, 1.0f);
  }
  return Vector4(tangent.x, tangent.y, tangent.z, 1.0f);
}

/*
 * Whether the mesh's bitangents point up the image (towards smaller V), the side the engine
 * keeps. Tangents written in the file follow coordinates that grow up the image, so theirs
 * point up; the ones assimp computes follow the coordinates after aiProcess_FlipUVs and point
 * down. Each face tells the direction in which V grows, so the faces decide.
 */
static bool
bitangentsPointUp(const aiMesh& mesh)
{
  float agreement = 0.0f;
  for (uint32 i = 0; i < mesh.mNumFaces; ++i) {
    const aiFace& face = mesh.mFaces[i];
    if (face.mNumIndices != 3) {
      continue;
    }
    const uint32 i0 = face.mIndices[0];
    const uint32 i1 = face.mIndices[1];
    const uint32 i2 = face.mIndices[2];
    const aiVector3D edge1 = mesh.mVertices[i1] - mesh.mVertices[i0];
    const aiVector3D edge2 = mesh.mVertices[i2] - mesh.mVertices[i0];
    const aiVector3D uvEdge1 = mesh.mTextureCoords[0][i1] - mesh.mTextureCoords[0][i0];
    const aiVector3D uvEdge2 = mesh.mTextureCoords[0][i2] - mesh.mTextureCoords[0][i0];
    const float determinant = uvEdge1.x * uvEdge2.y - uvEdge2.x * uvEdge1.y;
    if (determinant == 0.0f) {
      continue;
    }
    // Direction of growing V on this face.
    const aiVector3D alongV = (edge2 * uvEdge1.x - edge1 * uvEdge2.x) / determinant;
    const aiVector3D bitangents =
        mesh.mBitangents[i0] + mesh.mBitangents[i1] + mesh.mBitangents[i2];
    const float dot = alongV * bitangents;
    if (Math::isFinite(dot)) {
      agreement += dot;
    }
  }
  return agreement < 0.0f;
}

/*
 * Packs the tangent and the side of the bitangent into one vector, with the bitangent
 * pointing up the image. The tangent is made perpendicular to the normal again, because
 * joining vertices and smoothing normals can tilt it.
 */
static Vector4
makeTangent(const Vector3& normal,
            const Vector3& sourceTangent,
            const Vector3& bitangent,
            bool bBitangentPointsUp)
{
  if (!Math::isFinite(sourceTangent.x) || !Math::isFinite(sourceTangent.y) ||
      !Math::isFinite(sourceTangent.z)) {
    return makeAnyTangent(normal);
  }
  Vector3 tangent = sourceTangent - normal * normal.dot(sourceTangent);
  if (!tangent.normalize()) {
    return makeAnyTangent(normal);
  }
  const bool bSameSide = normal.cross(tangent).dot(bitangent) >= 0.0f;
  const float sign = bSameSide == bBitangentPointsUp ? 1.0f : -1.0f;
  return Vector4(tangent.x, tangent.y, tangent.z, sign);
}

/*
 */
static SPtr<Mesh>
processMesh(const aiMesh& mesh)
{
  if (!mesh.HasPositions()) {
    CH_LOG_ERROR(MeshSystem, "Mesh does not have position data");
    return nullptr;
  }

  // The import asks for smooth normals, so they are missing only on points and lines;
  // tangents are missing without texture coordinates.
  const bool hasNormals = mesh.HasNormals();
  const bool hasTexCoords = mesh.HasTextureCoords(0);
  const bool hasTangents = mesh.HasTangentsAndBitangents();
  const bool bBitangentsPointUp = hasTangents && hasTexCoords && bitangentsPointUp(mesh);

  Vector<VertexStatic> vertices(mesh.mNumVertices);
  for (uint32 i = 0; i < mesh.mNumVertices; ++i) {
    VertexStatic& vertex = vertices[i];
    vertex.position = convertAssimpVector(mesh.mVertices[i]);
    vertex.normal = hasNormals ? convertAssimpVector(mesh.mNormals[i]) : Vector3::UP;
    vertex.texCoord = hasTexCoords
                          ? Vector2(mesh.mTextureCoords[0][i].x, mesh.mTextureCoords[0][i].y)
                          : Vector2::ZERO;
    vertex.tangent = hasTangents ? makeTangent(vertex.normal,
                                               convertAssimpVector(mesh.mTangents[i]),
                                               convertAssimpVector(mesh.mBitangents[i]),
                                               bBitangentsPointUp)
                                 : makeAnyTangent(vertex.normal);
  }

  SPtr<Mesh> newMesh = chMakeShared<Mesh>();
  newMesh->setVertexData(vertices);
  newMesh->setMaterialSlot(mesh.mMaterialIndex);

  if (mesh.HasFaces()) {
    const uint32 numIndices = mesh.mNumFaces * 3;

    // Past 65535 vertices (the largest uint16) the indices need 32 bits.
    if (mesh.mNumVertices > std::numeric_limits<uint16>::max()) {
      Vector<uint32> indices(numIndices);
      uint32 index = 0;
      for (uint32 i = 0; i < mesh.mNumFaces; i++) {
        const aiFace& face = mesh.mFaces[i];
        for (uint32 j = 0; j < face.mNumIndices; j++) {
          indices[index++] = face.mIndices[j];
        }
      }
      newMesh->setIndexData(indices);
    }
    else {
      Vector<uint16> indices(numIndices);
      uint32 index = 0;
      for (uint32 i = 0; i < mesh.mNumFaces; i++) {
        const aiFace& face = mesh.mFaces[i];
        for (uint32 j = 0; j < face.mNumIndices; j++) {
          indices[index++] = static_cast<uint16>(face.mIndices[j]);
        }
      }
      newMesh->setIndexData(indices);
    }
  }

  return newMesh;
}

/*
 */
static void
processNode(const aiNode& node,
            const Vector<Vector<SPtr<Mesh>>>& meshes,
            Model& model,
            ModelNode* parentNode)
{
  const Matrix4 nodeLocalTransform = convertAssimpMatrix(node.mTransformation);
  ModelNode* modelNode = model.createNode(node.mName.C_Str(), nodeLocalTransform, parentNode);

  CH_ASSERT(parentNode == nullptr ||
            Algorithm::contains(parentNode->getChildren(), modelNode));

  for (uint32 i = 0; i < node.mNumMeshes; i++) {
    for (const SPtr<Mesh>& mesh : meshes[node.mMeshes[i]]) {
      if (mesh) {
        modelNode->addMesh(mesh);
      }
    }
  }

  for (uint32 i = 0; i < node.mNumChildren; i++) {
    processNode(*node.mChildren[i], meshes, model, modelNode);
  }
}

/*
 * A folder under parent named after the model; "_1", "_2"... when it is taken.
 */
static Path
makeUniqueFolder(const Path& parent, const String& name)
{
  Path folder = parent.join(Path(name));
  ANSICHAR numberBuffer[StringUtils::MAX_INTEGER_CHARS];
  for (uint32 suffix = 1; FileSystem::exists(folder) && suffix < 1000; ++suffix) {
    String candidate = name;
    candidate += '_';
    candidate += StringUtils::toChars(numberBuffer, suffix);
    folder = parent.join(Path(std::move(candidate)));
  }
  return folder;
}

/*
 */
template <typename AssetType>
static SPtr<AssetType>
findImported(const Path& sourceFile, StringView sourcePart)
{
  const SPtr<IAsset> asset =
      AssetManager::instance().findAssetBySource(sourceFile, sourcePart);
  return asset ? asset->as<AssetType>() : nullptr;
}

/*
 * The first texture the material has among the types, which go from the most to the least
 * specific.
 */
static bool
findTextureReference(const aiMaterial& source,
                     Span<const aiTextureType> types,
                     aiString& outReference)
{
  for (const aiTextureType type : types) {
    if (source.GetTexture(type, 0, &outReference) == AI_SUCCESS) {
      return true;
    }
  }
  return false;
}

/*
 * The material keeps metallic and roughness in one texture, as glTF does. Assimp also lists
 * that texture as both metalness and roughness, so the two count only when they are the same
 * file.
 */
static bool
findMetallicRoughnessReference(const aiMaterial& source, aiString& outReference)
{
  if (source.GetTexture(aiTextureType_GLTF_METALLIC_ROUGHNESS, 0, &outReference) ==
      AI_SUCCESS) {
    return true;
  }

  aiString roughness;
  const bool bHasMetalness =
      source.GetTexture(aiTextureType_METALNESS, 0, &outReference) == AI_SUCCESS;
  const bool bHasRoughness =
      source.GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &roughness) == AI_SUCCESS;
  if (bHasMetalness && bHasRoughness && outReference == roughness) {
    return true;
  }
  if (bHasMetalness || bHasRoughness) {
    CH_LOG_WARNING(MeshSystem,
                   "Material {0} has separate metallic and roughness textures, which are "
                   "not supported; its factors are used instead",
                   source.GetName().C_Str());
  }
  return false;
}

/*
 * Reads the factors after the textures are set, because what a factor means depends on
 * whether its texture exists.
 */
static void
readMaterialFactors(const aiMaterial& source, Material& material)
{
  // PBR formats (glTF) define every factor as a multiplier of its texture. Older formats
  // have no metallic or roughness, and tools ignore their diffuse and emissive colors once
  // a texture is set, so there a texture gets a factor of one.
  float metallic = 0.0f;
  float roughness = 0.0f;
  const bool bPbrSource = source.Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS;
  const bool bHasRoughness = source.Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS;
  if (bPbrSource) {
    material.setMetallicFactor(metallic);
  }
  if (bHasRoughness) {
    material.setRoughnessFactor(roughness);
  }
  if (material.getTexture(MaterialTexture::MetallicRoughness)) {
    if (!bPbrSource) {
      material.setMetallicFactor(1.0f);
    }
    if (!bHasRoughness) {
      material.setRoughnessFactor(1.0f);
    }
  }

  const bool bHasBaseColorTexture = material.getTexture(MaterialTexture::BaseColor) != nullptr;
  aiColor4D color;
  if (source.Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS ||
      (!bHasBaseColorTexture && source.Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)) {
    material.setBaseColorFactor(LinearColor(color.r, color.g, color.b, color.a));
  }

  const bool bHasEmissiveTexture = material.getTexture(MaterialTexture::Emissive) != nullptr;
  if (bHasEmissiveTexture && !bPbrSource) {
    material.setEmissiveFactor(LinearColor(1.0f, 1.0f, 1.0f, 1.0f));
  }
  else if (source.Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) {
    float intensity = 1.0f;
    source.Get(AI_MATKEY_EMISSIVE_INTENSITY, intensity);
    material.setEmissiveFactor(
        LinearColor(color.r * intensity, color.g * intensity, color.b * intensity, 1.0f));
  }

  float normalScale = 1.0f;
  if (source.Get(AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_NORMALS, 0), normalScale) ==
      AI_SUCCESS) {
    material.setNormalScale(normalScale);
  }

  // Assimp's glTF importer stores the occlusion strength under the texture file key plus
  // ".strength", not under AI_MATKEY_GLTF_TEXTURE_STRENGTH.
  float occlusionStrength = 1.0f;
  if (source.Get(_AI_MATKEY_TEXTURE_BASE ".strength", aiTextureType_LIGHTMAP, 0,
                 occlusionStrength) == AI_SUCCESS) {
    material.setOcclusionStrength(occlusionStrength);
  }
}

/*
 * Reads how the base color alpha is used, after the textures are set. glTF says it; other
 * formats do not, so there a base color texture with pixels under the cutoff makes a mask.
 */
static void
readAlphaMode(const aiMaterial& source, Material& material, bool bAutoMask)
{
  int32 twoSided = 0;
  if (source.Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS) {
    material.setTwoSided(twoSided != 0);
  }

  float cutoff = material.getAlphaCutoff();
  if (source.Get(AI_MATKEY_GLTF_ALPHACUTOFF, cutoff) == AI_SUCCESS) {
    material.setAlphaCutoff(cutoff);
  }

  aiString alphaMode;
  if (source.Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS) {
    const StringView mode(alphaMode.C_Str(), alphaMode.length);
    if (mode == "MASK") {
      material.setAlphaMode(MaterialAlphaMode::Mask);
    }
    else if (mode == "BLEND") {
      // Blended materials are mostly foliage and decals, which a mask keeps the shape of.
      CH_LOG_WARNING(MeshSystem,
                     "Material {0} blends its alpha, which is not supported; it is drawn as "
                     "a mask",
                     source.GetName().C_Str());
      material.setAlphaMode(MaterialAlphaMode::Mask);
    }
    return;
  }

  const SPtr<TextureAsset>& baseColor = material.getTexture(MaterialTexture::BaseColor);
  if (bAutoMask && baseColor && baseColor->hasAlphaBelow(material.getAlphaCutoff())) {
    CH_LOG_INFO(MeshSystem, "Material {0} is masked: its base color texture has alpha",
                source.GetName().C_Str());
    material.setAlphaMode(MaterialAlphaMode::Mask);
  }
}

/**
 * Exists so assimp can report how far the read got and stop it when the import is
 * cancelled.
 */
class AssimpProgress final : public Assimp::ProgressHandler
{
 public:
  explicit AssimpProgress(ImportProgress& progress) : m_progress(progress) {}

  bool
  Update(float percentage) override
  {
    if (percentage >= 0.0f) {
      m_progress.setFraction(percentage * READ_SHARE);
    }
    return !m_progress.isCancelled();
  }

  // Parts of the progress bar: reading the file, converting meshes, then textures.
  static constexpr float READ_SHARE = 0.4f;
  static constexpr float MESH_SHARE = 0.1f;

 private:
  ImportProgress& m_progress;
};

/**
 * Exists so the slow part of a model import (assimp, mesh conversion, texture decoding) runs
 * on the import's worker thread, and the part that needs AssetManager (folder, reuse, asset
 * names, saving) and the graphics API (textures) runs on the main thread.
 */
class MeshImportTask final : public IImportTask
{
 public:
  MeshImportTask(const Path& filePath, const String& assetName, const ImportSettings& settings)
    : m_filePath(filePath),
      m_assetName(assetName),
      m_settings(settings)
  {}

  NODISCARD bool
  readSource(ImportProgress& progress) override;

  SPtr<IAsset>
  createAssets(const Path& assetFolder) override;

 private:
  /**
   * One texture the materials point at, read on the worker and turned into an asset on the
   * main thread when a new material needs it.
   */
  struct TextureSource
  {
    String reference;
    ImportSettings settings;
    // Where a later import finds the asset: the texture file, or the model file plus the
    // reference for an embedded texture.
    Path sourceFile;
    String sourcePart;
    String assetName;
    // A texture file or a compressed embedded image.
    UniquePtr<IImportTask> task;
    // An uncompressed embedded image.
    Vector<uint8> pixels;
    uint32 width = 0;
    uint32 height = 0;
    uint32 mipLevels = 1;
    Format format = Format::Unknown;
    bool bRead = false;
    bool bCreated = false;
    SPtr<TextureAsset> asset;
  };

  NODISCARD int32
  addTextureSource(const aiString& reference, const ImportSettings& settings);

  void
  findMaterialTextures();

  NODISCARD bool
  readTexture(TextureSource& texture, ImportProgress& progress);

  NODISCARD SPtr<TextureAsset>
  getTextureAsset(int32 index, const Path& folder);

  NODISCARD ModelMaterialSlot
  createMaterial(uint32 index, const Path& folder);

  static constexpr int32 NO_TEXTURE = -1;
  static constexpr SIZE_T TEXTURE_KINDS = static_cast<SIZE_T>(MaterialTexture::COUNT);

  Path m_filePath;
  String m_assetName;
  ImportSettings m_settings;
  // Absolute, so it can be stored as the source of everything made from it.
  Path m_sourceFile;

  // The scene belongs to the importer and stays alive until the task is destroyed, because
  // materials are read on the main thread and embedded images point into it.
  Assimp::Importer m_importer;
  const aiScene* m_scene = nullptr;
  SPtr<Model> m_model;
  Vector<TextureSource> m_textures;
  // Per material, the index into m_textures of each MaterialTexture, or NO_TEXTURE.
  Vector<Array<int32, TEXTURE_KINDS>> m_materialTextures;
};

/*
 */
int32
MeshImportTask::addTextureSource(const aiString& reference, const ImportSettings& settings)
{
  // A texture used by several materials is read once; the first use decides its settings.
  for (SIZE_T i = 0; i < m_textures.size(); ++i) {
    if (m_textures[i].reference == reference.C_Str()) {
      return static_cast<int32>(i);
    }
  }
  TextureSource& texture = m_textures.emplace_back();
  texture.reference = reference.C_Str();
  texture.settings = settings;
  return static_cast<int32>(m_textures.size() - 1);
}

/*
 */
void
MeshImportTask::findMaterialTextures()
{
  constexpr ImportSettings kColorSettings{.srgb = true};
  constexpr ImportSettings kDataSettings{.srgb = false};
  constexpr aiTextureType kBaseColorTypes[] = {aiTextureType_BASE_COLOR,
                                               aiTextureType_DIFFUSE};
  constexpr aiTextureType kNormalTypes[] = {aiTextureType_NORMALS,
                                            aiTextureType_NORMAL_CAMERA};
  constexpr aiTextureType kEmissiveTypes[] = {aiTextureType_EMISSIVE,
                                              aiTextureType_EMISSION_COLOR};
  // glTF's occlusion arrives as a lightmap.
  constexpr aiTextureType kOcclusionTypes[] = {aiTextureType_AMBIENT_OCCLUSION,
                                               aiTextureType_LIGHTMAP};

  m_materialTextures.resize(m_scene->mNumMaterials);
  for (uint32 i = 0; i < m_scene->mNumMaterials; ++i) {
    const aiMaterial& source = *m_scene->mMaterials[i];
    Array<int32, TEXTURE_KINDS>& textures = m_materialTextures[i];
    textures.fill(NO_TEXTURE);

    aiString reference;
    if (findTextureReference(source, kBaseColorTypes, reference)) {
      textures[static_cast<SIZE_T>(MaterialTexture::BaseColor)] =
          addTextureSource(reference, kColorSettings);
    }
    if (findMetallicRoughnessReference(source, reference)) {
      textures[static_cast<SIZE_T>(MaterialTexture::MetallicRoughness)] =
          addTextureSource(reference, kDataSettings);
    }
    if (findTextureReference(source, kNormalTypes, reference)) {
      textures[static_cast<SIZE_T>(MaterialTexture::Normal)] =
          addTextureSource(reference, kDataSettings);
    }
    if (findTextureReference(source, kEmissiveTypes, reference)) {
      textures[static_cast<SIZE_T>(MaterialTexture::Emissive)] =
          addTextureSource(reference, kColorSettings);
    }
    if (findTextureReference(source, kOcclusionTypes, reference)) {
      textures[static_cast<SIZE_T>(MaterialTexture::Occlusion)] =
          addTextureSource(reference, kDataSettings);
    }
  }
}

/*
 */
bool
MeshImportTask::readTexture(TextureSource& texture, ImportProgress& progress)
{
  AssetCodecManager& codecs = AssetCodecManager::instance();
  if (const aiTexture* embedded = m_scene->GetEmbeddedTexture(texture.reference.c_str())) {
    texture.sourceFile = m_sourceFile;
    texture.sourcePart = texture.reference;
    // Embedded textures may have no file name ("*0"), so the model name stands in.
    const Path embeddedName(embedded->mFilename.C_Str());
    texture.assetName = "T_";
    texture.assetName += embeddedName.empty() ? m_assetName : embeddedName.getFileName(false);

    // A height of zero means mWidth bytes of a compressed file (PNG, JPG...).
    if (embedded->mHeight == 0) {
      const Span<const uint8> data(reinterpret_cast<const uint8*>(embedded->pcData),
                                   embedded->mWidth);
      texture.task = codecs.createImportTaskFromMemory(data, embedded->achFormatHint,
                                                       texture.assetName, m_sourceFile,
                                                       texture.reference, texture.settings);
      return texture.task && texture.task->readSource(progress);
    }

    const SIZE_T texelCount = static_cast<SIZE_T>(embedded->mWidth) * embedded->mHeight;
    texture.pixels.resize(texelCount * 4);
    for (SIZE_T i = 0; i < texelCount; ++i) {
      const aiTexel& texel = embedded->pcData[i];
      texture.pixels[i * 4 + 0] = texel.r;
      texture.pixels[i * 4 + 1] = texel.g;
      texture.pixels[i * 4 + 2] = texel.b;
      texture.pixels[i * 4 + 3] = texel.a;
    }
    texture.width = embedded->mWidth;
    texture.height = embedded->mHeight;
    texture.mipLevels = texture.settings.generateMips
                            ? TextureMips::appendChainRGBA8(texture.pixels, texture.width,
                                                            texture.height,
                                                            texture.settings.srgb)
                            : 1;
    texture.format =
        texture.settings.srgb ? Format::R8G8B8A8_SRGB : Format::R8G8B8A8_UNORM;
    return true;
  }

  // Files often keep the path of the machine that made them, so the texture is also looked
  // for by name next to the source file.
  const Path referencePath(texture.reference);
  const Path sourceFolder = m_sourceFile.getDirectory();
  const Path candidates[] = {sourceFolder.join(referencePath),
                             sourceFolder.join(Path(referencePath.getFileName()))};
  const Path* found = nullptr;
  for (const Path& candidate : candidates) {
    if (FileSystem::isFile(candidate)) {
      found = &candidate;
      break;
    }
  }
  if (!found) {
    CH_LOG_WARNING(MeshSystem, "Texture {0} not found near {1}", texture.reference,
                   m_sourceFile);
    return false;
  }

  texture.sourceFile = FileSystem::absolutePath(*found);
  texture.assetName = "T_" + texture.sourceFile.getFileName(false);
  texture.task =
      codecs.createImportTask(texture.sourceFile, texture.assetName, texture.settings);
  return texture.task && texture.task->readSource(progress);
}

/*
 */
bool
MeshImportTask::readSource(ImportProgress& progress)
{
  progress.set("Reading file", 0.0f);
  if (!FileSystem::isFile(m_filePath)) {
    CH_LOG_ERROR(MeshSystem, "File not found: {0}", m_filePath.toString());
    return false;
  }
  m_sourceFile = FileSystem::absolutePath(m_filePath);

  // The importer deletes its progress handler.
  m_importer.SetProgressHandler(new AssimpProgress(progress));
  m_scene = m_importer.ReadFile(m_filePath.toString(),
                                aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_FlipUVs |
                                    aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder);
  if (progress.isCancelled()) {
    return false;
  }
  if (!m_scene || m_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_scene->mRootNode) {
    CH_LOG_ERROR(MeshSystem, "Assimp error: {0}", m_importer.GetErrorString());
    return false;
  }

  progress.set("Converting meshes", AssimpProgress::READ_SHARE);
  findMaterialTextures();
  m_model = chMakeShared<Model>();
  // Converted once per assimp mesh, so nodes that share one share the Meshes too.
  Vector<Vector<SPtr<Mesh>>> meshes(m_scene->mNumMeshes);
  for (uint32 i = 0; i < m_scene->mNumMeshes; ++i) {
    if (progress.isCancelled()) {
      return false;
    }
    progress.setFraction(AssimpProgress::READ_SHARE + AssimpProgress::MESH_SHARE *
                                                          static_cast<float>(i) /
                                                          static_cast<float>(m_scene->mNumMeshes));
    SPtr<Mesh> mesh = processMesh(*m_scene->mMeshes[i]);
    meshes[i] =
        m_settings.splitParts ? MeshSplit::splitParts(mesh) : Vector<SPtr<Mesh>>{mesh};
  }
  processNode(*m_scene->mRootNode, meshes, *m_model, nullptr);
  m_model->updateTransforms();

  // Every texture is read here, although a material reused from an earlier import of the
  // same file does not need its own: which ones are reused is only known on the main thread.
  constexpr float kTexturesStart = AssimpProgress::READ_SHARE + AssimpProgress::MESH_SHARE;
  progress.set("Reading textures", kTexturesStart);
  for (SIZE_T i = 0; i < m_textures.size(); ++i) {
    if (progress.isCancelled()) {
      return false;
    }
    progress.setFraction(kTexturesStart + (1.0f - kTexturesStart) * static_cast<float>(i) /
                                              static_cast<float>(m_textures.size()));
    m_textures[i].bRead = readTexture(m_textures[i], progress);
  }
  return !progress.isCancelled();
}

/*
 */
SPtr<TextureAsset>
MeshImportTask::getTextureAsset(int32 index, const Path& folder)
{
  if (NO_TEXTURE == index) {
    return nullptr;
  }
  TextureSource& texture = m_textures[static_cast<SIZE_T>(index)];
  if (texture.bCreated || !texture.bRead) {
    return texture.asset;
  }
  texture.bCreated = true;

  if (SPtr<TextureAsset> existing =
          findImported<TextureAsset>(texture.sourceFile, texture.sourcePart)) {
    texture.asset = std::move(existing);
    return texture.asset;
  }

  if (texture.task) {
    const SPtr<IAsset> asset = texture.task->createAssets(folder);
    texture.asset = asset ? asset->as<TextureAsset>() : nullptr;
    return texture.asset;
  }

  const AssetMetadata metadata =
      IAssetCodec::makeMetadata<TextureAsset>(texture.assetName, folder);
  SPtr<TextureAsset> textureAsset =
      chMakeShared<TextureAsset>(metadata, std::move(texture.pixels), texture.width,
                                 texture.height, texture.format, texture.mipLevels);
  if (!IAssetCodec::saveAndRegister(textureAsset, texture.sourceFile, texture.sourcePart,
                                    texture.settings)) {
    CH_LOG_ERROR(MeshSystem, "Failed to save texture asset: {0}", texture.assetName);
    return nullptr;
  }
  texture.asset = std::move(textureAsset);
  return texture.asset;
}

/*
 */
ModelMaterialSlot
MeshImportTask::createMaterial(uint32 index, const Path& folder)
{
  const aiMaterial& source = *m_scene->mMaterials[index];
  ModelMaterialSlot slot;
  slot.name = source.GetName().C_Str();
  if (slot.name.empty()) {
    ANSICHAR numberBuffer[StringUtils::MAX_INTEGER_CHARS];
    slot.name = "Material";
    slot.name += StringUtils::toChars(numberBuffer, index);
  }

  if (SPtr<MaterialAsset> existing = findImported<MaterialAsset>(m_sourceFile, slot.name)) {
    slot.materialId = existing->getUUID();
    slot.material = std::move(existing);
    return slot;
  }

  const AssetMetadata metadata =
      IAssetCodec::makeMetadata<MaterialAsset>("M_" + slot.name, folder);
  SPtr<MaterialAsset> materialAsset = chMakeShared<MaterialAsset>(metadata);
  Material& material = materialAsset->getMaterial();
  const Array<int32, TEXTURE_KINDS>& textures = m_materialTextures[index];
  for (SIZE_T kind = 0; kind < TEXTURE_KINDS; ++kind) {
    if (SPtr<TextureAsset> texture = getTextureAsset(textures[kind], folder)) {
      material.setTexture(static_cast<MaterialTexture>(kind), std::move(texture));
    }
  }

  readMaterialFactors(source, material);
  readAlphaMode(source, material, m_settings.autoAlphaMask);

  if (!IAssetCodec::saveAndRegister(materialAsset, m_sourceFile, slot.name, m_settings)) {
    CH_LOG_ERROR(MeshSystem, "Failed to save material asset: {0}", slot.name);
    return slot;
  }

  slot.materialId = materialAsset->getUUID();
  slot.material = std::move(materialAsset);
  return slot;
}

/*
 */
SPtr<IAsset>
MeshImportTask::createAssets(const Path& assetFolder)
{
  const Path modelFolder = makeUniqueFolder(assetFolder, m_assetName);
  if (!FileSystem::createDirectories(modelFolder)) {
    CH_LOG_ERROR(MeshSystem, "Failed to create the folder {0}", modelFolder);
    return nullptr;
  }

  for (uint32 i = 0; i < m_scene->mNumMaterials; ++i) {
    m_model->addMaterialSlot(createMaterial(i, modelFolder));
  }

  const AssetMetadata metadata =
      IAssetCodec::makeMetadata<ModelAsset>(m_assetName, modelFolder);
  SPtr<ModelAsset> modelAsset = chMakeShared<ModelAsset>(metadata, m_model);
  if (!IAssetCodec::saveAndRegister(modelAsset, m_sourceFile, StringView(), m_settings)) {
    CH_LOG_ERROR(MeshSystem, "Failed to save model asset: {0}", m_assetName);
    return nullptr;
  }
  return modelAsset;
}
} // namespace MeshManagerHelpers

/*
 */
MeshCodec::MeshCodec()
{
  // An Importer registers every Assimp format, so it is built once here instead of on
  // every query. Assimp lists the extensions as "*.3ds;*.obj;...".
  Assimp::Importer importer;
  String extensions;
  importer.GetExtensionList(extensions);

  for (const String& pattern : StringUtils::splitString(extensions, ';')) {
    const SIZE_T start = pattern.find_first_not_of("*.");
    if (String::npos != start) {
      const StringView extension = StringView(pattern).substr(start);
      m_extensions.push_back(IAssetCodec::normalizeExtension(extension));
    }
  }
}

/*
 */
UniquePtr<IImportTask>
MeshCodec::createImportTask(const Path& filePath,
                            const String& assetName,
                            const ImportSettings& settings)
{
  CH_LOG_INFO(MeshSystem, "Importing asset: {0}", filePath.toString());
  return chMakeUnique<MeshManagerHelpers::MeshImportTask>(filePath, assetName, settings);
}

} // namespace chEngineSDK
#endif // USING(CH_CODECS)
