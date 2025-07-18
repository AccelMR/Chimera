/************************************************************************/
/**
 * @file chMeshImporter.cpp
 * @author AccelMR
 * @date 2025/04/19
 * @brief
 * Implementation of the MeshImpotrter class for loading and managing mesh resources.
 */
/************************************************************************/

#include "chMeshImporter.h"

#include "chAssetManager.h"
#include "chFileSystem.h"
#include "chLogger.h"
#include "chMatrix4.h"
#include "chModelAsset.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace chEngineSDK {
namespace MeshManagerHelpers {
/*
 */
static Matrix4
convertAssimpMatrix(const aiMatrix4x4& matrix) {
  return Matrix4(matrix.a1, matrix.a2, matrix.a3, matrix.a4, matrix.b1, matrix.b2, matrix.b3,
                 matrix.b4, matrix.c1, matrix.c2, matrix.c3, matrix.c4, matrix.d1, matrix.d2,
                 matrix.d3, matrix.d4);
}
} // namespace MeshManagerHelpers

CH_LOG_DECLARE_STATIC(MeshSystem, All);

/*
 */
Vector<String>
MeshImpotrter::getSupportedExtensions() const {
  Assimp::Importer importer;
  String extensions;
  importer.GetExtensionList(extensions);

  Vector<String> supportedExtensions;
  for (const String& ext : chString::splitString(extensions, ';')) {
    // Remove any leading '*' or '.' characters
    size_t start = 0;
    while (start < ext.size() && (ext[start] == '*' || ext[start] == '.')) {
      ++start;
    }
    if (start < ext.size()) {
      supportedExtensions.push_back(ext.substr(start));
    }
  }

  return supportedExtensions;
}

/*
 */
SPtr<IAsset>
MeshImpotrter::importAsset(const Path& filePath, const String& assetName) {
  CH_LOG_INFO(MeshSystem, "Importing asset: {0}", filePath.toString());
  if (!FileSystem::isFile(filePath)) {
    CH_LOG_ERROR(MeshSystem, "File not found: {0}", filePath.toString());
    return nullptr;
  }

  SPtr<Model> model = loadModel(filePath);
  if (!model) {
    CH_LOG_ERROR(MeshSystem, "Failed to load model from path: {0}", filePath.toString());
    return nullptr;
  }

  SPtr<ModelAsset> modelAsset =
      AssetManager::instance()
          .createAsset<ModelAsset>(assetName, EnginePaths::getGameAssetDirectory())
          .lock(); // TODO: Use WeakPtr to avoid shared ownership issues
  CH_ASSERT(modelAsset);

  setOriginalPath(modelAsset, filePath);
  modelAsset->setModel(model);

  if (!modelAsset->save()) {
    CH_LOG_ERROR(MeshSystem, "Failed to save model asset: {0}", assetName);
    return nullptr;
  }

  return std::static_pointer_cast<IAsset>(modelAsset);
}

/*
 */
SPtr<Mesh>
MeshImpotrter::loadMesh(const Path& meshPath, const String& meshName) {
  String name = meshName.empty() ? meshPath.getFileName() : meshName;

  auto it = m_meshes.find(name);
  if (it != m_meshes.end()) {
    return it->second;
  }

  SPtr<Model> model = loadModel(meshPath);
  if (!model || model->getAllNodes().empty()) {
    CH_LOG_ERROR(MeshSystem, "Failed to load mesh from path: {0}", meshPath.toString());
    return nullptr;
  }

  // Buscar el primer nodo que tenga meshes
  SPtr<Mesh> firstMesh = nullptr;
  for (ModelNode* node : model->getAllNodes()) {
    if (!node->getMeshes().empty()) {
      firstMesh = node->getMeshes()[0];
      break;
    }
  }

  if (!firstMesh) {
    CH_LOG_ERROR(MeshSystem, "Model has no meshes: {0}", meshPath.toString());
    return nullptr;
  }

  m_meshes[name] = firstMesh;
  CH_LOG_DEBUG(MeshSystem, "Loaded mesh from path: {0}", meshPath.toString());

  return firstMesh;
}

/*
 */
SPtr<Model>
MeshImpotrter::loadModel(const Path& filePath) {
  CH_LOG_INFO(MeshSystem, "Loading model: {0}", filePath.toString());

  String modelName = filePath.getFileName();

  auto it = m_models.find(modelName);
  if (it != m_models.end()) {
    return it->second;
  }

  if (!FileSystem::isFile(filePath)) {
    CH_LOG_ERROR(MeshSystem, "File not found: {0}", filePath.toString());
    return nullptr;
  }

  Assimp::Importer importer;

  const aiScene* scene = importer.ReadFile(filePath.toString(),
                                           aiProcessPreset_TargetRealtime_MaxQuality |
                                               aiProcess_FlipUVs | aiProcess_MakeLeftHanded //|
                                           // aiProcess_PreTransformVertices
  );

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    CH_LOG_ERROR(MeshSystem, "Assimp error: {0}", importer.GetErrorString());
    return nullptr;
  }

  SPtr<Model> model = chMakeShared<Model>();

  // Procesar el árbol de nodos comenzando por la raíz
  processNodeForModel(scene->mRootNode, scene, model, nullptr);

  // Actualizar todas las transformaciones
  model->updateTransforms();

  m_models[modelName] = model;

  return model;
}

/*
 */
void
MeshImpotrter::unloadMesh(const WeakPtr<Mesh>& mesh) {
  CH_PAMRAMETER_UNUSED(mesh);
}

/*
 */
