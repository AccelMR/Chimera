/************************************************************************/
/**
 * @file chModelComponent.cpp
 * @author AccelMR
 * @date 2026/10/07
 * @brief
 *  Component that draws a Model at its GameObject.
 */
/************************************************************************/
#include "chModelComponent.h"

#include "chGameObject.h"
#include "chModel.h"
#include "chScene.h"

namespace chEngineSDK {

/*
 */
ModelComponent::ModelComponent(const SPtr<Model>& model)
{
  setModel(model);
}

/*
 */
void
ModelComponent::setModel(const SPtr<Model>& model)
{
  if (isRegistered()) {
    removeRenderItems();
  }

  m_model = model;
  if (m_model) {
    // The node matrices are read when the items are placed, so they must be current.
    m_model->updateTransforms();
  }

  if (isRegistered()) {
    addRenderItems();
  }
}

/*
 */
void
ModelComponent::setTexture(const SPtr<ITexture>& texture)
{
  m_texture = texture;
  if (!isRegistered()) {
    return;
  }

  Scene& scene = *getOwner()->getScene();
  for (const MeshPart& part : m_parts) {
    scene.getRenderItem(part.renderItemId).texture = m_texture.get();
  }
}

/*
 */
AABox
ModelComponent::getWorldBounds() const
{
  AABox bounds(Vector3::ZERO, Vector3::ZERO);
  if (!isRegistered() || m_parts.empty()) {
    return bounds;
  }

  Scene& scene = *getOwner()->getScene();
  bounds = scene.getRenderItem(m_parts[0].renderItemId).worldBounds.getBox();
  for (const MeshPart& part : Span<const MeshPart>(m_parts).subspan(1)) {
    bounds += scene.getRenderItem(part.renderItemId).worldBounds.getBox();
  }
  return bounds;
}

/*
 */
void
ModelComponent::onRegister()
{
  addRenderItems();
}

/*
 */
void
ModelComponent::onUnregister()
{
  removeRenderItems();
}

/*
 */
void
ModelComponent::onTransformChanged()
{
  Scene& scene = *getOwner()->getScene();
  const Matrix4& ownerWorld = getOwner()->getTransform().getWorldMatrix();
  for (const MeshPart& part : m_parts) {
    RenderItem& item = scene.getRenderItem(part.renderItemId);
    item.worldMatrix = part.node->getGlobalTransform() * ownerWorld;
    item.worldBounds = part.mesh->getBounds().getTransformed(item.worldMatrix);
  }
}

/*
 */
void
ModelComponent::addRenderItems()
{
  if (!m_model) {
    return;
  }

  Scene& scene = *getOwner()->getScene();
  const Matrix4& ownerWorld = getOwner()->getTransform().getWorldMatrix();
  for (const ModelNode* node : m_model->getAllNodes()) {
    for (const SPtr<Mesh>& mesh : node->getMeshes()) {
      // Row vectors: the node's place in the model applies first, then the object's.
      const Matrix4 world = node->getGlobalTransform() * ownerWorld;
      const RenderItem item{.worldMatrix = world,
                            .worldBounds = mesh->getBounds().getTransformed(world),
                            .mesh = mesh.get(),
                            .texture = m_texture.get()};
      const uint32 renderItemId = scene.addRenderItem(item);
      m_parts.push_back({.node = node, .mesh = mesh.get(), .renderItemId = renderItemId});
    }
  }
}

/*
 */
void
ModelComponent::removeRenderItems()
{
  Scene& scene = *getOwner()->getScene();
  for (const MeshPart& part : m_parts) {
    scene.removeRenderItem(part.renderItemId);
  }
  m_parts.clear();
}

} // namespace chEngineSDK
