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
#include "chMatrix4.h"
#include "chMesh.h"
#include "chModelAsset.h"
#include "chTextureAsset.h"
#include "chTextureMips.h"

#include <assimp/Importer.hpp>
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
 */
static SPtr<Mesh>
processMesh(const aiMesh& mesh)
{
  SPtr<Mesh> newMesh = chMakeShared<Mesh>();

  const bool hasPositions = mesh.HasPositions();
  const bool hasNormals = mesh.HasNormals();
  const bool hasTexCoords = mesh.HasTextureCoords(0);
  const bool hasColors = mesh.HasVertexColors(0);

  if (hasPositions && hasNormals && hasTexCoords) {
    Vector<VertexNormalTexCoord> vertices(mesh.mNumVertices);

    for (uint32 i = 0; i < mesh.mNumVertices; ++i) {
      vertices[i].position = convertAssimpVector(mesh.mVertices[i]);
      vertices[i].normal = convertAssimpVector(mesh.mNormals[i]);
      vertices[i].texCoord = {mesh.mTextureCoords[0][i].x, mesh.mTextureCoords[0][i].y};
    }

    newMesh->setVertexData(std::move(vertices));
  }
  else if (hasPositions) {
    if (!hasColors) {
      CH_LOG_WARNING(MeshSystem, "Mesh does not have color data, using default color");
    }
    Vector<VertexPosColor> vertices(mesh.mNumVertices);

    for (uint32 i = 0; i < mesh.mNumVertices; ++i) {
      vertices[i].position = convertAssimpVector(mesh.mVertices[i]);
      if (hasColors) {
        const aiColor4D& color = mesh.mColors[0][i];
        vertices[i].color = {color.r, color.g, color.b, color.a};
      }
      else {
        vertices[i].color = {0.7f, 0.7f, 0.7f, 1.0f};
      }
    }

    newMesh->setVertexData(std::move(vertices));
  }
  else {
    CH_LOG_ERROR(MeshSystem, "Mesh does not have position data");
    return nullptr;
  }

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
            const Vector<SPtr<Mesh>>& meshes,
            Model& model,
            ModelNode* parentNode)
{
  const Matrix4 nodeLocalTransform = convertAssimpMatrix(node.mTransformation);
  ModelNode* modelNode = model.createNode(node.mName.C_Str(), nodeLocalTransform, parentNode);

  CH_ASSERT(parentNode == nullptr ||
            Algorithm::contains(parentNode->getChildren(), modelNode));

  for (uint32 i = 0; i < node.mNumMeshes; i++) {
    if (const SPtr<Mesh>& mesh = meshes[node.mMeshes[i]]) {
      modelNode->addMesh(mesh);
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
 * The name an asset made from a part of a file stores as its imported path, so importing
 * the file again finds it.
 */
static String
makePartPath(const Path& sourceFile, StringView part)
{
  String importedPath = sourceFile.toString();
  importedPath += '#';
  importedPath += part;
  return importedPath;
}

/*
 */
template <typename AssetType>
static SPtr<AssetType>
findImported(StringView importedPath)
{
  const SPtr<IAsset> asset = AssetManager::instance().findAssetByImportedPath(importedPath);
  return asset ? asset->as<AssetType>() : nullptr;
}
} // namespace MeshManagerHelpers

using namespace MeshManagerHelpers;

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
SPtr<IAsset>
MeshCodec::importAsset(const Path& filePath,
                       const String& assetName,
                       const Path& assetFolder,
                       const ImportSettings& settings)
{
  // Each texture of a model gets the settings of the material input it feeds.
  CH_PARAMETER_UNUSED(settings);
  CH_LOG_INFO(MeshSystem, "Importing asset: {0}", filePath.toString());
  if (!FileSystem::isFile(filePath)) {
    CH_LOG_ERROR(MeshSystem, "File not found: {0}", filePath.toString());
    return nullptr;
  }

  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(filePath.toString(),
                                           aiProcessPreset_TargetRealtime_MaxQuality |
                                               aiProcess_FlipUVs | aiProcess_MakeLeftHanded |
                                               aiProcess_FlipWindingOrder);
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    CH_LOG_ERROR(MeshSystem, "Assimp error: {0}", importer.GetErrorString());
    return nullptr;
  }

  const Path modelFolder = makeUniqueFolder(assetFolder, assetName);
  if (!FileSystem::createDirectories(modelFolder)) {
    CH_LOG_ERROR(MeshSystem, "Failed to create the folder {0}", modelFolder);
    return nullptr;
  }

  const ImportContext context{.scene = scene,
                              .sourceFile = FileSystem::absolutePath(filePath),
                              .assetFolder = modelFolder,
                              .modelName = assetName};

  SPtr<Model> model = chMakeShared<Model>();
  for (uint32 i = 0; i < scene->mNumMaterials; ++i) {
    model->addMaterialSlot(importMaterial(context, *scene->mMaterials[i], i));
  }

  // Converted once per assimp mesh, so nodes that share one share the Mesh too.
  Vector<SPtr<Mesh>> meshes(scene->mNumMeshes);
  for (uint32 i = 0; i < scene->mNumMeshes; ++i) {
    meshes[i] = processMesh(*scene->mMeshes[i]);
  }
  processNode(*scene->mRootNode, meshes, *model, nullptr);
  model->updateTransforms();

  const AssetMetadata metadata =
      makeMetadata<ModelAsset>(assetName, context.sourceFile.toString(), modelFolder);
  SPtr<ModelAsset> modelAsset = chMakeShared<ModelAsset>(metadata, model);
  if (!saveAndRegister(modelAsset)) {
    CH_LOG_ERROR(MeshSystem, "Failed to save model asset: {0}", assetName);
    return nullptr;
  }

  return modelAsset;
}

/*
 */
ModelMaterialSlot
MeshCodec::importMaterial(const ImportContext& context, const aiMaterial& source, uint32 index)
{
  ModelMaterialSlot slot;
  slot.name = source.GetName().C_Str();
  if (slot.name.empty()) {
    ANSICHAR numberBuffer[StringUtils::MAX_INTEGER_CHARS];
    slot.name = "Material";
    slot.name += StringUtils::toChars(numberBuffer, index);
  }

  const String importedPath = makePartPath(context.sourceFile, slot.name);
  if (SPtr<MaterialAsset> existing = findImported<MaterialAsset>(importedPath)) {
    slot.materialId = existing->getUUID();
    slot.material = std::move(existing);
    return slot;
  }

  const AssetMetadata metadata =
      makeMetadata<MaterialAsset>("M_" + slot.name, importedPath, context.assetFolder);
  SPtr<MaterialAsset> materialAsset = chMakeShared<MaterialAsset>(metadata);
  Material& material = materialAsset->getMaterial();

  SPtr<TextureAsset> texture = importBaseColorTexture(context, source);

  // A base color (glTF and other PBR formats) multiplies its texture by definition. The
  // diffuse color of older formats is usually ignored by their tools once a texture is set,
  // and multiplying by it would darken the texture, so it is used only without one.
  aiColor4D color;
  if (source.Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS ||
      (!texture && source.Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)) {
    material.setBaseColorFactor(LinearColor(color.r, color.g, color.b, color.a));
  }
  material.setBaseColorTexture(texture);

  if (!saveAndRegister(materialAsset)) {
    CH_LOG_ERROR(MeshSystem, "Failed to save material asset: {0}", slot.name);
    return slot;
  }

  slot.materialId = materialAsset->getUUID();
  slot.material = std::move(materialAsset);
  return slot;
}

/*
 */
SPtr<TextureAsset>
MeshCodec::importBaseColorTexture(const ImportContext& context, const aiMaterial& source)
{
  constexpr ImportSettings kColorSettings{.srgb = true};

  aiString reference;
  if (source.GetTexture(aiTextureType_BASE_COLOR, 0, &reference) != AI_SUCCESS &&
      source.GetTexture(aiTextureType_DIFFUSE, 0, &reference) != AI_SUCCESS) {
    return nullptr;
  }

  if (const aiTexture* embedded = context.scene->GetEmbeddedTexture(reference.C_Str())) {
    return importEmbeddedTexture(context, *embedded, reference.C_Str(), kColorSettings);
  }

  // Files often keep the path of the machine that made them, so the texture is also looked
  // for by name next to the source file.
  const Path referencePath(reference.C_Str());
  const Path sourceFolder = context.sourceFile.getDirectory();
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
    CH_LOG_WARNING(MeshSystem, "Texture {0} of material {1} not found near {2}",
                   reference.C_Str(), source.GetName().C_Str(), context.sourceFile);
    return nullptr;
  }

  const Path texturePath = FileSystem::absolutePath(*found);
  if (SPtr<TextureAsset> existing = findImported<TextureAsset>(texturePath.toString())) {
    return existing;
  }

  const SPtr<IAsset> asset = AssetCodecManager::instance().importAsset(
      texturePath, "T_" + texturePath.getFileName(false), context.assetFolder, kColorSettings);
  return asset ? asset->as<TextureAsset>() : nullptr;
}