Vector<SPtr<Mesh>>
MeshImpotrter::processNode(aiNode* node, const aiScene* scene) {
  Vector<SPtr<Mesh>> meshes;

  for (uint32 i = 0; i < node->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    SPtr<Mesh> processedMesh = processMesh(mesh, scene);

    if (processedMesh) {
      meshes.push_back(processedMesh);
    }
  }

  for (uint32 i = 0; i < node->mNumChildren; i++) {
    Vector<SPtr<Mesh>> childMeshes = processNode(node->mChildren[i], scene);
    meshes.insert(meshes.end(), childMeshes.begin(), childMeshes.end());
  }

  return meshes;
}

/*
 */
SPtr<Mesh>
MeshImpotrter::processMesh(aiMesh* mesh, const aiScene* scene) {
  SPtr<Mesh> newMesh = chMakeShared<Mesh>();

  const bool hasPositions = mesh->HasPositions();
  const bool hasNormals = mesh->HasNormals();
  const bool hasTexCoords = mesh->HasTextureCoords(0);
  const bool hasColors = mesh->HasVertexColors(0);
  // const bool hasTangents = mesh->HasTangentsAndBitangents();

  if (hasPositions && hasNormals && hasTexCoords) {
    Vector<VertexNormalTexCoord> vertices(mesh->mNumVertices);

    for (uint32 i = 0; i < mesh->mNumVertices; ++i) {
      vertices[i].position = {
          // mesh->mVertices[i].z,
          // mesh->mVertices[i].x,
          // mesh->mVertices[i].y
          mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z
      };
      vertices[i].normal = {
          //mesh->mNormals[i].z, mesh->mNormals[i].x, mesh->mNormals[i].y
          mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z
      };

      if (hasTexCoords) {
        vertices[i].texCoord = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
      }
      else {
        CH_LOG_WARNING(MeshSystem, "Mesh does not have texture coordinates");
        vertices[i].texCoord = {0.0f, 0.0f};
      }
    }

    newMesh->setVertexData(std::move(vertices));
  }
  else if (hasPositions && hasColors) {
    Vector<VertexPosColor> vertices(mesh->mNumVertices);

    for (uint32 i = 0; i < mesh->mNumVertices; ++i) {
      vertices[i].position = {mesh->mVertices[i].x, mesh->mVertices[i].y,
                              mesh->mVertices[i].z};

      if (hasColors) {
        vertices[i].color = {mesh->mColors[0][i].r, mesh->mColors[0][i].g,
                             mesh->mColors[0][i].b, mesh->mColors[0][i].a};
      }
      else {
        vertices[i].color = {1.0f, 1.0f, 1.0f, 1.0f};
      }
    }

    newMesh->setVertexData(std::move(vertices));
  }
  else if (hasPositions) {
    CH_LOG_WARNING(MeshSystem, "Mesh does not have color data, using default color");
    Vector<VertexPosColor> vertices(mesh->mNumVertices);

    for (uint32 i = 0; i < mesh->mNumVertices; ++i) {
      vertices[i].position = {mesh->mVertices[i].x, mesh->mVertices[i].y,
                              mesh->mVertices[i].z};

      // Assign default color if no color data is available
      vertices[i].color = {0.7f, 0.7f, 0.7f, 1.0f};
    }

    newMesh->setVertexData(std::move(vertices));
  }
  else {
    CH_LOG_ERROR(MeshSystem, "Mesh does not have position data");
    return nullptr;
  }

  if (mesh->mMaterialIndex != 0) {
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    // TODO: Process material properties

    aiString texturePath;
    if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
      const String diffuseTexPath = texturePath.C_Str();
      CH_LOG_INFO(MeshSystem, "Found diffuse texture: {0}", diffuseTexPath);
    }
  }

  // Procesar índices
  if (mesh->HasFaces()) {
    const uint32 numIndices = mesh->mNumFaces * 3;

    // If the number of vertices is greater than 65535, use uint32 indices
    // 65535 = 2^16 - 1 which is the max value for uint16
    if (mesh->mNumVertices > std::numeric_limits<uint16>::max()) {
      Vector<uint32> indices(numIndices);
      uint32 index = 0;
      for (uint32 i = 0; i < mesh->mNumFaces; i++) {
        const aiFace& face = mesh->mFaces[i];
        for (uint32 j = 0; j < face.mNumIndices; j++) {
          indices[index++] = face.mIndices[j];
        }
      }
      newMesh->setIndexData(indices);
    }
    else {
      Vector<uint16> indices(numIndices);
      uint32 index = 0;
      for (uint32 i = 0; i < mesh->mNumFaces; i++) {
        const aiFace& face = mesh->mFaces[i];
        for (uint32 j = 0; j < face.mNumIndices; j++) {
          indices[index++] = static_cast<uint16>(face.mIndices[j]);
        }
      }
      newMesh->setIndexData(indices);
    }
  }

  return newMesh;
}

void
MeshImpotrter::processNodeForModel(aiNode* node, const aiScene* scene, SPtr<Model> model,
                                   ModelNode* parentNode) {
  Matrix4 nodeLocalTransform = MeshManagerHelpers::convertAssimpMatrix(node->mTransformation);

  String nodeName = node->mName.C_Str();
  ModelNode* modelNode = model->createNode(nodeName, nodeLocalTransform, parentNode);

  CH_ASSERT(parentNode == nullptr ||
            std::find(parentNode->getChildren().begin(), parentNode->getChildren().end(),
                      modelNode) != parentNode->getChildren().end());

  for (uint32 i = 0; i < node->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    SPtr<Mesh> processedMesh = processMesh(mesh, scene);

    if (processedMesh) {
      modelNode->addMesh(processedMesh);
    }
  }

  for (uint32 i = 0; i < node->mNumChildren; i++) {
    processNodeForModel(node->mChildren[i], scene, model, modelNode);
  }
}
} // namespace chEngineSDK
