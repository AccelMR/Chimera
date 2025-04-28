/************************************************************************/
/**
 * @file chModel.cpp
 * @author AccelMR
 * @date 2025/04/18
 * @brief
 *  Implementation of the Model class with hierarchical tree structure.
 */
/************************************************************************/
#include "chModel.h"
#include "chSphereBoxBounds.h"
#include "chVector3.h"
#include "chVector4.h"

namespace chEngineSDK {

/*
 * ModelNode constructor
 */
ModelNode::ModelNode(const String& nodeName, Model* ownerModel, ModelNode* parent)
 : m_name(nodeName),
   m_parent(parent),
   m_localTransform(Matrix4::IDENTITY),
   m_globalTransform(Matrix4::IDENTITY),
   m_model(ownerModel),
   m_dirty(true) {}

/*
 * Add a child node
 */
void
ModelNode::addChild(ModelNode* child) {
  if (!child) {
    return;
  }
  auto it = std::find(m_children.begin(), m_children.end(), child);
  if (it == m_children.end()) {
    m_children.push_back(child);
    child->m_parent = this;
    child->markDirty();
  }
}

/*
 * Remove a child node
 */
void
ModelNode::removeChild(ModelNode* child) {
  auto it = std::find(m_children.begin(), m_children.end(), child);
  if (it != m_children.end()) {
    (*it)->m_parent = nullptr;
    m_children.erase(it);
  }
}

/*
 * Add a mesh to this node
 */
void
ModelNode::addMesh(SPtr<Mesh> mesh) {
  if (mesh) {
    m_meshes.push_back(mesh);

    if (m_model) {
      m_model->registerMeshForNode(mesh, this);
    }
  }
}

/*
 * Remove a mesh from this node
 */
void
ModelNode::removeMesh(SPtr<Mesh> mesh) {
  auto it = std::find(m_meshes.begin(), m_meshes.end(), mesh);
  if (it != m_meshes.end()) {
    m_meshes.erase(it);

    if (m_model) {
      m_model->unregisterMeshForNode(mesh, this);
    }
  }
}

/*
 * Set the local transform matrix
 */
void
ModelNode::setLocalTransform(const Matrix4& transform) {
  m_localTransform = transform;
  markDirty();
}

/*
 * Update the global transform based on parent's global transform
 */
void
ModelNode::updateGlobalTransform() {
  if (!m_dirty) {
    return;
  }

  if (m_parent) {
    m_globalTransform = m_parent->getGlobalTransform() * m_localTransform;
  } 
  else if (m_model){
    m_globalTransform = m_model->getTransform() * m_localTransform;
  }
  else {
    m_globalTransform = m_localTransform;
  }

  m_dirty = false;
}

/*
 * Find a child node by name
 */
ModelNode*
ModelNode::findChildByName(const String& name) {
  for (ModelNode* child : m_children) {
    if (child->getName() == name) {
      return child;
    }
  }
  return nullptr;
}

/*
 * Mark this node and all its children as dirty (needing transform update)
 */
void
ModelNode::markDirty() {
  m_dirty = true;
  for (ModelNode* child : m_children) {
    child->markDirty();
  }
}

/*
 * Model constructor
 */
Model::Model() : m_globalTransform(Matrix4::IDENTITY) {}

/*
 * Model destructor
 */
Model::~Model() {
  // Limpiar todos los nodos
  for (ModelNode* node : m_allNodes) {
    delete node;
  }
  m_allNodes.clear();
  m_rootNodes.clear();
  m_nodeMap.clear();
  m_meshToNodesMap.clear();
}

/*
 * Create a node without a transform
 */
ModelNode*
Model::createNode(const String& name, ModelNode* parent) {
  return createNode(name, Matrix4::IDENTITY, parent);
}

/*
 * Create a node with a transform
 */
ModelNode*
Model::createNode(const String& name, const Matrix4& localTransform, ModelNode* parent) {
  ModelNode* node = new ModelNode(name, this, parent);
  node->setLocalTransform(localTransform);

  if (parent) {
    parent->addChild(node);
  } 
  else {
    m_rootNodes.push_back(node);
  }

  addNodeToStructures(node);
  return node;
}

/*
 * Find a node by name
 */
ModelNode*
Model::findNode(const String& name) const {
  auto it = m_nodeMap.find(name);
  if (it != m_nodeMap.end()) {
    return it->second;
  }
  return nullptr;
}

/*
 * Update transforms for all dirty nodes
 */
void
Model::updateTransforms() {
  // Primero actualizar los nodos raíz
  for (ModelNode* root : m_rootNodes) {
    updateNodeHierarchy(root);
  }
}

/*
 * Update transform for a specific node
 */
void
Model::updateNodeTransform(ModelNode* node, const Matrix4& localTransform) {
  if (node) {
    node->setLocalTransform(localTransform);
    updateNodeHierarchy(node);
  }
}

/*
 * Update a node and its hierarchy
 */
void
Model::updateNodeHierarchy(ModelNode* node) {
  if (node->isDirty()) {
    node->updateGlobalTransform();

    for (ModelNode* child : node->getChildren()) {
      updateNodeHierarchy(child);
    }
  }
}

/*
 * Calculate bounds for the model
 */
SphereBoxBounds
Model::calculateBounds() const {
  Vector<Vector3> allPositions;

  // Para cada nodo
  for (ModelNode* node : m_allNodes) {
    // Para cada mesh en el nodo
    for (const auto& mesh : node->getMeshes()) {
      // Extraer posiciones
      Vector<Vector3> meshPositions = mesh->extractPositions();

      // Aplicar transformación global del nodo
      for (auto& pos : meshPositions) {
        const Vector4 transformPos = node->getGlobalTransform().transformPosition(pos);
        pos = Vector3(transformPos.x, transformPos.y, transformPos.z);
      }

      // Agregar a todas las posiciones
      allPositions.insert(allPositions.end(), meshPositions.begin(), meshPositions.end());
    }
  }

  return SphereBoxBounds(allPositions);
}

/*
 * Set the global transform for the entire model
 */
void
Model::setTransform(const Matrix4& transform) {
  m_globalTransform = transform;

  // Marcar todos los nodos raíz como dirty para que se actualicen
  for (ModelNode* root : m_rootNodes) {
    root->markDirty();
  }
}

void 
Model::registerMeshForNode(SPtr<Mesh> mesh, ModelNode* node) {
  m_meshToNodesMap[mesh].push_back(node);
}

void 
Model::unregisterMeshForNode(SPtr<Mesh> mesh, ModelNode* node) {
  auto& nodes = m_meshToNodesMap[mesh];
  nodes.erase(std::remove(nodes.begin(), nodes.end(), node), nodes.end());
  
  if (nodes.empty()) {
      m_meshToNodesMap.erase(mesh);
  }
}

/*
 * Add a node to internal structures
 */
void
Model::addNodeToStructures(ModelNode* node) {
  m_allNodes.push_back(node);

  if (!node->getName().empty()) {
    m_nodeMap[node->getName()] = node;
  }
}

/*
 * Remove a node from internal structures
 */
void
Model::removeNodeFromStructures(ModelNode* node) {
  // Eliminar de m_allNodes
  auto it = std::find(m_allNodes.begin(), m_allNodes.end(), node);
  if (it != m_allNodes.end()) {
    m_allNodes.erase(it);
  }

  // Eliminar del mapa de nombres
  if (!node->getName().empty()) {
    m_nodeMap.erase(node->getName());
  }

  // Eliminar de los nodos raíz si es necesario
  auto rootIt = std::find(m_rootNodes.begin(), m_rootNodes.end(), node);
  if (rootIt != m_rootNodes.end()) {
    m_rootNodes.erase(rootIt);
  }

  // Actualizar el mapeo de mesh a nodos
  for (const auto& mesh : node->getMeshes()) {
    auto& nodes = m_meshToNodesMap[mesh];
    nodes.erase(std::remove(nodes.begin(), nodes.end(), node), nodes.end());

    if (nodes.empty()) {
      m_meshToNodesMap.erase(mesh);
    }
  }
}

} // namespace chEngineSDK