/*
 */
SPtr<TextureAsset>
MeshCodec::importEmbeddedTexture(const ImportContext& context,
                                 const aiTexture& texture,
                                 StringView reference,
                                 const ImportSettings& settings)
{
  const String importedPath = makePartPath(context.sourceFile, reference);
  if (SPtr<TextureAsset> existing = findImported<TextureAsset>(importedPath)) {
    return existing;
  }

  // Embedded textures may have no file name ("*0"), so the model name stands in.
  String assetName = "T_";
  const Path embeddedName(texture.mFilename.C_Str());
  assetName += embeddedName.empty() ? context.modelName : embeddedName.getFileName(false);

  // A height of zero means mWidth bytes of a compressed file (PNG, JPG...).
  if (texture.mHeight == 0) {
    const Span<const uint8> data(reinterpret_cast<const uint8*>(texture.pcData),
                                 texture.mWidth);
    const SPtr<IAsset> asset = AssetCodecManager::instance().importAssetFromMemory(
        data, texture.achFormatHint, assetName, context.assetFolder, importedPath, settings);
    return asset ? asset->as<TextureAsset>() : nullptr;
  }

  const SIZE_T texelCount = static_cast<SIZE_T>(texture.mWidth) * texture.mHeight;
  Vector<uint8> pixels(texelCount * 4);
  for (SIZE_T i = 0; i < texelCount; ++i) {
    const aiTexel& texel = texture.pcData[i];
    pixels[i * 4 + 0] = texel.r;
    pixels[i * 4 + 1] = texel.g;
    pixels[i * 4 + 2] = texel.b;
    pixels[i * 4 + 3] = texel.a;
  }

  const AssetMetadata metadata =
      makeMetadata<TextureAsset>(assetName, importedPath, context.assetFolder);
  const uint32 mipLevels = settings.generateMips
                               ? TextureMips::appendChainRGBA8(pixels, texture.mWidth,
                                                               texture.mHeight, settings.srgb)
                               : 1;
  const Format format = settings.srgb ? Format::R8G8B8A8_SRGB : Format::R8G8B8A8_UNORM;
  SPtr<TextureAsset> textureAsset = chMakeShared<TextureAsset>(
      metadata, std::move(pixels), texture.mWidth, texture.mHeight, format, mipLevels);
  if (!saveAndRegister(textureAsset)) {
    CH_LOG_ERROR(MeshSystem, "Failed to save texture asset: {0}", assetName);
    return nullptr;
  }
  return textureAsset;
}

} // namespace chEngineSDK

#endif // USING(CH_CODECS)
