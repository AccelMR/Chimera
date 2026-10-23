/************************************************************************/
/**
 * @file chCoreTestMain.cpp
 * @author AccelMR
 * @date 2026/10/03
 * @brief Unit tests for chCore code that runs without a window or a GPU.
 */
/************************************************************************/
#include <chrono>
#include <cstddef>
#include <cstring>
#include <limits>

#include "chAlgorithm.h"
#include "chAnimationClip.h"
#include "chAnimationClipAsset.h"
#include "chBox.h"
#include "chCamera.h"
#include "chCameraComponent.h"
#include "chCapsule.h"
#include "chDebugDraw.h"
#include "chDynamicLibrary.h"
#include "chAngle.h"
#include "chAssetFile.h"
#include "chAssetMeta.h"
#include "chComponentRegister.h"
#include "chConfigFile.h"
#include "chGraphicsTypes.h"
#include "chInputManager.h"
#include "chIShader.h"
#include "chITexture.h"
#include "chICommandList.h"
#include "chITextureView.h"
#include "chLightComponent.h"
#include "chFileStream.h"
#include "chFileSystem.h"
#include "chFloat16.h"
#include "chGameObjectSerializer.h"
#include "chMaterial.h"
#include "chMaterialAsset.h"
#include "chMath.h"
#include "chMatrixHelpers.h"
#include "chMesh.h"
#include "chMeshGenerator.h"
#include "chMeshSplit.h"
#include "chModel.h"
#include "chModelAsset.h"
#include "chModelComponent.h"
#include "chModelExpand.h"
#include "chModelSerializer.h"
#include "chModelSerializationHeaders.h"
#include "chPrefabAsset.h"
#include "chProjectDescriptor.h"
#include "chProjectPackager.h"
#include "chQuaternion.h"
#include "chRay.h"
#include "chRenderFrame.h"
#include "chRenderGraph.h"
#include "chRenderSettings.h"
#include "chRotator.h"
#include "chScene.h"
#include "chSceneHistory.h"
#include "chSceneManager.h"
#include "chScriptAllocator.h"
#include "chScriptApi.h"
#include "chScriptAsset.h"
#include "chScriptBinder.h"
#include "chScriptComponent.h"
#include "chScriptManager.h"
#include "chScriptObjectTypes.h"
#include "chShadowRenderer.h"
#include "chShapeComponent.h"
#include "chSkeleton.h"
#include "chShapeOverlap.h"
#include "chSphere.h"
#include "chOrientedBox.h"
#include "chLinearColor.h"
#include "chSkyLightComponent.h"
#include "chSphereBoxBounds.h"
#include "chStringUtils.h"
#include "chTextureMips.h"
#include "chTransientTexturePool.h"
#include "chUUID.h"
#include "chVector2.h"
#include "chVector3.h"
#include "chVector4.h"
#include "chVertexLayout.h"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace chEngineSDK;

namespace {

constexpr float kTolerance = 0.0001f;

// Looks along +X (FORWARD) from 5 units behind the origin.
Camera
createTestCamera()
{
  Camera camera(Vector3(-5.0f, 0.0f, 0.0f), Vector3::ZERO, 1280.0f, 720.0f);
  camera.setProjectionType(CameraProjectionType::Perspective);
  camera.setFieldOfView(Radian(Degree(90.0f)));
  camera.setClipPlanes(0.1f, 100.0f);
  return camera;
}

} // namespace

TEST_CASE("chCore - Camera view")
{
  const Camera camera = createTestCamera();

  // View space is X right, Y up, Z forward.
  const Vector4 eye = camera.getViewMatrix().transformPosition(camera.getPosition());
  REQUIRE(eye.x == Approx(0.0f).margin(kTolerance));
  REQUIRE(eye.y == Approx(0.0f).margin(kTolerance));
  REQUIRE(eye.z == Approx(0.0f).margin(kTolerance));

  const Vector4 target = camera.getViewMatrix().transformPosition(Vector3::ZERO);
  REQUIRE(target.x == Approx(0.0f).margin(kTolerance));
  REQUIRE(target.y == Approx(0.0f).margin(kTolerance));
  REQUIRE(target.z == Approx(5.0f).margin(kTolerance));

  const Vector4 right = camera.getViewMatrix().transformPosition(Vector3(0.0f, 1.0f, 0.0f));
  REQUIRE(right.x == Approx(1.0f).margin(kTolerance));

  const Vector4 up = camera.getViewMatrix().transformPosition(Vector3(0.0f, 0.0f, 1.0f));
  REQUIRE(up.y == Approx(1.0f).margin(kTolerance));

  const Vector3 forward = camera.getForwardVector();
  REQUIRE(forward.x == Approx(1.0f).margin(kTolerance));
  REQUIRE(forward.y == Approx(0.0f).margin(kTolerance));
  REQUIRE(forward.z == Approx(0.0f).margin(kTolerance));
}

TEST_CASE("chCore - Camera projection")
{
  const Camera camera = createTestCamera();
  const Matrix4 viewProjection = camera.getViewProjectionMatrix();

  // Depth goes from 0 at the near plane to 1 at the far plane.
  const Vector4 nearPoint = viewProjection.transformPosition(Vector3(-4.9f, 0.0f, 0.0f));
  REQUIRE(nearPoint.z / nearPoint.w == Approx(0.0f).margin(kTolerance));

  const Vector4 farPoint = viewProjection.transformPosition(Vector3(95.0f, 0.0f, 0.0f));
  REQUIRE(farPoint.z / farPoint.w == Approx(1.0f).margin(kTolerance));

  // The center of the view lands in the middle of the screen; screen Y grows downwards.
  Vector2 center;
  REQUIRE(camera.worldToScreenPoint(Vector3::ZERO, center));
  REQUIRE(center.x == Approx(0.5f).margin(kTolerance));
  REQUIRE(center.y == Approx(0.5f).margin(kTolerance));

  Vector2 right;
  REQUIRE(camera.worldToScreenPoint(Vector3(0.0f, 1.0f, 0.0f), right));
  REQUIRE(right.x > 0.5f);

  Vector2 up;
  REQUIRE(camera.worldToScreenPoint(Vector3(0.0f, 0.0f, 1.0f), up));
  REQUIRE(up.y < 0.5f);

  // A point behind the camera has no place on screen.
  Vector2 behind;
  REQUIRE_FALSE(camera.worldToScreenPoint(Vector3(-10.0f, 0.0f, 0.0f), behind));

  // The ray through the center of the screen starts on the near plane and looks forward.
  const Ray centerRay = camera.screenToWorldRay(Vector2(0.5f, 0.5f));
  REQUIRE(centerRay.origin.nearEqual(Vector3(-4.9f, 0.0f, 0.0f), 1e-3f));
  REQUIRE(centerRay.direction.nearEqual(Vector3::FORWARD, 1e-4f));
}

TEST_CASE("chCore - Camera frustum")
{
  const Camera camera = createTestCamera();
  const Frustum& frustum = camera.getFrustum();

  REQUIRE(ShapeOverlap::frustumPoint(frustum, Vector3::ZERO));
  REQUIRE(ShapeOverlap::frustumPoint(frustum, Vector3(90.0f, 0.0f, 0.0f)));

  // Behind the camera, closer than the near plane and past the far plane.
  REQUIRE_FALSE(ShapeOverlap::frustumPoint(frustum, Vector3(-10.0f, 0.0f, 0.0f)));
  REQUIRE_FALSE(ShapeOverlap::frustumPoint(frustum, Vector3(-4.95f, 0.0f, 0.0f)));
  REQUIRE_FALSE(ShapeOverlap::frustumPoint(frustum, Vector3(100.0f, 0.0f, 0.0f)));

  // Far to a side, outside the 90 degree field of view.
  REQUIRE_FALSE(ShapeOverlap::frustumPoint(frustum, Vector3(0.0f, 50.0f, 0.0f)));
  REQUIRE_FALSE(ShapeOverlap::frustumPoint(frustum, Vector3(0.0f, 0.0f, 50.0f)));

  // A sphere outside the side plane but close enough to touch it is still visible.
  REQUIRE(ShapeOverlap::frustumSphere(frustum, Sphere(Vector3(0.0f, 12.0f, 0.0f), 5.0f)));
  REQUIRE_FALSE(
      ShapeOverlap::frustumSphere(frustum, Sphere(Vector3(0.0f, 50.0f, 0.0f), 1.0f)));

  const AABox visibleBox(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));
  REQUIRE(ShapeOverlap::frustumBox(frustum, visibleBox));

  const AABox boxBehind(Vector3(-20.0f, -1.0f, -1.0f), Vector3(-15.0f, 1.0f, 1.0f));
  REQUIRE_FALSE(ShapeOverlap::frustumBox(frustum, boxBehind));
}

TEST_CASE("chCore - Camera updates")
{
  Camera camera = createTestCamera();

  // Moving must also move the frustum; it used to keep the planes of the old position.
  REQUIRE_FALSE(ShapeOverlap::frustumPoint(camera.getFrustum(), Vector3(-10.0f, 0.0f, 0.0f)));
  camera.moveForward(-10.0f);
  REQUIRE(camera.getPosition().nearEqual(Vector3(-15.0f, 0.0f, 0.0f)));
  REQUIRE(camera.getLookAt().nearEqual(Vector3(-10.0f, 0.0f, 0.0f)));
  REQUIRE(ShapeOverlap::frustumPoint(camera.getFrustum(), Vector3(-10.0f, 0.0f, 0.0f)));

  camera.moveRight(2.0f);
  REQUIRE(camera.getPosition().nearEqual(Vector3(-15.0f, 2.0f, 0.0f)));
  camera.moveUp(3.0f);
  REQUIRE(camera.getPosition().nearEqual(Vector3(-15.0f, 2.0f, 3.0f)));
  REQUIRE(camera.getRightVector().nearEqual(Vector3::RIGHT, kTolerance));
  REQUIRE(camera.getUpVector().nearEqual(Vector3::UP, kTolerance));

  // Switching to orthographic takes effect without any other call.
  const Matrix4 perspective = camera.getProjectionMatrix();
  camera.setProjectionType(CameraProjectionType::Orthographic);
  REQUIRE_FALSE(camera.getProjectionMatrix().nearEqual(perspective, kTolerance));
  REQUIRE(camera.getProjectionMatrix()[3][3] == 1.0f);

  // Orbiting keeps the distance to the look at point and stops pitch at 89 degrees.
  Camera orbiting = createTestCamera();
  orbiting.rotate(0.0f, 90.0f);
  REQUIRE(orbiting.getPosition().nearEqual(Vector3(0.0f, -5.0f, 0.0f), 1e-4f));
  REQUIRE(orbiting.getForwardVector().nearEqual(Vector3::RIGHT, 1e-4f));
  orbiting.rotate(200.0f, 0.0f);
  REQUIRE(orbiting.getPosition().distance(Vector3::ZERO) == Approx(5.0f));
  REQUIRE(orbiting.getRotator().pitch.valueDegree() == Approx(89.0f).margin(1e-2f));

  // The rotation read back from the view matches where the camera looks.
  REQUIRE(camera.getRotation().rotateVector(Vector3::FORWARD)
              .nearEqual(camera.getForwardVector(), 1e-5f));
}

TEST_CASE("chCore - VertexLayout")
{
  // Each predefined layout must describe its vertex struct byte for byte, or the GPU reads
  // the vertices with the wrong offsets.
  REQUIRE(VertexPosColor::getLayout().getVertexSize() == sizeof(VertexPosColor));
  REQUIRE(VertexStatic::getLayout().getVertexSize() == sizeof(VertexStatic));

  const VertexLayout layout = VertexStatic::getLayout();
  const Vector<VertexAttributeDesc>& attributes = layout.getAttributes();
  REQUIRE(attributes.size() == 4);
  REQUIRE(attributes[0].offset == offsetof(VertexStatic, position));
  REQUIRE(attributes[1].offset == offsetof(VertexStatic, normal));
  REQUIRE(attributes[2].offset == offsetof(VertexStatic, texCoord));
  REQUIRE(attributes[3].offset == offsetof(VertexStatic, tangent));
  REQUIRE(attributes[3].format == VertexFormat::Float4);

  // Without an explicit offset, each attribute goes right after the previous one, and each
  // binding has its own stride.
  VertexLayout custom;
  custom.addAttribute(VertexAttributeType::Position, VertexFormat::Float3);
  custom.addAttribute(VertexAttributeType::TexCoord0, VertexFormat::Float2);
  custom.addAttribute(VertexAttributeType::Color, VertexFormat::Float4, UINT32_MAX, 1);
  REQUIRE(custom.getAttributes()[1].offset == 12);
  REQUIRE(custom.getAttributes()[2].offset == 0);
  REQUIRE(custom.getVertexSize() == 20);
  REQUIRE(custom.getStride(0) == 20);
  REQUIRE(custom.getStride(1) == 16);
  REQUIRE(custom.getBindingCount() == 2);

  // Packed formats count their real size (Dear ImGui's vertex is two Float2 and a color).
  VertexLayout packed;
  packed.addAttribute(VertexAttributeType::Position, VertexFormat::Float2);
  packed.addAttribute(VertexAttributeType::TexCoord0, VertexFormat::Float2);
  packed.addAttribute(VertexAttributeType::Color, VertexFormat::UByte4Normalized);
  packed.addAttribute(VertexAttributeType::Custom, VertexFormat::Short2Normalized);
  packed.addAttribute(VertexAttributeType::Custom, VertexFormat::UInt3);
  REQUIRE(packed.getAttributes()[3].offset == 20);
  REQUIRE(packed.getAttributes()[4].offset == 24);
  REQUIRE(packed.getStride(0) == 36);
}

namespace {
// The pipeline key only uses shader identity, so a shader needs no GPU code here.
class TestShader : public IShader
{
 public:
  chEngineSDK::UUID
  getShaderId() const override
  {
    return chEngineSDK::UUID::null();
  }
};

GraphicsPipelineDesc
makeTestPipelineDesc(const SPtr<IShader>& vertexShader, const SPtr<IShader>& fragmentShader)
{
  GraphicsPipelineDesc desc{.vertexShader = vertexShader,
                            .fragmentShader = fragmentShader,
                            .vertexLayout = VertexLayout::createStaticLayout(),
                            .colorAttachmentCount = 1,
                            .depthFormat = Format::D32_SFLOAT};
  desc.colorFormats[0] = Format::R8G8B8A8_UNORM;
  return desc;
}
} // namespace

TEST_CASE("chCore - GraphicsPipelineDesc hash")
{
  const SPtr<IShader> vertexShader = chMakeShared<TestShader>();
  const SPtr<IShader> fragmentShader = chMakeShared<TestShader>();
  const GraphicsPipelineDesc base = makeTestPipelineDesc(vertexShader, fragmentShader);

  SECTION("Equal descriptions give equal keys")
  {
    REQUIRE(base.getHash() == makeTestPipelineDesc(vertexShader, fragmentShader).getHash());
  }

  SECTION("Every kind of state changes the key")
  {
    GraphicsPipelineDesc otherShader = base;
    otherShader.fragmentShader = chMakeShared<TestShader>();
    REQUIRE(otherShader.getHash() != base.getHash());

    GraphicsPipelineDesc otherLayout = base;
    otherLayout.vertexLayout = VertexLayout::createPostionColorLayout();
    REQUIRE(otherLayout.getHash() != base.getHash());

    GraphicsPipelineDesc otherCull = base;
    otherCull.raster.cullMode = CullMode::None;
    REQUIRE(otherCull.getHash() != base.getHash());

    GraphicsPipelineDesc otherDepth = base;
    otherDepth.depth.writeEnable = false;
    REQUIRE(otherDepth.getHash() != base.getHash());

    GraphicsPipelineDesc otherBlend = base;
    otherBlend.blendStates[0].enable = true;
    REQUIRE(otherBlend.getHash() != base.getHash());

    GraphicsPipelineDesc otherFormat = base;
    otherFormat.colorFormats[0] = Format::R16G16B16A16_SFLOAT;
    REQUIRE(otherFormat.getHash() != base.getHash());

    GraphicsPipelineDesc moreTargets = base;
    moreTargets.colorAttachmentCount = 2;
    moreTargets.colorFormats[1] = Format::R8G8B8A8_UNORM;
    REQUIRE(moreTargets.getHash() != base.getHash());
  }

  SECTION("Unused attachment slots do not change the key")
  {
    GraphicsPipelineDesc unusedSlot = base;
    unusedSlot.colorFormats[3] = Format::R16G16B16A16_SFLOAT;
    unusedSlot.blendStates[3].enable = true;
    REQUIRE(unusedSlot.getHash() == base.getHash());
  }
}

TEST_CASE("chCore - FormatUtils")
{
  // Texture assets store these values, so they must never move.
  STATIC_REQUIRE(static_cast<uint32>(Format::R8G8B8A8_UNORM) == 1);
  STATIC_REQUIRE(static_cast<uint32>(Format::D24_UNORM_S8_UINT) == 6);

  SECTION("Every format but Unknown has a size")
  {
    REQUIRE(FormatUtils::getInfo(Format::Unknown).bytesPerBlock == 0);
    for (uint32 i = 1; i < static_cast<uint32>(Format::COUNT); ++i) {
      REQUIRE(FormatUtils::getInfo(static_cast<Format>(i)).bytesPerBlock > 0);
    }
  }

  SECTION("Depth and stencil")
  {
    REQUIRE(FormatUtils::isDepth(Format::D32_SFLOAT));
    REQUIRE_FALSE(FormatUtils::getInfo(Format::D32_SFLOAT).hasStencil);
    REQUIRE(FormatUtils::getInfo(Format::D24_UNORM_S8_UINT).hasStencil);
    REQUIRE(FormatUtils::getInfo(Format::D32_SFLOAT_S8_UINT).hasStencil);
    REQUIRE_FALSE(FormatUtils::isDepth(Format::R8G8B8A8_SRGB));
    REQUIRE(FormatUtils::getInfo(Format::R8G8B8A8_SRGB).isSrgb);
  }

  SECTION("Mip size")
  {
    REQUIRE(FormatUtils::getMipSize(Format::R8G8B8A8_UNORM, 4, 2) == 32);
    REQUIRE(FormatUtils::getMipSize(Format::R8_UNORM, 3, 3) == 9);
    REQUIRE(FormatUtils::getMipSize(Format::R32G32B32A32_SFLOAT, 1, 1, 2) == 32);
  }
}

namespace {

uint32
addTestNode(Model& model,
            StringView name,
            uint32 parent,
            const Matrix4& localMatrix,
            const Vector<uint32>& meshIndices)
{
  return model.addNode(name, parent, localMatrix, meshIndices);
}

// One triangle in front of the test camera when the object is at the origin.
SPtr<Model>
createTriangleModel()
{
  SPtr<Mesh> mesh = chMakeShared<Mesh>();
  mesh->setVertexData(Vector<VertexStatic>{
      {Vector3(1.0f, 0.0f, 0.0f), Vector3::UP, Vector2(0.0f, 0.0f),
       Vector4(1.0f, 0.0f, 0.0f, 1.0f)},
      {Vector3(-1.0f, 0.0f, 0.0f), Vector3::UP, Vector2(1.0f, 0.0f),
       Vector4(1.0f, 0.0f, 0.0f, 1.0f)},
      {Vector3(0.0f, 2.0f, 0.0f), Vector3::UP, Vector2(0.0f, 1.0f),
       Vector4(1.0f, 0.0f, 0.0f, 1.0f)}});
  mesh->setIndexData(Vector<uint16>{0, 1, 2});

  SPtr<Model> model = chMakeShared<Model>();
  addTestNode(*model, "Root", ModelNode::NO_PARENT, Matrix4::IDENTITY, {model->addMesh(mesh)});
  return model;
}

SPtr<ModelAsset>
createTestModelAsset(const SPtr<Model>& model, StringView name = "SM_Test")
{
  AssetMetadata metadata;
  metadata.uuid = chEngineSDK::UUID::createRandom();
  metadata.assetType = AssetTypeTraits<ModelAsset>::getTypeId();
  StringUtils::copyToBuffer(metadata.name, name);
  return chMakeShared<ModelAsset>(metadata, model);
}

} // namespace

TEST_CASE("chCore - Mesh bounds")
{
  const SPtr<Model> model = createTriangleModel();
  const SphereBoxBounds& bounds = model->getMeshes()[0]->getBounds();
  REQUIRE(bounds.center.nearEqual(Vector3(0.0f, 1.0f, 0.0f)));
  REQUIRE(bounds.boxExtent.nearEqual(Vector3(1.0f, 1.0f, 0.0f)));
  // The farthest vertices from the center are (1, 0, 0) and (-1, 0, 0).
  REQUIRE(bounds.sphereRadius == Approx(Math::sqrt(2.0f)));

  const Mesh empty;
  REQUIRE(empty.getBounds().sphereRadius == 0.0f);
}

TEST_CASE("chCore - Scene transforms")
{
  Scene scene("Test", chEngineSDK::UUID::createRandom());
  const SPtr<GameObject> parent = scene.createGameObject("Parent");
  const SPtr<GameObject> child = scene.createGameObject("Child", parent.get());
  REQUIRE(scene.getRootGameObjects().size() == 1);
  REQUIRE(child->getParent() == parent.get());
  REQUIRE(child->getScene() == &scene);

  parent->getTransform().setLocalPosition(Vector3(10.0f, 0.0f, 0.0f));
  parent->getTransform().setLocalRotation(Quaternion(Rotator(0.0f, 90.0f, 0.0f)));
  child->getTransform().setLocalPosition(Vector3(1.0f, 0.0f, 0.0f));
  REQUIRE(child->getTransform().isDirty());

  scene.updateTransforms();
  REQUIRE_FALSE(child->getTransform().isDirty());
  // The parent's yaw turns the child's forward offset to the right.
  REQUIRE(child->getTransform().getWorldPosition().nearEqual(Vector3(10.0f, 1.0f, 0.0f),
                                                             1e-5f));

  // Moving only the parent still moves the child.
  parent->getTransform().setLocalPosition(Vector3(0.0f, 0.0f, 5.0f));
  scene.updateTransforms();
  REQUIRE(child->getTransform().getWorldPosition().nearEqual(Vector3(0.0f, 1.0f, 5.0f),
                                                             1e-5f));

  scene.destroyGameObject(*parent);
  REQUIRE(scene.getRootGameObjects().empty());
  REQUIRE(parent->getScene() == nullptr);
  REQUIRE(child->getScene() == nullptr);
}

TEST_CASE("chCore - Scene render items")
{
  Scene scene("Test", chEngineSDK::UUID::createRandom());
  const SPtr<GameObject> gameObject = scene.createGameObject("Model");
  const SPtr<Model> model = createTriangleModel();
  ModelComponent& modelComponent =
      gameObject->addComponent<ModelComponent>(createTestModelAsset(model));
  REQUIRE(gameObject->getComponent<ModelComponent>() == &modelComponent);
  REQUIRE(modelComponent.isRegistered());
  REQUIRE(scene.getRenderItemCount() == 1);

  const Camera camera = createTestCamera();
  Vector<const RenderItem*> visible;
  scene.gatherRenderItems(camera.getFrustum(), visible);
  REQUIRE(visible.size() == 1);
  REQUIRE(visible[0]->mesh == model->getMeshes()[0].get());

  SECTION("Items follow the object and are culled behind the camera")
  {
    gameObject->getTransform().setLocalPosition(Vector3(-20.0f, 0.0f, 0.0f));
    scene.updateTransforms();
    REQUIRE(visible[0]->worldBounds.center.nearEqual(Vector3(-20.0f, 1.0f, 0.0f)));
    scene.gatherRenderItems(camera.getFrustum(), visible);
    REQUIRE(visible.empty());
    AABox bounds(Vector3::ZERO, Vector3::ZERO);
    REQUIRE(modelComponent.getWorldBounds(bounds));
    REQUIRE(bounds.getCenter().nearEqual(Vector3(-20.0f, 1.0f, 0.0f)));
  }

  SECTION("Disabling unregisters")
  {
    modelComponent.setEnabled(false);
    REQUIRE_FALSE(modelComponent.isRegistered());
    REQUIRE(scene.getRenderItemCount() == 0);
    modelComponent.setEnabled(true);
    REQUIRE(scene.getRenderItemCount() == 1);
  }

  SECTION("Destroying the object removes its items")
  {
    scene.destroyGameObject(*gameObject);
    REQUIRE(scene.getRenderItemCount() == 0);
    REQUIRE_FALSE(modelComponent.isRegistered());

    // A freed slot is reused.
    const SPtr<GameObject> other = scene.createGameObject("Other");
    other->addComponent<ModelComponent>(createTestModelAsset(createTriangleModel()));
    REQUIRE(scene.getRenderItemCount() == 1);
  }
}

TEST_CASE("chCore - Scene raycast")
{
  Scene scene("Test", chEngineSDK::UUID::createRandom());
  // The triangle lies flat at its object's Z, covering x -1..1 and y 0..2 (times scale).
  const SPtr<GameObject> back = scene.createGameObject("Back");
  back->addComponent<ModelComponent>(createTestModelAsset(createTriangleModel()));
  const SPtr<GameObject> front = scene.createGameObject("Front");
  front->addComponent<ModelComponent>(createTestModelAsset(createTriangleModel()));
  front->getTransform().setLocalPosition(Vector3(3.0f, 0.0f, -2.0f));
  front->getTransform().setLocalScale(Vector3(2.0f, 3.0f, 1.0f));
  scene.updateTransforms();

  SceneRayHit hit;
  REQUIRE(scene.raycast(Ray(Vector3(3.0f, 0.5f, -5.0f), Vector3::UP), hit));
  REQUIRE(hit.gameObject == front.get());
  REQUIRE(hit.distance == Approx(3.0f));

  REQUIRE(scene.raycast(Ray(Vector3(0.0f, 0.5f, -5.0f), Vector3::UP), hit));
  REQUIRE(hit.gameObject == back.get());
  REQUIRE(hit.distance == Approx(5.0f));

  // Inside the back triangle's box but outside the triangle, and behind the ray.
  REQUIRE_FALSE(scene.raycast(Ray(Vector3(0.95f, 1.95f, -5.0f), Vector3::UP), hit));
  REQUIRE_FALSE(scene.raycast(Ray(Vector3(3.0f, 0.5f, -5.0f), -Vector3::UP), hit));

  // In front of the back triangle, the front one is picked.
  back->getTransform().setLocalPosition(Vector3(3.0f, 0.0f, 0.0f));
  scene.updateTransforms();
  REQUIRE(scene.raycast(Ray(Vector3(3.0f, 0.5f, -5.0f), Vector3::UP), hit));
  REQUIRE(hit.gameObject == front.get());
}

TEST_CASE("chCore - Scene lights")
{
  Scene scene("Test", chEngineSDK::UUID::createRandom());
  const Camera camera = createTestCamera();
  Vector<const LightItem*> visible;

  // Positive pitch turns forward up, so -90 makes the sun shine straight down.
  const SPtr<GameObject> sun = scene.createGameObject("Sun");
  sun->getTransform().setLocalRotation(Quaternion(Rotator(-90.0f, 0.0f, 0.0f)));
  DirectionalLightComponent& sunLight = sun->addComponent<DirectionalLightComponent>();
  sunLight.setColor(LinearColor(1.0f, 0.5f, 0.25f, 1.0f));
  sunLight.setIntensity(2.0f);
  scene.updateTransforms();
  REQUIRE(scene.getLightCount() == 1);

  scene.gatherLights(camera.getFrustum(), visible);
  REQUIRE(visible.size() == 1);
  REQUIRE(visible[0]->type == LightType::Directional);
  REQUIRE(visible[0]->owner == sun.get());
  REQUIRE(visible[0]->direction.nearEqual(Vector3(0.0f, 0.0f, -1.0f)));
  REQUIRE(visible[0]->color.r == 2.0f);
  REQUIRE(visible[0]->color.g == 1.0f);
  REQUIRE(visible[0]->color.b == 0.5f);
  // Only directional lights cast shadows by default.
  REQUIRE(visible[0]->bCastShadows);

  const SPtr<GameObject> lamp = scene.createGameObject("Lamp");
  PointLightComponent& lampLight = lamp->addComponent<PointLightComponent>();
  lampLight.setRange(5.0f);
  scene.updateTransforms();
  REQUIRE(scene.getLightCount() == 2);
  REQUIRE_FALSE(lampLight.getCastShadows());

  SECTION("Point lights are culled by their range")
  {
    lamp->getTransform().setLocalPosition(Vector3(-20.0f, 0.0f, 0.0f));
    scene.updateTransforms();
    scene.gatherLights(camera.getFrustum(), visible);
    REQUIRE(visible.size() == 1);

    // Behind the camera, but its range reaches what the camera sees.
    lampLight.setRange(30.0f);
    scene.gatherLights(camera.getFrustum(), visible);
    REQUIRE(visible.size() == 2);
    REQUIRE(visible[1]->position.nearEqual(Vector3(-20.0f, 0.0f, 0.0f)));
    REQUIRE(visible[1]->range == 30.0f);
  }

  SECTION("Spot cone angles are kept in order")
  {
    const SPtr<GameObject> spot = scene.createGameObject("Spot");
    SpotLightComponent& spotLight = spot->addComponent<SpotLightComponent>();
    spotLight.setConeAngles(Degree(60.0f), Degree(30.0f));
    REQUIRE(spotLight.getOuterConeAngle().valueDegree() == 30.0f);
    REQUIRE(spotLight.getInnerConeAngle().valueDegree() == 30.0f);
    spotLight.setConeAngles(Degree(20.0f), Degree(120.0f));
    REQUIRE(spotLight.getOuterConeAngle().valueDegree() == 89.0f);

    scene.updateTransforms();
    scene.gatherLights(camera.getFrustum(), visible);
    REQUIRE(visible.size() == 3);
    REQUIRE(visible[2]->type == LightType::Spot);
    REQUIRE(Math::abs(visible[2]->innerConeCos - Math::cos(Degree(20.0f))) < 1e-5f);
    REQUIRE(Math::abs(visible[2]->outerConeCos - Math::cos(Degree(89.0f))) < 1e-5f);
  }

  SECTION("Disabling or destroying removes the light")
  {
    lampLight.setEnabled(false);
    REQUIRE(scene.getLightCount() == 1);
    lampLight.setEnabled(true);
    REQUIRE(scene.getLightCount() == 2);
    scene.destroyGameObject(*lamp);
    REQUIRE(scene.getLightCount() == 1);
  }
}

namespace {

// Stands in for a swap chain image: the graph only reads its description.
class FakeTexture : public ITexture
{
 public:
  FakeTexture(Format format, uint32 width, uint32 height)
    : m_format(format),
      m_width(width),
      m_height(height)
  {}

  NODISCARD TextureType
  getType() const override { return TextureType::Texture2D; }

  NODISCARD Format
  getFormat() const override { return m_format; }

  NODISCARD uint32
  getWidth() const override { return m_width; }

  NODISCARD uint32
  getHeight() const override { return m_height; }

  NODISCARD uint32
  getDepth() const override { return 1; }

  NODISCARD uint32
  getMipLevels() const override { return 1; }

  NODISCARD uint32
  getArrayLayers() const override { return 1; }

  NODISCARD uint32
  getBindlessIndex() const override { return GraphicsLimits::INVALID_BINDLESS_INDEX; }

  NODISCARD SPtr<ITextureView>
  createView(const TextureViewCreateInfo& createInfo) override
  {
    CH_PARAMETER_UNUSED(createInfo);
    return nullptr;
  }

  void
  uploadData(const void* data, SIZE_T size) override
  {
    CH_PARAMETER_UNUSED(data);
    CH_PARAMETER_UNUSED(size);
  }

 private:
  Format m_format;
  uint32 m_width;
  uint32 m_height;
};

class FakeTextureView : public ITextureView
{
 public:
  NODISCARD Format
  getFormat() const override { return Format::Unknown; }

  NODISCARD TextureViewType
  getViewType() const override { return TextureViewType::View2D; }

  NODISCARD uint32
  getBaseMipLevel() const override { return 0; }

  NODISCARD uint32
  getMipLevelCount() const override { return 1; }

  NODISCARD uint32
  getBaseArrayLayer() const override { return 0; }

  NODISCARD uint32
  getArrayLayerCount() const override { return 1; }

  NODISCARD uint32
  getBindlessIndex() const override { return GraphicsLimits::INVALID_BINDLESS_INDEX; }

  NODISCARD uint32
  getStorageBindlessIndex() const override { return GraphicsLimits::INVALID_BINDLESS_INDEX; }
};

bool
hasBarrier(Span<const RGBarrier> barriers,
           RGTextureHandle texture,
           ResourceState before,
           ResourceState after)
{
  for (const RGBarrier& barrier : barriers) {
    if (barrier.texture == texture && barrier.before == before && barrier.after == after) {
      return true;
    }
  }
  return false;
}

} // namespace

TEST_CASE("chCore - Directional shadow projection")
{
  Camera camera = createTestCamera();
  constexpr uint32 kMapSize = 1024;
  constexpr float kDistance = 20.0f;
  // Comes down at an angle, from behind the camera and to its left.
  const Vector3 lightDirection = Vector3(1.0f, 0.5f, -1.0f).getNormalized();
  const DirectionalShadowProjection projection =
      ShadowRenderer::computeDirectionalProjection(camera, lightDirection, 0.0f, kDistance,
                                                   kMapSize);

  const auto project = [](const Matrix4& viewProjection, const Vector3& point) {
    const Vector4 clip = viewProjection.transformVector4(Vector4(point.x, point.y, point.z, 1.0f));
    return Vector3(clip.x / clip.w, clip.y / clip.w, clip.z / clip.w);
  };

  SECTION("The covered part of the view is inside the map")
  {
    // A horizontal field of view of 90 degrees reaches as far sideways as forward.
    const Vector3 forward = camera.getForwardVector();
    const Vector3 right = camera.getRightVector();
    const Vector3 up = camera.getUpVector();
    constexpr float kAspect = 720.0f / 1280.0f;
    for (const float depth : {0.1f, 10.0f, kDistance}) {
      for (const float x : {-0.99f, 0.0f, 0.99f}) {
        for (const float y : {-0.99f, 0.0f, 0.99f}) {
          const Vector3 point = camera.getPosition() + forward * depth +
                                right * (x * depth) + up * (y * depth * kAspect);
          const Vector3 ndc = project(projection.viewProjection, point);
          REQUIRE(Math::abs(ndc.x) <= 1.0f);
          REQUIRE(Math::abs(ndc.y) <= 1.0f);
          REQUIRE(ndc.z >= 0.0f);
          REQUIRE(ndc.z <= 1.0f);
        }
      }
    }
  }

  SECTION("Casters toward the light are kept, casters beside the map are not")
  {
    const Vector3 viewCenter = camera.getPosition() + camera.getForwardVector() * 10.0f;
    const Vector3 towardLight = viewCenter - lightDirection * 1000.0f;
    REQUIRE(ShapeOverlap::frustumPoint(projection.casterFrustum, towardLight));
    // In front of the near plane: the depth clamp draws it at 0.
    REQUIRE(project(projection.viewProjection, towardLight).z < 0.0f);

    const Vector3 side = lightDirection.cross(Vector3::UP).getNormalized();
    REQUIRE_FALSE(ShapeOverlap::frustumPoint(projection.casterFrustum, viewCenter + side * 1000.0f));
    REQUIRE_FALSE(
        ShapeOverlap::frustumPoint(projection.casterFrustum, viewCenter + lightDirection * 1000.0f));
  }

  SECTION("The map moves in whole texels")
  {
    const Vector3 move(0.013f, 0.0271f, -0.0057f);
    Camera moved = camera;
    moved.setPosition(camera.getPosition() + move);
    moved.lookAt(camera.getLookAt() + move);
    const DirectionalShadowProjection movedProjection =
        ShadowRenderer::computeDirectionalProjection(moved, lightDirection, 0.0f, kDistance,
                                                     kMapSize);
    REQUIRE(movedProjection.texelWorldSize == projection.texelWorldSize);

    const Vector3 point(3.0f, 1.0f, 0.5f);
    const Vector3 before = project(projection.viewProjection, point);
    const Vector3 after = project(movedProjection.viewProjection, point);
    // NDC spans two units over the map.
    for (const float texels : {(after.x - before.x) * kMapSize * 0.5f,
                               (after.y - before.y) * kMapSize * 0.5f}) {
      REQUIRE(Math::abs(texels - Math::floor(texels + 0.5f)) < 0.01f);
    }
  }

  SECTION("A cascade covers its own slice of the view with smaller texels")
  {
    const DirectionalShadowProjection slice = ShadowRenderer::computeDirectionalProjection(
        camera, lightDirection, 5.0f, 10.0f, kMapSize);
    REQUIRE(slice.texelWorldSize < projection.texelWorldSize);

    const Vector3 forward = camera.getForwardVector();
    const Vector3 right = camera.getRightVector();
    const Vector3 up = camera.getUpVector();
    constexpr float kAspect = 720.0f / 1280.0f;
    for (const float depth : {5.0f, 7.5f, 10.0f}) {
      for (const float x : {-0.99f, 0.0f, 0.99f}) {
        for (const float y : {-0.99f, 0.0f, 0.99f}) {
          const Vector3 point = camera.getPosition() + forward * depth +
                                right * (x * depth) + up * (y * depth * kAspect);
          const Vector3 ndc = project(slice.viewProjection, point);
          REQUIRE(Math::abs(ndc.x) <= 1.0f);
          REQUIRE(Math::abs(ndc.y) <= 1.0f);
        }
      }
    }
  }
}

TEST_CASE("chCore - Shadow cascade splits")
{
  float splits[4];

  SECTION("Lambda 0 gives even steps")
  {
    ShadowRenderer::computeCascadeSplits(1.0f, 101.0f, 0.0f, splits);
    REQUIRE(splits[0] == Approx(26.0f));
    REQUIRE(splits[1] == Approx(51.0f));
    REQUIRE(splits[2] == Approx(76.0f));
    REQUIRE(splits[3] == 101.0f);
  }

  SECTION("Lambda 1 gives steps in proportion to the distance")
  {
    ShadowRenderer::computeCascadeSplits(1.0f, 10000.0f, 1.0f, splits);
    REQUIRE(splits[0] == Approx(10.0f));
    REQUIRE(splits[1] == Approx(100.0f));
    REQUIRE(splits[2] == Approx(1000.0f));
    REQUIRE(splits[3] == 10000.0f);
  }

  SECTION("Splits grow and the last one ends at the distance")
  {
    ShadowRenderer::computeCascadeSplits(0.1f, 200.0f, 0.8f, splits);
    REQUIRE(splits[0] > 0.1f);
    REQUIRE(splits[0] < splits[1]);
    REQUIRE(splits[1] < splits[2]);
    REQUIRE(splits[2] < splits[3]);
    REQUIRE(splits[3] == 200.0f);
  }

  SECTION("One cascade covers the whole distance")
  {
    ShadowRenderer::computeCascadeSplits(0.1f, 200.0f, 0.8f, Span<float>(splits, 1));
    REQUIRE(splits[0] == 200.0f);
  }
}

TEST_CASE("chCore - Spot shadow projection")
{
  constexpr uint32 kMapSize = 1024;
  const Degree halfAngle(30.0f);
  LightItem light;
  light.type = LightType::Spot;
  light.position = Vector3(0.0f, 0.0f, 2.0f);
  light.direction = Vector3::FORWARD;
  light.range = 10.0f;
  light.outerConeCos = Math::cos(Radian(halfAngle));
  const SpotShadowProjection projection =
      ShadowRenderer::computeSpotProjection(light, 0.1f, kMapSize);

  const auto project = [&projection](const Vector3& point) {
    const Vector4 clip =
        projection.viewProjection.transformVector4(Vector4(point.x, point.y, point.z, 1.0f));
    return Vector3(clip.x / clip.w, clip.y / clip.w, clip.z / clip.w);
  };

  SECTION("The axis is the center of the map, the range its far plane")
  {
    const Vector3 middle = project(light.position + light.direction * 5.0f);
    REQUIRE(middle.x == Approx(0.0f).margin(1e-5f));
    REQUIRE(middle.y == Approx(0.0f).margin(1e-5f));
    REQUIRE(middle.z > 0.0f);
    REQUIRE(middle.z < 1.0f);
    REQUIRE(project(light.position + light.direction * 9.9f).z < 1.0f);
    REQUIRE(project(light.position + light.direction * 10.5f).z > 1.0f);
  }

  SECTION("The cone fits the map")
  {
    for (const float degrees : {29.0f, 31.0f}) {
      const float side = 5.0f * Math::tan(Radian(Degree(degrees)));
      const Vector3 right = project(light.position + Vector3(5.0f, side, 0.0f));
      const Vector3 up = project(light.position + Vector3(5.0f, 0.0f, side));
      if (degrees < 30.0f) {
        REQUIRE(Math::abs(right.x) < 1.0f);
        REQUIRE(Math::abs(up.y) < 1.0f);
      }
      else {
        REQUIRE(Math::abs(right.x) > 1.0f);
        REQUIRE(Math::abs(up.y) > 1.0f);
      }
    }
    REQUIRE(projection.texelWorldSizePerDistance ==
            Approx(2.0f * Math::tan(Radian(halfAngle)) / kMapSize));
  }

  SECTION("Casters between the light and the near plane are kept, those behind it are not")
  {
    REQUIRE(ShapeOverlap::frustumPoint(projection.casterFrustum,
                                       light.position + light.direction * 0.05f));
    REQUIRE_FALSE(ShapeOverlap::frustumPoint(projection.casterFrustum,
                                             light.position - light.direction));
  }

  SECTION("The near plane stays under half the range")
  {
    const SpotShadowProjection farNear =
        ShadowRenderer::computeSpotProjection(light, 20.0f, kMapSize);
    REQUIRE(farNear.nearPlane == 5.0f);
    REQUIRE(projection.nearPlane == 0.1f);
  }
}

TEST_CASE("chCore - Point shadow projection")
{
  LightItem light;
  light.type = LightType::Point;
  light.position = Vector3(3.0f, -2.0f, 1.0f);
  light.range = 10.0f;
  const PointShadowProjection projection = ShadowRenderer::computePointProjection(light, 0.1f);

  const auto project = [&projection](uint32 face, const Vector3& point) {
    const Vector4 clip = projection.faceViewProjections[face].transformVector4(
        Vector4(point.x, point.y, point.z, 1.0f));
    return Vector3(clip.x / clip.w, clip.y / clip.w, clip.z / clip.w);
  };

  SECTION("Each face's image lands where cube map sampling reads it")
  {
    // Cube map face selection as Direct3D and Vulkan define it: for the major axis ma of
    // the direction, s = (sc / |ma| + 1) / 2 and t = (tc / |ma| + 1) / 2, with t going down
    // the image. A face's NDC x is then sc / |ma| and its NDC y is -tc / |ma|.
    const auto cubeCoords = [](uint32 face, const Vector3& direction, float& sc, float& tc) {
      switch (face) {
        case 0: sc = -direction.z; tc = -direction.y; return direction.x;
        case 1: sc = direction.z; tc = -direction.y; return -direction.x;
        case 2: sc = direction.x; tc = direction.z; return direction.y;
        case 3: sc = direction.x; tc = -direction.z; return -direction.y;
        case 4: sc = direction.x; tc = -direction.y; return direction.z;
        default: sc = -direction.x; tc = -direction.y; return -direction.z;
      }
    };
    const Vector3 directions[] = {Vector3(4.0f, 1.0f, -2.0f), Vector3(-5.0f, 2.0f, 1.0f),
                                  Vector3(1.0f, 3.0f, 2.0f),  Vector3(-2.0f, -4.0f, 1.0f),
                                  Vector3(1.0f, -2.0f, 6.0f), Vector3(2.0f, 1.0f, -3.0f)};
    for (uint32 face = 0; face < PointShadowProjection::FACE_COUNT; ++face) {
      const Vector3& direction = directions[face];
      float sc = 0.0f;
      float tc = 0.0f;
      const float major = cubeCoords(face, direction, sc, tc);
      REQUIRE(major > 0.0f);
      const Vector3 ndc = project(face, light.position + direction);
      REQUIRE(ndc.x == Approx(sc / major).margin(1e-4f));
      REQUIRE(ndc.y == Approx(-tc / major).margin(1e-4f));
    }
  }

  SECTION("The stored depth follows the distance along the face's axis")
  {
    for (const float distance : {0.5f, 3.0f, 9.0f}) {
      const Vector3 ndc = project(2, light.position + Vector3(0.5f, distance, -0.3f));
      REQUIRE(ndc.z ==
              Approx(projection.depthScale + projection.depthOffset / distance).margin(1e-5f));
    }
    REQUIRE(project(0, light.position + Vector3(9.9f, 0.0f, 0.0f)).z < 1.0f);
    REQUIRE(project(0, light.position + Vector3(10.5f, 0.0f, 0.0f)).z > 1.0f);
  }

  SECTION("Casters between the light and the near plane are kept, those behind it are not")
  {
    REQUIRE(ShapeOverlap::frustumPoint(projection.faceCasterFrustums[4],
                                       light.position + Vector3(0.0f, 0.0f, 0.05f)));
    REQUIRE_FALSE(ShapeOverlap::frustumPoint(projection.faceCasterFrustums[4],
                                             light.position - Vector3(0.0f, 0.0f, 1.0f)));
  }

  SECTION("The near plane stays under half the range")
  {
    REQUIRE(ShadowRenderer::computePointProjection(light, 20.0f).nearPlane == 5.0f);
  }
}

TEST_CASE("chCore - Spot and point shadow selection")
{
  // Every light stands on the X axis, the camera at the origin.
  const auto makeLight = [](LightType type, float x, float range, bool bCast) {
    LightItem light;
    light.type = type;
    light.position = Vector3(x, 0.0f, 0.0f);
    light.range = range;
    light.bCastShadows = bCast;
    return light;
  };
  const LightItem nearSmall = makeLight(LightType::Spot, 10.0f, 2.0f, true);
  const LightItem farBig = makeLight(LightType::Spot, 40.0f, 20.0f, true);
  const LightItem around = makeLight(LightType::Spot, 1.0f, 5.0f, true);
  const LightItem tooFar = makeLight(LightType::Spot, 500.0f, 10.0f, true);
  const LightItem noShadow = makeLight(LightType::Spot, 2.0f, 5.0f, false);
  const LightItem point = makeLight(LightType::Point, 2.0f, 5.0f, true);
  const LightItem* lights[] = {&nearSmall, &farBig, &around, &tooFar, &noShadow, &point};

  Vector<LocalShadowCandidate> candidates;
  ShadowRenderer::selectLocalShadows(Vector3::ZERO, lights, LightType::Spot, 8, 100.0f,
                                     candidates);

  SECTION("Only spot lights with shadows inside the distance, the biggest on screen first")
  {
    REQUIRE(candidates.size() == 3);
    REQUIRE(candidates[0].light == &around);
    REQUIRE(candidates[0].rangeDistance == 0.0f);
    REQUIRE(candidates[1].light == &farBig);
    REQUIRE(candidates[1].rangeDistance == Approx(20.0f));
    REQUIRE(candidates[2].light == &nearSmall);
    REQUIRE(candidates[2].rangeDistance == Approx(8.0f));
  }

  SECTION("At most maxCount are kept")
  {
    ShadowRenderer::selectLocalShadows(Vector3::ZERO, lights, LightType::Spot, 2, 100.0f,
                                       candidates);
    REQUIRE(candidates.size() == 2);
    REQUIRE(candidates[1].light == &farBig);
  }

  SECTION("The distance counts from the camera to the edge of the range")
  {
    ShadowRenderer::selectLocalShadows(Vector3::ZERO, lights, LightType::Spot, 8, 15.0f,
                                       candidates);
    REQUIRE(candidates.size() == 2);
    REQUIRE(candidates[1].light == &nearSmall);
  }

  SECTION("Point lights are picked on their own")
  {
    ShadowRenderer::selectLocalShadows(Vector3::ZERO, lights, LightType::Point, 8, 100.0f,
                                       candidates);
    REQUIRE(candidates.size() == 1);
    REQUIRE(candidates[0].light == &point);
  }
}

/**
 * Records what the render graph asks for, so execute() can be checked without a GPU.
 */
class FakeCommandList : public ICommandList
{
 public:
  void
  beginRendering(const RenderingDesc& desc) override
  {
    m_stats.recordRendering();
    renderings.push_back(desc);
  }

  void
  endRendering() override
  {}

  void
  barrier(Span<const TextureBarrier> textureBarriers) override
  {
    m_stats.recordBarriers(textureBarriers.size());
  }

  void
  memoryBarrier(ResourceState before, ResourceState after) override
  {
    CH_PARAMETER_UNUSED(before);
    CH_PARAMETER_UNUSED(after);
    m_stats.recordBarriers(1);
  }

  void
  bindPipeline(const IPipeline& pipeline) override
  {
    CH_PARAMETER_UNUSED(pipeline);
  }

  void
  pushConstants(const void* data, uint32 size, uint32 offset) override
  {
    CH_PARAMETER_UNUSED(data);
    CH_PARAMETER_UNUSED(size);
    CH_PARAMETER_UNUSED(offset);
  }

  void
  bindVertexBuffer(const IBuffer& buffer, uint32 binding, uint64 offset) override
  {
    CH_PARAMETER_UNUSED(buffer);
    CH_PARAMETER_UNUSED(binding);
    CH_PARAMETER_UNUSED(offset);
  }

  void
  bindIndexBuffer(const IBuffer& buffer, IndexType indexType, uint64 offset) override
  {
    CH_PARAMETER_UNUSED(buffer);
    CH_PARAMETER_UNUSED(indexType);
    CH_PARAMETER_UNUSED(offset);
  }

  void
  draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)
      override
  {
    CH_PARAMETER_UNUSED(firstVertex);
    CH_PARAMETER_UNUSED(firstInstance);
    m_stats.recordDraw(vertexCount, instanceCount);
  }

  void
  drawIndexed(uint32 indexCount,
              uint32 instanceCount,
              uint32 firstIndex,
              int32 vertexOffset,
              uint32 firstInstance) override
  {
    CH_PARAMETER_UNUSED(firstIndex);
    CH_PARAMETER_UNUSED(vertexOffset);
    CH_PARAMETER_UNUSED(firstInstance);
    m_stats.recordDraw(indexCount, instanceCount);
  }

  void
  dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) override
  {
    CH_PARAMETER_UNUSED(groupCountX);
    CH_PARAMETER_UNUSED(groupCountY);
    CH_PARAMETER_UNUSED(groupCountZ);
    m_stats.recordDispatch();
  }

  void
  setViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
      override
  {
    CH_PARAMETER_UNUSED(x);
    CH_PARAMETER_UNUSED(y);
    CH_PARAMETER_UNUSED(width);
    CH_PARAMETER_UNUSED(height);
    CH_PARAMETER_UNUSED(minDepth);
    CH_PARAMETER_UNUSED(maxDepth);
  }

  void
  setScissor(uint32 x, uint32 y, uint32 width, uint32 height) override
  {
    CH_PARAMETER_UNUSED(x);
    CH_PARAMETER_UNUSED(y);
    CH_PARAMETER_UNUSED(width);
    CH_PARAMETER_UNUSED(height);
  }

  void
  writeTimestamp(uint32 index) override
  {
    CH_PARAMETER_UNUSED(index);
  }

  Vector<RenderingDesc> renderings;
};

TEST_CASE("chCore - RenderGraph")
{
  const FakeTexture backBuffer(Format::B8G8R8A8_UNORM, 64, 32);
  const FakeTextureView backBufferView;
  const RGTextureDesc colorDesc{
      .format = Format::R16G16B16A16_SFLOAT, .width = 64, .height = 32};
  const RGTextureDesc depthDesc{.format = Format::D32_SFLOAT, .width = 64, .height = 32};

  RenderGraph graph;
  const auto importBackBuffer = [&]() {
    return graph.importTexture("BackBuffer", backBuffer, backBufferView,
                               ResourceState::Undefined, ResourceState::Present);
  };

  SECTION("Forward pass and UI over it")
  {
    const RGTextureHandle output = importBackBuffer();
    REQUIRE(graph.getTextureDesc(output).width == 64);
    const RGTextureHandle depth = graph.createTexture("Depth", depthDesc);
    graph.addPass("Forward").writeColor(output).writeDepth(depth);
    graph.addPass("UI").writeColor(output, LoadOp::Load);
    graph.compile();

    REQUIRE(graph.getCompiledPassCount() == 2);
    REQUIRE(graph.getCompiledPassName(0) == "Forward");
    REQUIRE(graph.getCompiledPassName(1) == "UI");

    const Span<const RGBarrier> forward = graph.getCompiledPassBarriers(0);
    REQUIRE(forward.size() == 2);
    REQUIRE(
        hasBarrier(forward, output, ResourceState::Undefined, ResourceState::RenderTarget));
    REQUIRE(hasBarrier(forward, depth, ResourceState::Undefined, ResourceState::DepthWrite));

    // Writing again in the same state still waits for the first pass.
    const Span<const RGBarrier> ui = graph.getCompiledPassBarriers(1);
    REQUIRE(ui.size() == 1);
    REQUIRE(hasBarrier(ui, output, ResourceState::RenderTarget, ResourceState::RenderTarget));

    const Span<const RGBarrier> finalBarriers = graph.getFinalBarriers();
    REQUIRE(finalBarriers.size() == 1);
    REQUIRE(hasBarrier(finalBarriers, output, ResourceState::RenderTarget,
                       ResourceState::Present));

    REQUIRE(graph.getTextureSlot(output) == RenderGraph::INVALID_INDEX);
    REQUIRE(graph.getTextureSlot(depth) == 0);
    REQUIRE(graph.getTextureUsage(depth) == TextureUsage::DepthStencil);
    REQUIRE(graph.getTransientSlotCount() == 1);
  }

  SECTION("Attachments are stored only when needed later")
  {
    const RGTextureHandle output = importBackBuffer();
    const RGTextureHandle depth = graph.createTexture("Depth", depthDesc);
    graph.addPass("Scene").writeColor(output).writeDepth(depth);
    graph.compile();
    REQUIRE(graph.getAttachmentStoreOp(0, output) == StoreOp::Store);
    REQUIRE(graph.getAttachmentStoreOp(0, depth) == StoreOp::DontCare);

    // An overlay that loads the depth makes the scene pass keep it; the overlay itself
    // is the last user.
    graph.reset();
    const RGTextureHandle output2 = importBackBuffer();
    const RGTextureHandle depth2 = graph.createTexture("Depth", depthDesc);
    graph.addPass("Scene").writeColor(output2).writeDepth(depth2);
    graph.addPass("Overlay").writeColor(output2, LoadOp::Load).writeDepth(depth2, LoadOp::Load);
    graph.compile();
    REQUIRE(graph.getAttachmentStoreOp(0, depth2) == StoreOp::Store);
    REQUIRE(graph.getAttachmentStoreOp(1, depth2) == StoreOp::DontCare);
    REQUIRE(graph.getAttachmentStoreOp(1, output2) == StoreOp::Store);
  }

  SECTION("Passes nobody needs are dropped")
  {
    const RGTextureHandle output = importBackBuffer();
    const RGTextureHandle unused = graph.createTexture("Unused", colorDesc);
    graph.addPass("Unused").writeColor(unused);
    graph.addPass("Overwritten").writeColor(output);
    graph.addPass("Final").writeColor(output, LoadOp::Clear);
    graph.compile();

    REQUIRE(graph.getCompiledPassCount() == 1);
    REQUIRE(graph.getCompiledPassName(0) == "Final");
    REQUIRE(graph.getTextureSlot(unused) == RenderGraph::INVALID_INDEX);
    REQUIRE(graph.getTransientSlotCount() == 0);
  }

  SECTION("Side effects and loads keep passes")
  {
    const RGTextureHandle output = importBackBuffer();
    const RGTextureHandle capture = graph.createTexture("Capture", colorDesc);
    graph.addPass("Capture").writeColor(capture).setSideEffect();
    graph.addPass("Base").writeColor(output);
    graph.addPass("Overlay").writeColor(output, LoadOp::Load);
    graph.compile();

    REQUIRE(graph.getCompiledPassCount() == 3);
  }

  SECTION("A texture read by a later pass")
  {
    const RGTextureHandle output = importBackBuffer();
    const RGTextureHandle color = graph.createTexture("Color", colorDesc);
    const RGTextureHandle depth = graph.createTexture("Depth", depthDesc);
    graph.addPass("Scene").writeColor(color).writeDepth(depth);
    graph.addPass("Resolve").read(color).read(depth).writeColor(output);
    graph.compile();

    REQUIRE(graph.getCompiledPassCount() == 2);
    const Span<const RGBarrier> resolve = graph.getCompiledPassBarriers(1);
    REQUIRE(
        hasBarrier(resolve, color, ResourceState::RenderTarget, ResourceState::ShaderRead));
    REQUIRE(hasBarrier(resolve, depth, ResourceState::DepthWrite, ResourceState::DepthRead));
    REQUIRE(graph.getTextureUsage(color) ==
            (TextureUsageFlags(TextureUsage::ColorAttachment) | TextureUsage::Sampled));
  }

  SECTION("Depth layers written by the pass itself")
  {
    const RGTextureHandle output = importBackBuffer();
    const RGTextureHandle cubes = graph.createTexture("Cubes", depthDesc);
    graph.addPass("CubeFaces").writeDepthLayers(cubes);
    graph.addPass("Lighting").read(cubes).writeColor(output);
    graph.compile();

    REQUIRE(graph.getCompiledPassCount() == 2);
    REQUIRE(hasBarrier(graph.getCompiledPassBarriers(0), cubes, ResourceState::Undefined,
                       ResourceState::DepthWrite));
    REQUIRE(hasBarrier(graph.getCompiledPassBarriers(1), cubes, ResourceState::DepthWrite,
                       ResourceState::DepthRead));
    REQUIRE(graph.getTextureUsage(cubes) ==
            (TextureUsageFlags(TextureUsage::DepthStencil) | TextureUsage::Sampled));
  }

  SECTION("Storage written by a compute pass")
  {
    const RGTextureHandle output = importBackBuffer();
    const RGTextureHandle lookup = graph.createTexture("Lookup", colorDesc);
    graph.addPass("Generate").writeStorage(lookup);
    graph.addPass("Refine").writeStorage(lookup, LoadOp::Load);
    graph.addPass("Lighting").read(lookup).writeColor(output);
    graph.compile();

    REQUIRE(graph.getCompiledPassCount() == 3);
    REQUIRE(hasBarrier(graph.getCompiledPassBarriers(0), lookup, ResourceState::Undefined,
                       ResourceState::UnorderedAccess));
    // The second writer waits for the first.
    REQUIRE(hasBarrier(graph.getCompiledPassBarriers(1), lookup,
                       ResourceState::UnorderedAccess, ResourceState::UnorderedAccess));
    REQUIRE(hasBarrier(graph.getCompiledPassBarriers(2), lookup,
                       ResourceState::UnorderedAccess, ResourceState::ShaderRead));
    REQUIRE(graph.getTextureUsage(lookup) ==
            (TextureUsageFlags(TextureUsage::Storage) | TextureUsage::Sampled));
  }

  SECTION("A storage write that nobody reads is dropped")
  {
    const RGTextureHandle output = importBackBuffer();
    const RGTextureHandle unused = graph.createTexture("Unused", colorDesc);
    graph.addPass("Generate").writeStorage(unused);
    graph.addPass("Lighting").writeColor(output);
    graph.compile();

    REQUIRE(graph.getCompiledPassCount() == 1);
    REQUIRE(graph.getCompiledPassName(0) == "Lighting");
  }

  SECTION("Dead textures give their slot to new ones")
  {
    const RGTextureHandle output = importBackBuffer();
    const RGTextureHandle first = graph.createTexture("First", colorDesc);
    const RGTextureHandle second = graph.createTexture("Second", colorDesc);
    const RGTextureHandle third = graph.createTexture("Third", colorDesc);
    const RGTextureHandle half = graph.createTexture(
        "Half", {.format = colorDesc.format, .width = 32, .height = 16});
    graph.addPass("A").writeColor(first);
    graph.addPass("B").read(first).writeColor(second);
    graph.addPass("C").read(second).writeColor(third);
    graph.addPass("D").read(third).writeColor(half);
    graph.addPass("E").read(half).writeColor(output);
    graph.compile();

    REQUIRE(graph.getCompiledPassCount() == 5);
    REQUIRE(graph.getTextureSlot(first) == 0);
    REQUIRE(graph.getTextureSlot(second) == 1);
    // First died in B, so Third (same description and usage) takes its slot in C.
    REQUIRE(graph.getTextureSlot(third) == 0);
    // Another size never shares.
    REQUIRE(graph.getTextureSlot(half) == 2);
    REQUIRE(graph.getTransientSlotCount() == 3);

    // The slot keeps the state First left it in.
    REQUIRE(hasBarrier(graph.getCompiledPassBarriers(2), third, ResourceState::ShaderRead,
                       ResourceState::RenderTarget));
  }

  SECTION("Imported textures")
  {
    const FakeTexture history(Format::R16G16B16A16_SFLOAT, 64, 32);
    const FakeTextureView historyView;

    SECTION("Without a final state, passes that only write it are dropped")
    {
      const RGTextureHandle scratch =
          graph.importTexture("Scratch", history, historyView, ResourceState::ShaderRead,
                              ResourceState::Undefined);
      graph.addPass("Write").writeColor(scratch);
      graph.compile();
      REQUIRE(graph.getCompiledPassCount() == 0);
      REQUIRE(graph.getFinalBarriers().empty());
    }

    SECTION("Untouched, it still reaches its final state")
    {
      const RGTextureHandle output = importBackBuffer();
      graph.compile();
      REQUIRE(graph.getCompiledPassCount() == 0);
      REQUIRE(hasBarrier(graph.getFinalBarriers(), output, ResourceState::Undefined,
                         ResourceState::Present));
    }

    SECTION("Already in its final state, no barrier")
    {
      const RGTextureHandle kept =
          graph.importTexture("History", history, historyView, ResourceState::ShaderRead,
                              ResourceState::ShaderRead);
      const RGTextureHandle output = importBackBuffer();
      graph.addPass("Use").read(kept).writeColor(output);
      graph.compile();
      REQUIRE(graph.getCompiledPassBarriers(0).size() == 1);
      REQUIRE(graph.getFinalBarriers().size() == 1);
    }
  }

  SECTION("A turned off optional pass skips its code but still clears its outputs")
  {
    const FakeTexture depthTexture(Format::D32_SFLOAT, 64, 32);
    const FakeTextureView depthView;
    int32 runs = 0;
    const auto addPasses = [&]() {
      graph.reset();
      const RGTextureHandle output = importBackBuffer();
      const RGTextureHandle depth =
          graph.importTexture("Depth", depthTexture, depthView, ResourceState::Undefined,
                              ResourceState::Undefined);
      graph.addPass("GBuffer")
          .setOptional()
          .writeColor(output, LoadOp::DontCare)
          .writeDepth(depth)
          .setExecute([&runs](RenderPassContext& context) {
            CH_PARAMETER_UNUSED(context);
            ++runs;
          });
      graph.addPass("UI").writeColor(output, LoadOp::Load).setExecute(
          [&runs](RenderPassContext& context) {
            CH_PARAMETER_UNUSED(context);
            runs += 10;
          });
      graph.compile();
    };

    TransientTexturePool pool;
    FakeCommandList commandList;
    addPasses();
    graph.execute(commandList, pool);
    REQUIRE(runs == 11);
    REQUIRE(commandList.renderings[0].colorAttachments[0].loadOp == LoadOp::DontCare);

    // Not optional passes ignore the list.
    const Vector<String> disabled{"Other", "GBuffer", "UI"};
    graph.setDisabledPasses(disabled);
    commandList.renderings.clear();
    runs = 0;
    addPasses();
    graph.execute(commandList, pool);
    REQUIRE(runs == 10);
    REQUIRE(commandList.renderings.size() == 2);
    REQUIRE(commandList.renderings[0].colorAttachments[0].loadOp == LoadOp::Clear);
    REQUIRE(commandList.renderings[0].depthAttachment.loadOp == LoadOp::Clear);
    REQUIRE(commandList.renderings[1].colorAttachments[0].loadOp == LoadOp::Load);

#if USING(CH_PROFILER)
    const Span<const RenderPassStats> stats = graph.getExecutedPassStats();
    REQUIRE(stats.size() == 2);
    REQUIRE(stats[0].bOptional);
    REQUIRE_FALSE(stats[0].bRan);
    REQUIRE(stats[0].commands.renderings == 1);
    REQUIRE_FALSE(stats[1].bOptional);
    REQUIRE(stats[1].bRan);
#endif // USING(CH_PROFILER)
    graph.setDisabledPasses({});
  }

  SECTION("Reset frees the pass code and the graph builds again")
  {
    const SPtr<int32> counter = chMakeShared<int32>(0);
    for (int32 frame = 0; frame < 3; ++frame) {
      graph.reset();
      const RGTextureHandle output = importBackBuffer();
      graph.addPass("Forward").writeColor(output).setExecute(
          [counter](RenderPassContext& context) {
            CH_PARAMETER_UNUSED(context);
            ++*counter;
          });
      REQUIRE(counter.use_count() == 2);
      graph.compile();
      REQUIRE(graph.getCompiledPassCount() == 1);
    }
    graph.reset();
    REQUIRE(counter.use_count() == 1);
  }
}

TEST_CASE("chCore - Render settings names")
{
  for (uint32 i = 0; i < static_cast<uint32>(ViewMode::COUNT); ++i) {
    const ViewMode mode = static_cast<ViewMode>(i);
    REQUIRE(RenderSettingsUtils::viewModeFromName(RenderSettingsUtils::getName(mode)) == mode);
  }
  REQUIRE(RenderSettingsUtils::viewModeFromName("wireFRAME") == ViewMode::Wireframe);
  REQUIRE_FALSE(RenderSettingsUtils::viewModeFromName("Unknown").has_value());
  REQUIRE(RenderSettingsUtils::viewModeFromName(g_cvarViewMode.getDefault()) == ViewMode::Lit);

  for (uint32 i = 0; i < static_cast<uint32>(RenderPathType::COUNT); ++i) {
    const RenderPathType path = static_cast<RenderPathType>(i);
    REQUIRE(RenderSettingsUtils::renderPathFromName(RenderSettingsUtils::getName(path)) ==
            path);
  }
  REQUIRE(RenderSettingsUtils::renderPathFromName("forward") == RenderPathType::Forward);
  REQUIRE_FALSE(RenderSettingsUtils::renderPathFromName("Unknown").has_value());
  REQUIRE(RenderSettingsUtils::renderPathFromName(g_cvarRenderPath.getDefault()) ==
          RenderPathType::Deferred);
}

namespace {

// Material assets only need valid metadata until they are saved or loaded.
SPtr<MaterialAsset>
createTestMaterialAsset(StringView name)
{
  AssetMetadata metadata;
  metadata.uuid = chEngineSDK::UUID::createRandom();
  metadata.assetType = AssetTypeTraits<MaterialAsset>::getTypeId();
  StringUtils::copyToBuffer(metadata.name, name);
  return chMakeShared<MaterialAsset>(metadata);
}

} // namespace

TEST_CASE("chCore - Asset file start")
{
  const chEngineSDK::UUID uuid = chEngineSDK::UUID::createRandom();
  const chEngineSDK::UUID assetType = AssetTypeTraits<MaterialAsset>::getTypeId();
  const chEngineSDK::UUID references[] = {chEngineSDK::UUID::createRandom(),
                                          chEngineSDK::UUID::createRandom()};
  const String editorText = "[Asset]\nCreated=1\n";

  MemoryDataStream stream(4096);
  AssetFile::writeStart(stream, uuid, assetType, references, editorText);
  // Only the identity follows the header; the name and folder come from the file's place.
  const SIZE_T referenceCountOffset = sizeof(AssetFileHeader) + 2 * sizeof(chEngineSDK::UUID);
  const SIZE_T editorDataOffset = referenceCountOffset + sizeof(uint32) + sizeof(references);
  REQUIRE(stream.tell() == editorDataOffset + sizeof(uint32) + editorText.size());
  AssetFile::writeString(stream, "Slot");
  stream.seek(0);

  AssetFileInfo info;
  Vector<chEngineSDK::UUID> readReferences;
  REQUIRE(AssetFile::readInfo(stream, info));
  REQUIRE(info.formatVersion == AssetFileHeader::FORMAT_VERSION);
  REQUIRE(info.uuid == uuid);
  REQUIRE(info.assetType == assetType);
  REQUIRE(AssetFile::readReferences(stream, readReferences));
  REQUIRE(readReferences.size() == 2);
  REQUIRE(readReferences[0] == references[0]);
  REQUIRE(readReferences[1] == references[1]);
  String editorData;
  REQUIRE(AssetFile::readEditorData(stream, &editorData));
  REQUIRE(editorData == editorText);
  String text;
  REQUIRE(AssetFile::readString(stream, text, 64));
  REQUIRE(text == "Slot");

  SECTION("The game moves past the editor data without reading it")
  {
    stream.seek(editorDataOffset);
    REQUIRE(AssetFile::readEditorData(stream, nullptr));
    REQUIRE(AssetFile::readString(stream, text, 64));
    REQUIRE(text == "Slot");
  }

  SECTION("A file without the magic number is rejected")
  {
    MemoryDataStream other(64);
    const uint32 junk[2] = {0x12345678, AssetFileHeader::FORMAT_VERSION};
    other.write(junk, sizeof(junk));
    other.seek(0);
    REQUIRE_FALSE(AssetFile::readInfo(other, info));
  }

  SECTION("Other format versions are rejected")
  {
    for (const uint32 version : {AssetFileHeader::FORMAT_VERSION - 1,
                                 AssetFileHeader::FORMAT_VERSION + 1}) {
      MemoryDataStream other(64);
      const uint32 header[2] = {AssetFileHeader::MAGIC, version};
      other.write(header, sizeof(header));
      other.seek(0);
      REQUIRE_FALSE(AssetFile::readInfo(other, info));
    }
  }

  SECTION("Broken lengths are rejected")
  {
    stream.seek(referenceCountOffset);
    const uint32 tooMany = AssetFile::MAX_REFERENCES + 1;
    stream.write(&tooMany, sizeof(tooMany));
    stream.seek(referenceCountOffset);
    REQUIRE_FALSE(AssetFile::readReferences(stream, readReferences));
    REQUIRE(readReferences.empty());

    stream.seek(editorDataOffset);
    const uint32 tooLong = AssetFile::MAX_EDITOR_DATA_SIZE + 1;
    stream.write(&tooLong, sizeof(tooLong));
    stream.seek(editorDataOffset);
    REQUIRE_FALSE(AssetFile::readEditorData(stream, &editorData));
  }

  SECTION("Rewriting replaces only the editor data")
  {
    const Path root = FileSystem::absolutePath(Path("AssetFileTestDir"));
    FileSystem::removeAll(root);
    const Path source = root.join(Path("Rock.chAss"));
    const Path stripped = root.join(Path("Out/Rock.chAss"));
    SPtr<DataStream> file = FileSystem::createAndOpenFile(source);
    REQUIRE(file);
    stream.seek(0);
    Vector<uint8> bytes(stream.size());
    REQUIRE(stream.read(bytes.data(), bytes.size()) == bytes.size());
    file->write(bytes.data(), bytes.size());
    file->close();

    REQUIRE(AssetFile::rewriteWithEditorData(source, stripped, StringView()));
    SPtr<DataStream> result = FileSystem::openFile(stripped);
    REQUIRE(result);
    REQUIRE(AssetFile::readInfo(*result, info));
    REQUIRE(info.uuid == uuid);
    REQUIRE(AssetFile::readReferences(*result, readReferences));
    REQUIRE(readReferences.size() == 2);
    REQUIRE(AssetFile::readEditorData(*result, &editorData));
    REQUIRE(editorData.empty());
    REQUIRE(AssetFile::readString(*result, text, 64));
    REQUIRE(text == "Slot");
    result->close();

    // In place, as the editor would.
    REQUIRE(AssetFile::rewriteWithEditorData(stripped, stripped, "[Asset]\nCreated=2\n"));
    result = FileSystem::openFile(stripped);
    REQUIRE(AssetFile::readInfo(*result, info));
    REQUIRE(AssetFile::readReferences(*result, readReferences));
    REQUIRE(AssetFile::readEditorData(*result, &editorData));
    REQUIRE(editorData == "[Asset]\nCreated=2\n");
    REQUIRE(AssetFile::readString(*result, text, 64));
    REQUIRE(text == "Slot");
    result->close();
    FileSystem::removeAll(root);
  }
}

TEST_CASE("chCore - Script UUID line")
{
  const chEngineSDK::UUID uuid = chEngineSDK::UUID::createRandom();
  chEngineSDK::UUID read = chEngineSDK::UUID::null();
  REQUIRE_FALSE(ScriptAsset::readUUIDLine("local A = {}\n", read));
  REQUIRE(read.isNull());

  const String added = ScriptAsset::withUUIDLine("local A = {}\nreturn A\n", uuid);
  REQUIRE(added == "-- chimera-uuid: " + uuid.toString() + "\nlocal A = {}\nreturn A\n");
  REQUIRE(ScriptAsset::readUUIDLine(added, read));
  REQUIRE(read == uuid);

  // An old UUID line is replaced, and the file keeps its line endings.
  const chEngineSDK::UUID other = chEngineSDK::UUID::createRandom();
  const String crlf = ScriptAsset::withUUIDLine("local A = {}\r\nreturn A\r\n", uuid);
  const String replaced = ScriptAsset::withUUIDLine(crlf, other);
  REQUIRE(replaced == "-- chimera-uuid: " + other.toString() + "\r\nlocal A = {}\r\nreturn A\r\n");
  REQUIRE(ScriptAsset::readUUIDLine(replaced, read));
  REQUIRE(read == other);

  REQUIRE(ScriptAsset::withUUIDLine("", uuid) == "-- chimera-uuid: " + uuid.toString() + "\n");
  REQUIRE_FALSE(ScriptAsset::readUUIDLine("-- chimera-uuid: not-a-uuid\n", read));
}

#if USING(CH_EDITOR)
TEST_CASE("chCore - Asset editor data")
{
  const Path project = FileSystem::absolutePath(Path("MetaTestProject"));
  ImportSettings settings;
  settings.srgb = false;
  settings.expandMode = ModelExpandMode::PerMesh;
  settings.splitParts = true;
  settings.autoAlphaMask = false;
  settings.importModel = false;
  const AssetEditorData data{.creationTime = 1760000000,
                             .engineVersion = "1.2.3",
                             .sourceFile = project.join(Path("../Art/Sponza.gltf")),
                             .sourcePart = "Material #1",
                             .importSettings = settings};

  ConfigFile file;
  AssetMeta::write(data, project, file);
  // Relative to the project, so the project can move together with its sources.
  REQUIRE(file.getValue("Import", "Source"));
  REQUIRE(*file.getValue("Import", "Source") == "../Art/Sponza.gltf");
  REQUIRE(*file.getValue("Import", "ExpandMode") == "PerMesh");

  ConfigFile parsed;
  parsed.parse(file.toText());
  AssetEditorData read;
  AssetMeta::read(parsed, project, read);
  REQUIRE(read.creationTime == data.creationTime);
  REQUIRE(read.engineVersion == "1.2.3");
  REQUIRE(read.sourceFile == FileSystem::absolutePath(data.sourceFile));
  REQUIRE(read.sourcePart == "Material #1");
  REQUIRE_FALSE(read.importSettings.srgb);
  REQUIRE(read.importSettings.generateMips);
  REQUIRE(read.importSettings.expandMode == ModelExpandMode::PerMesh);
  REQUIRE(read.importSettings.splitParts);
  REQUIRE_FALSE(read.importSettings.autoAlphaMask);
  REQUIRE_FALSE(read.importSettings.importModel);

  SECTION("Missing and broken values keep the defaults")
  {
    ConfigFile broken;
    broken.parse("[Import]\nExpandMode=Sideways\nSplitParts=maybe\n");
    AssetEditorData defaults;
    AssetMeta::read(broken, project, defaults);
    REQUIRE(defaults.creationTime == 0);
    REQUIRE(defaults.sourceFile.empty());
    REQUIRE(defaults.importSettings.expandMode == ModelExpandMode::Single);
    REQUIRE_FALSE(defaults.importSettings.splitParts);
  }
}

namespace {
void
writeTestFile(const Path& path, StringView text)
{
  SPtr<DataStream> stream = FileSystem::createAndOpenFile(path);
  REQUIRE(stream);
  stream->write(text.data(), text.size());
  stream->close();
}

// An asset file start with editorData, then data as the asset's own bytes.
void
writeTestAsset(const Path& path, StringView editorData, StringView data)
{
  SPtr<DataStream> stream = FileSystem::createAndOpenFile(path);
  REQUIRE(stream);
  AssetFile::writeStart(*stream, chEngineSDK::UUID::createRandom(),
                        AssetTypeTraits<SceneAsset>::getTypeId(), {}, editorData);
  stream->write(data.data(), data.size());
  stream->close();
}
} // namespace

TEST_CASE("chCore - Project descriptor")
{
  const Path root = FileSystem::absolutePath(Path("ProjectDescriptorTestDir"));
  FileSystem::removeAll(root);

  SECTION("An empty file is a project named after the file")
  {
    const Path file = root.join(Path("Old/Old.chproject"));
    writeTestFile(file, "");
    ProjectDescriptor descriptor;
    REQUIRE(descriptor.load(file));
    REQUIRE(descriptor.name == "Old");
    REQUIRE(descriptor.engineVersion.empty());
    REQUIRE(descriptor.gameModules.empty());
    REQUIRE(ProjectDescriptor::findProjectFile(root.join(Path("Old"))) == file);
    REQUIRE(ProjectDescriptor::findProjectFile(root).empty());
  }

  SECTION("Saved values read back")
  {
    ProjectDescriptor descriptor;
    descriptor.name = "My Game";
    descriptor.engineVersion = "1.2.3.4";
    descriptor.description = "A test";
    descriptor.gameModules = {"GameCode", "Tools"};
    const Path file = root.join(Path("Saved/Saved.chproject"));
    REQUIRE(descriptor.save(file));

    ProjectDescriptor loaded;
    REQUIRE(loaded.load(file));
    REQUIRE(loaded.name == "My Game");
    REQUIRE(loaded.engineVersion == "1.2.3.4");
    REQUIRE(loaded.description == "A test");
    REQUIRE(loaded.gameModules == Vector<String>{"GameCode", "Tools"});

    writeTestFile(file, "[Modules]\nGameModules= A , ,B\n");
    REQUIRE(loaded.load(file));
    REQUIRE(loaded.name == "Saved");
    REQUIRE(loaded.gameModules == Vector<String>{"A", "B"});
  }

  SECTION("Create makes the folders and the file")
  {
    Path projectFile;
    String error;
    REQUIRE(ProjectDescriptor::create(root, "NewGame", projectFile, error));
    REQUIRE(projectFile == root.join(Path("NewGame/NewGame.chproject")));
    REQUIRE(FileSystem::isDirectory(root.join(Path("NewGame/Assets"))));
    REQUIRE(FileSystem::isDirectory(root.join(Path("NewGame/Config"))));
    ProjectDescriptor descriptor;
    REQUIRE(descriptor.load(projectFile));
    REQUIRE(descriptor.name == "NewGame");
    REQUIRE(descriptor.engineVersion == CH_ENGINE_VERSION_STRING);

    // The folder is no longer empty.
    REQUIRE_FALSE(ProjectDescriptor::create(root, "NewGame", projectFile, error));
    REQUIRE_FALSE(error.empty());
    REQUIRE_FALSE(ProjectDescriptor::create(root, "Bad/Name", projectFile, error));
    REQUIRE_FALSE(ProjectDescriptor::create(Path(), "Fine", projectFile, error));
  }

  SECTION("Names")
  {
    REQUIRE(ProjectDescriptor::isValidName("My Game_2"));
    REQUIRE_FALSE(ProjectDescriptor::isValidName(""));
    REQUIRE_FALSE(ProjectDescriptor::isValidName(".."));
    REQUIRE_FALSE(ProjectDescriptor::isValidName("Game."));
    REQUIRE_FALSE(ProjectDescriptor::isValidName("a:b"));
  }

  FileSystem::removeAll(root);
}

TEST_CASE("chCore - Project packager")
{
  const Path root = FileSystem::absolutePath(Path("PackagerTestDir"));
  FileSystem::removeAll(root);

  const Path binaries = root.join(Path("Build"));
  String executable(ProjectPackager::GAME_EXECUTABLE_NAME);
  executable += ProjectPackager::EXECUTABLE_EXTENSION;
  String library("chCore");
  library += DynamicLibrary::EXTENSION;
  writeTestFile(binaries.join(Path(executable)), "exe");
  writeTestFile(binaries.join(Path(library)), "lib");
  writeTestFile(binaries.join(Path("chGame.pdb")), "symbols");
  writeTestFile(binaries.join(Path("Codecs/chImageCodec.bin")), "codec");
  writeTestFile(binaries.join(Path("Shaders/SPIRV/forward.ps.spv")), "spirv");

  const Path engine = root.join(Path("EngineContent"));
  writeTestFile(engine.join(Path("Config/Engine.ini")), "[Window]\nWidth=640\n");
  writeTestFile(engine.join(Path("Shaders/forward.hlsl")), "source");

  const Path project = root.join(Path("Project"));
  writeTestFile(project.join(Path("Project.chproject")), "[Modules]\nGameModules=GameCode\n");
  writeTestFile(project.join(Path("Config/Game.ini")), "[Game]\nTitle=Test\n");
  writeTestFile(project.join(Path("Config/Editor.ini")), "[Editor]\nGrid=false\n");
  writeTestFile(project.join(Path("Config/Package.ini")), "[Package]\nScenes=/Game/Maps/Main\n");
  const Path mainScene = project.join(Path("Assets/Maps/Main.chAss"));
  writeTestAsset(mainScene, "[Import]\nSource=C:/Users/Someone/Art/Main.fbx\n", "scene");
  writeTestFile(project.join(Path("Assets/Unused.chAss")), "unused");

  const Path output = root.join(Path("Out/MyGame"));
  PackageDesc desc{.gameName = "MyGame",
                   .outputDirectory = output,
                   .binaryDirectory = binaries,
                   .engineContentDirectory = engine,
                   .projectDirectory = project,
                   .assetFiles = {mainScene},
                   .startupScene = "/Game/Maps/Main"};

  SECTION("The package holds what a game reads and nothing else")
  {
    const PackageResult result = ProjectPackager::package(desc);
    REQUIRE(result.bSucceeded);
    REQUIRE(result.binaryFileCount == 3);
    REQUIRE(result.assetFileCount == 1);

    String packagedExecutable("MyGame");
    packagedExecutable += ProjectPackager::EXECUTABLE_EXTENSION;
    REQUIRE(FileSystem::isFile(output.join(Path(packagedExecutable))));
    REQUIRE_FALSE(FileSystem::exists(output.join(Path(executable))));
    REQUIRE(FileSystem::isFile(output.join(Path(library))));
    REQUIRE_FALSE(FileSystem::exists(output.join(Path("chGame.pdb"))));
    REQUIRE_FALSE(FileSystem::exists(output.join(Path("Codecs"))));
    REQUIRE(FileSystem::isFile(output.join(Path("Shaders/SPIRV/forward.ps.spv"))));
    REQUIRE(FileSystem::isFile(output.join(Path("Engine/Config/Engine.ini"))));
    REQUIRE_FALSE(FileSystem::exists(output.join(Path("Engine/Shaders"))));
    REQUIRE_FALSE(FileSystem::exists(output.join(Path("Config/Editor.ini"))));
    REQUIRE_FALSE(FileSystem::exists(output.join(Path("Config/Package.ini"))));
    REQUIRE(FileSystem::isFile(output.join(Path("Assets/Maps/Main.chAss"))));
    REQUIRE_FALSE(FileSystem::exists(output.join(Path("Assets/Unused.chAss"))));
    // The editor data (paths of this machine) stays out of the package; the data is kept.
    SPtr<DataStream> packagedScene =
        FileSystem::openFile(output.join(Path("Assets/Maps/Main.chAss")));
    REQUIRE(packagedScene);
    AssetFileInfo sceneInfo;
    Vector<chEngineSDK::UUID> sceneReferences;
    String sceneEditorData;
    REQUIRE(AssetFile::readInfo(*packagedScene, sceneInfo));
    REQUIRE(AssetFile::readReferences(*packagedScene, sceneReferences));
    REQUIRE(AssetFile::readEditorData(*packagedScene, &sceneEditorData));
    REQUIRE(sceneEditorData.empty());
    ANSICHAR sceneBytes[5] = {};
    REQUIRE(packagedScene->read(sceneBytes, sizeof(sceneBytes)) == sizeof(sceneBytes));
    REQUIRE(StringView(sceneBytes, sizeof(sceneBytes)) == "scene");
    packagedScene->close();
    REQUIRE(FileSystem::isFile(output.join(Path("MyGame.chproject"))));
    ProjectDescriptor packagedProject;
    REQUIRE(packagedProject.load(output.join(Path("MyGame.chproject"))));
    REQUIRE(packagedProject.name == "MyGame");
    REQUIRE(packagedProject.engineVersion == CH_ENGINE_VERSION_STRING);
    REQUIRE(packagedProject.gameModules == Vector<String>{"GameCode"});

    ConfigFile gameConfig;
    REQUIRE(gameConfig.load(output.join(Path("Config/Game.ini"))));
    REQUIRE(gameConfig.getValue("Game", "StartupScene"));
    REQUIRE(*gameConfig.getValue("Game", "StartupScene") == "/Game/Maps/Main");
    REQUIRE(*gameConfig.getValue("Game", "Title") == "Test");
    REQUIRE(gameConfig.getValue("Window", "Title"));
    REQUIRE(*gameConfig.getValue("Window", "Title") == "MyGame");

    // A window title the project set itself is kept.
    writeTestFile(project.join(Path("Config/Engine.ini")), "[Window]\nTitle=Mine\n");
    REQUIRE(ProjectPackager::package(desc).bSucceeded);
    ConfigFile secondConfig;
    REQUIRE(secondConfig.load(output.join(Path("Config/Game.ini"))));
    REQUIRE_FALSE(secondConfig.getValue("Window", "Title"));

    // A second package replaces the first as a whole.
    writeTestFile(output.join(Path("Stale.txt")), "old");
    REQUIRE(ProjectPackager::package(desc).bSucceeded);
    REQUIRE_FALSE(FileSystem::exists(output.join(Path("Stale.txt"))));
  }

  SECTION("Progress counts every file and a cancel removes the output")
  {
    PackageProgress progress;
    REQUIRE(ProjectPackager::package(desc, &progress).bSucceeded);
    // Executable, library, shader, asset, Engine.ini and Game.ini.
    REQUIRE(progress.totalFiles.load() == 6);
    REQUIRE(progress.copiedFiles.load() == 6);

    PackageProgress cancelled;
    cancelled.bCancelRequested.store(true);
    const PackageResult result = ProjectPackager::package(desc, &cancelled);
    REQUIRE_FALSE(result.bSucceeded);
    REQUIRE(result.bCancelled);
    REQUIRE_FALSE(FileSystem::exists(output));
  }

  SECTION("Bad requests are refused before anything is copied")
  {
    PackageDesc otherFolder = desc;
    otherFolder.outputDirectory = root.join(Path("NotAPackage"));
    writeTestFile(otherFolder.outputDirectory.join(Path("Keep.txt")), "mine");
    REQUIRE_FALSE(ProjectPackager::package(otherFolder).bSucceeded);
    REQUIRE(FileSystem::isFile(otherFolder.outputDirectory.join(Path("Keep.txt"))));

    PackageDesc noBuild = desc;
    noBuild.binaryDirectory = root.join(Path("Missing"));
    REQUIRE_FALSE(ProjectPackager::package(noBuild).bSucceeded);

    PackageDesc insideAssets = desc;
    insideAssets.outputDirectory = project.join(Path("Assets/Package"));
    REQUIRE_FALSE(ProjectPackager::package(insideAssets).bSucceeded);

    PackageDesc badName = desc;
    badName.gameName = "My:Game";
    REQUIRE_FALSE(ProjectPackager::package(badName).bSucceeded);
    REQUIRE_FALSE(FileSystem::exists(output));
  }

  SECTION("Debug builds are told apart by the engine library")
  {
    REQUIRE_FALSE(ProjectPackager::isDebugBuild(binaries));
    String debugLibrary("chCored");
    debugLibrary += DynamicLibrary::EXTENSION;
    writeTestFile(binaries.join(Path(debugLibrary)), "lib");
    REQUIRE(ProjectPackager::isDebugBuild(binaries));
  }

  SECTION("Game names")
  {
    REQUIRE(ProjectPackager::isValidGameName("My Game-2"));
    REQUIRE_FALSE(ProjectPackager::isValidGameName(""));
    REQUIRE_FALSE(ProjectPackager::isValidGameName("a/b"));
    REQUIRE_FALSE(ProjectPackager::isValidGameName("Game."));
    REQUIRE_FALSE(ProjectPackager::isValidGameName(".."));
  }

  FileSystem::removeAll(root);
}

TEST_CASE("chCore - Packaged assets follow references")
{
  const chEngineSDK::UUID scene = chEngineSDK::UUID::createRandom();
  const chEngineSDK::UUID model = chEngineSDK::UUID::createRandom();
  const chEngineSDK::UUID material = chEngineSDK::UUID::createRandom();
  const chEngineSDK::UUID texture = chEngineSDK::UUID::createRandom();
  const chEngineSDK::UUID missing = chEngineSDK::UUID::createRandom();
  const chEngineSDK::UUID unused = chEngineSDK::UUID::createRandom();
  const chEngineSDK::UUID secondScene = chEngineSDK::UUID::createRandom();
  const chEngineSDK::UUID secondModel = chEngineSDK::UUID::createRandom();

  // The material is reached twice and the texture points back at the scene.
  UnorderedMap<chEngineSDK::UUID, Vector<chEngineSDK::UUID>> references;
  references[scene] = {model, material};
  references[model] = {material, missing};
  references[material] = {texture};
  references[texture] = {scene};
  references[unused] = {};
  // A second scene shares the texture and brings its own model.
  references[secondScene] = {texture, secondModel};
  references[secondModel] = {};

  Vector<chEngineSDK::UUID> assets;
  Vector<chEngineSDK::UUID> missingAssets;
  const chEngineSDK::UUID roots[] = {scene, secondScene};
  ProjectPackager::collectAssets(
      roots,
      [&references](const chEngineSDK::UUID& uuid) -> const Vector<chEngineSDK::UUID>* {
        const auto found = references.find(uuid);
        return found != references.end() ? &found->second : nullptr;
      },
      assets, missingAssets);

  REQUIRE(assets.size() == 6);
  REQUIRE(assets[0] == scene);
  REQUIRE(Algorithm::contains(assets, secondScene));
  REQUIRE(Algorithm::contains(assets, secondModel));
  REQUIRE(Algorithm::contains(assets, model));
  REQUIRE(Algorithm::contains(assets, material));
  REQUIRE(Algorithm::contains(assets, texture));
  REQUIRE_FALSE(Algorithm::contains(assets, unused));
  REQUIRE(missingAssets.size() == 1);
  REQUIRE(missingAssets[0] == missing);
}
#endif // USING(CH_EDITOR)

namespace {

// A square made of two triangles at x, with its corners repeated for each triangle (no
// shared index), like the faces of a mesh cut along UV seams.
void
appendSquare(Vector<VertexStatic>& vertices, Vector<uint16>& indices, float x, float size)
{
  const Vector3 corners[4] = {Vector3(x, 0.0f, 0.0f), Vector3(x + size, 0.0f, 0.0f),
                              Vector3(x + size, size, 0.0f), Vector3(x, size, 0.0f)};
  const uint32 order[6] = {0, 1, 2, 0, 2, 3};
  for (const uint32 corner : order) {
    indices.push_back(static_cast<uint16>(vertices.size()));
    vertices.push_back({corners[corner], Vector3::UP, Vector2(0.0f, 0.0f),
                        Vector4(1.0f, 0.0f, 0.0f, 1.0f)});
  }
}

SPtr<Mesh>
createSquaresMesh(Span<const float> starts, float size)
{
  Vector<VertexStatic> vertices;
  Vector<uint16> indices;
  for (const float x : starts) {
    appendSquare(vertices, indices, x, size);
  }
  SPtr<Mesh> mesh = chMakeShared<Mesh>();
  mesh->setVertexData(vertices);
  mesh->setIndexData(indices);
  mesh->setMaterialSlot(3);
  return mesh;
}

} // namespace

TEST_CASE("chCore - Mesh split")
{
  SECTION("Triangles that only share positions stay one piece")
  {
    const float starts[] = {0.0f};
    const SPtr<Mesh> mesh = createSquaresMesh(starts, 1.0f);
    const Vector<SPtr<Mesh>> parts = MeshSplit::splitParts(mesh);
    REQUIRE(parts.size() == 1);
    REQUIRE(parts[0] == mesh);
  }

  SECTION("Pieces apart become meshes of their own")
  {
    const float starts[] = {0.0f, 5.0f, 10.0f};
    const Vector<SPtr<Mesh>> parts = MeshSplit::splitParts(createSquaresMesh(starts, 1.0f));
    REQUIRE(parts.size() == 3);
    for (SIZE_T i = 0; i < parts.size(); ++i) {
      const Mesh& part = *parts[i];
      REQUIRE(part.getMaterialSlot() == 3);
      REQUIRE(part.getIndexCount() == 6);
      REQUIRE(part.getVertexCount() == 6);
      REQUIRE(part.getVertexLayout().getVertexSize() == sizeof(VertexStatic));
      // In the order the triangles meet them.
      REQUIRE(part.getBounds().center.nearEqual(Vector3(starts[i] + 0.5f, 0.5f, 0.0f)));
    }
  }

  SECTION("Pieces whose boxes overlap or touch are joined")
  {
    // The second square starts where the first ends; the third overlaps the second.
    const float starts[] = {0.0f, 1.0f, 1.5f, 20.0f};
    const Vector<SPtr<Mesh>> parts = MeshSplit::splitParts(createSquaresMesh(starts, 1.0f));
    REQUIRE(parts.size() == 2);
    REQUIRE(parts[0]->getIndexCount() == 18);
    REQUIRE(parts[1]->getIndexCount() == 6);
  }

  SECTION("A mesh without positions is kept")
  {
    const SPtr<Mesh> empty = chMakeShared<Mesh>();
    const Vector<SPtr<Mesh>> parts = MeshSplit::splitParts(empty);
    REQUIRE(parts.size() == 1);
    REQUIRE(parts[0] == empty);
  }
}

TEST_CASE("chCore - Material")
{
  Material material;
  REQUIRE(material.getBaseColorFactor().r == 1.0f);
  REQUIRE(material.getMetallicFactor() == 0.0f);
  REQUIRE(material.getRoughnessFactor() == 0.5f);
  REQUIRE(material.getEmissiveFactor().r == 0.0f);
  REQUIRE(material.getAlphaMode() == MaterialAlphaMode::Opaque);
  REQUIRE(material.getAlphaCutoff() == 0.5f);
  REQUIRE_FALSE(material.isTwoSided());
  for (uint32 i = 0; i < static_cast<uint32>(MaterialTexture::COUNT); ++i) {
    REQUIRE(material.getTextureId(static_cast<MaterialTexture>(i)).isNull());
    REQUIRE(material.getGpuTexture(static_cast<MaterialTexture>(i)) == nullptr);
  }

  Vector<chEngineSDK::UUID> references;
  material.collectReferences(references);
  REQUIRE(references.empty());

  // A texture that is missing keeps its id, so saving again does not lose it.
  const chEngineSDK::UUID baseColorId = chEngineSDK::UUID::createRandom();
  const chEngineSDK::UUID normalId = chEngineSDK::UUID::createRandom();
  material.setBaseColorFactor(LinearColor(0.25f, 0.5f, 0.75f, 1.0f));
  material.setMetallicFactor(0.75f);
  material.setRoughnessFactor(0.125f);
  material.setNormalScale(2.0f);
  material.setOcclusionStrength(0.5f);
  material.setEmissiveFactor(LinearColor(4.0f, 2.0f, 1.0f, 1.0f));
  material.setAlphaMode(MaterialAlphaMode::Mask);
  material.setAlphaCutoff(0.25f);
  material.setTwoSided(true);
  material.setTexture(MaterialTexture::BaseColor, baseColorId, nullptr);
  material.setTexture(MaterialTexture::Normal, normalId, nullptr);
  material.collectReferences(references);
  REQUIRE(references.size() == 2);
  REQUIRE(references[0] == baseColorId);
  REQUIRE(references[1] == normalId);

  MemoryDataStream stream(256);
  material.serialize(stream);
  const SIZE_T writtenSize = stream.tell();
  stream.seek(0);

  Material loaded;
  REQUIRE(loaded.deserialize(stream));
  REQUIRE(loaded.getBaseColorFactor().g == 0.5f);
  REQUIRE(loaded.getBaseColorFactor().b == 0.75f);
  REQUIRE(loaded.getMetallicFactor() == 0.75f);
  REQUIRE(loaded.getRoughnessFactor() == 0.125f);
  REQUIRE(loaded.getNormalScale() == 2.0f);
  REQUIRE(loaded.getOcclusionStrength() == 0.5f);
  REQUIRE(loaded.getEmissiveFactor().r == 4.0f);
  REQUIRE(loaded.getEmissiveFactor().b == 1.0f);
  REQUIRE(loaded.getTextureId(MaterialTexture::BaseColor) == baseColorId);
  REQUIRE(loaded.getTextureId(MaterialTexture::Normal) == normalId);
  REQUIRE(loaded.getTextureId(MaterialTexture::MetallicRoughness).isNull());
  REQUIRE(loaded.getTexture(MaterialTexture::BaseColor) == nullptr);
  REQUIRE(loaded.getAlphaMode() == MaterialAlphaMode::Mask);
  REQUIRE(loaded.getAlphaCutoff() == 0.25f);
  REQUIRE(loaded.isTwoSided());

  SECTION("A newer version is rejected")
  {
    stream.seek(0);
    const uint32 otherVersion = Material::SERIALIZATION_VERSION + 1;
    stream.write(&otherVersion, sizeof(otherVersion));
    stream.seek(0);
    REQUIRE_FALSE(loaded.deserialize(stream));
  }

  SECTION("An unknown alpha mode is rejected")
  {
    const uint32 badMode = static_cast<uint32>(MaterialAlphaMode::COUNT);
    // The mode follows the cutoff at the end of the data.
    stream.seek(writtenSize - sizeof(uint32) * 2);
    stream.write(&badMode, sizeof(badMode));
    stream.seek(0);
    REQUIRE_FALSE(loaded.deserialize(stream));
  }

  SECTION("Clearing a texture clears only its id")
  {
    material.setTexture(MaterialTexture::BaseColor, nullptr);
    REQUIRE(material.getTextureId(MaterialTexture::BaseColor).isNull());
    REQUIRE(material.getTextureId(MaterialTexture::Normal) == normalId);
  }
}

TEST_CASE("chCore - Model material slots")
{
  const SPtr<Model> model = createTriangleModel();
  const SPtr<Mesh> mesh = model->getMeshes()[0];
  REQUIRE(mesh->getMaterialSlot() == 0);

  // A second mesh on another node uses slot 1.
  SPtr<Mesh> secondMesh = chMakeShared<Mesh>();
  secondMesh->setVertexData(Vector<VertexStatic>{
      {Vector3(1.0f, 0.0f, 0.0f), Vector3::UP, Vector2(0.0f, 0.0f),
       Vector4(1.0f, 0.0f, 0.0f, 1.0f)},
      {Vector3(-1.0f, 0.0f, 0.0f), Vector3::UP, Vector2(1.0f, 0.0f),
       Vector4(1.0f, 0.0f, 0.0f, 1.0f)},
      {Vector3(0.0f, 1.0f, 0.0f), Vector3::UP, Vector2(0.0f, 1.0f),
       Vector4(1.0f, 0.0f, 0.0f, 1.0f)}});
  secondMesh->setIndexData(Vector<uint16>{0, 1, 2});
  secondMesh->setMaterialSlot(1);
  addTestNode(*model, "Sword", 0, Matrix4::IDENTITY, {model->addMesh(secondMesh)});

  const SPtr<MaterialAsset> body = createTestMaterialAsset("M_Body");
  const SPtr<MaterialAsset> sword = createTestMaterialAsset("M_Sword");
  REQUIRE(model->addMaterialSlot({.name = "Body", .materialId = body->getUUID(),
                                  .material = body}) == 0);
  REQUIRE(model->addMaterialSlot({.name = "Sword", .materialId = sword->getUUID(),
                                  .material = sword}) == 1);

  Scene scene("Test", chEngineSDK::UUID::createRandom());
  const SPtr<GameObject> gameObject = scene.createGameObject("Skeleton");
  ModelComponent& modelComponent =
      gameObject->addComponent<ModelComponent>(createTestModelAsset(model));
  REQUIRE(modelComponent.getUsedMaterialSlots().size() == 2);
  REQUIRE(scene.getRenderItemCount() == 2);

  const auto materialOf = [&scene](const Mesh* itemMesh) -> const Material* {
    const Camera camera = createTestCamera();
    Vector<const RenderItem*> visible;
    scene.gatherRenderItems(camera.getFrustum(), visible);
    for (const RenderItem* item : visible) {
      if (item->mesh == itemMesh) {
        return item->material;
      }
    }
    return nullptr;
  };
  REQUIRE(materialOf(mesh.get()) == &body->getMaterial());
  REQUIRE(materialOf(secondMesh.get()) == &sword->getMaterial());

  SECTION("An override changes only its slot and this object")
  {
    const SPtr<MaterialAsset> gold = createTestMaterialAsset("M_Gold");
    modelComponent.setMaterial(1, gold);
    REQUIRE(modelComponent.getMaterialOverride(1) == gold);
    REQUIRE(modelComponent.getMaterial(1) == gold);
    REQUIRE(modelComponent.getMaterial(0) == body);
    REQUIRE(materialOf(secondMesh.get()) == &gold->getMaterial());
    REQUIRE(materialOf(mesh.get()) == &body->getMaterial());

    modelComponent.setMaterial(1, nullptr);
    REQUIRE(materialOf(secondMesh.get()) == &sword->getMaterial());
  }

  SECTION("Overrides set while unregistered apply on register")
  {
    const SPtr<MaterialAsset> gold = createTestMaterialAsset("M_Gold");
    modelComponent.setEnabled(false);
    modelComponent.setMaterial(0, gold);
    modelComponent.setEnabled(true);
    REQUIRE(materialOf(mesh.get()) == &gold->getMaterial());
  }

  SECTION("Slots past the end have no material")
  {
    REQUIRE(modelComponent.getMaterial(2) == nullptr);
    modelComponent.setMaterial(5, body);
    secondMesh->setMaterialSlot(7);
    modelComponent.setModel(modelComponent.getModelAsset());
    REQUIRE(materialOf(secondMesh.get()) == nullptr);
  }

  SECTION("Editing a material shows in the items that use it")
  {
    body->getMaterial().setBaseColorFactor(LinearColor(1.0f, 0.0f, 0.0f, 1.0f));
    REQUIRE(materialOf(mesh.get())->getBaseColorFactor().g == 0.0f);
  }
}

namespace {
Vector<uint8>
readAllBytes(MemoryDataStream& stream)
{
  Vector<uint8> bytes(stream.size());
  stream.seek(0);
  REQUIRE(stream.read(bytes.data(), bytes.size()) == bytes.size());
  stream.seek(0);
  return bytes;
}
} // namespace

TEST_CASE("chCore - Model serializer")
{
  // Root (moved and turned) > Child with both meshes; Shared is a second root that draws
  // mesh 0 again.
  const SPtr<Model> model = chMakeShared<Model>();
  const SPtr<Mesh> triangle = createTriangleModel()->getMeshes()[0];
  SPtr<Mesh> second = chMakeShared<Mesh>();
  second->setVertexData(triangle->getVertexData<VertexStatic>());
  second->setIndexData(Vector<uint32>{2, 1, 0});
  second->setMaterialSlot(1);
  // The second mesh follows Root and Child.
  MeshSkin skin;
  skin.boneNodes = {0, 1};
  skin.inverseBindMatrices = {Matrix4::IDENTITY, TranslationMatrix(Vector3(0.0f, 0.0f, -5.0f))};
  skin.vertices = {MeshSkin::packVertex(Vector<BoneInfluence>{{0, 1.0f}}),
                   MeshSkin::packVertex(Vector<BoneInfluence>{{0, 0.5f}, {1, 0.5f}}),
                   MeshSkin::packVertex(Vector<BoneInfluence>{{1, 1.0f}})};
  second->setSkin(skin);
  const uint32 mesh0 = model->addMesh(triangle);
  const uint32 mesh1 = model->addMesh(second);
  const Quaternion turn(Vector3::UP, Degree(90.0f));
  const uint32 root = model->addNode("Root", ModelNode::NO_PARENT, Vector3(1.0f, 2.0f, 3.0f),
                                     turn, Vector3(2.0f, 2.0f, 2.0f), {});
  addTestNode(*model, "Child", root, TranslationMatrix(Vector3(0.0f, 0.0f, 5.0f)),
              {mesh0, mesh1});
  addTestNode(*model, "Shared", ModelNode::NO_PARENT, Matrix4::IDENTITY, {mesh0});
  const chEngineSDK::UUID bodyId = chEngineSDK::UUID::createRandom();
  model->addMaterialSlot({.name = "Body", .materialId = bodyId, .material = nullptr});
  model->addMaterialSlot({.name = "Empty", .materialId = chEngineSDK::UUID::null(),
                          .material = nullptr});
  const chEngineSDK::UUID skeletonId = chEngineSDK::UUID::createRandom();
  model->setSkeleton(skeletonId, nullptr);
  model->setNodeBone(0, 0);
  model->setNodeBone(1, 1);

  REQUIRE(model->getNodeRestMatrix(1).nearEqual(
      TranslationMatrix(Vector3(0.0f, 0.0f, 5.0f)) * model->getNodeRestMatrix(0)));

  MemoryDataStream stream;
  ModelSerializer::write(stream, *model);
  const Vector<uint8> bytes = readAllBytes(stream);

  SECTION("Everything comes back in the same order")
  {
    const SPtr<Model> read = ModelSerializer::read(stream);
    REQUIRE(read);
    REQUIRE(read->getNodeCount() == 3);
    REQUIRE(read->getMeshCount() == 2);
    for (uint32 i = 0; i < 3; ++i) {
      const ModelNode& expected = model->getNodes()[i];
      const ModelNode& actual = read->getNodes()[i];
      REQUIRE(actual.name == expected.name);
      REQUIRE(actual.parent == expected.parent);
      REQUIRE(actual.position.nearEqual(expected.position));
      REQUIRE(actual.rotation.isRotationEqual(expected.rotation));
      REQUIRE(actual.scale.nearEqual(expected.scale));
      REQUIRE(read->getNodeRestMatrix(i).nearEqual(model->getNodeRestMatrix(i)));
      const Span<const uint32> expectedMeshes = model->getNodeMeshes(i);
      const Span<const uint32> actualMeshes = read->getNodeMeshes(i);
      REQUIRE(actualMeshes.size() == expectedMeshes.size());
      for (SIZE_T j = 0; j < actualMeshes.size(); ++j) {
        REQUIRE(actualMeshes[j] == expectedMeshes[j]);
      }
    }
    for (uint32 i = 0; i < 2; ++i) {
      const Mesh& expected = *model->getMeshes()[i];
      const Mesh& actual = *read->getMeshes()[i];
      REQUIRE(actual.getVertexData() == expected.getVertexData());
      REQUIRE(actual.getIndexData() == expected.getIndexData());
      REQUIRE(actual.getIndexType() == expected.getIndexType());
      REQUIRE(actual.getIndexCount() == expected.getIndexCount());
      REQUIRE(actual.getMaterialSlot() == expected.getMaterialSlot());
      REQUIRE(actual.getVertexLayout().getVertexSize() == sizeof(VertexStatic));
    }
    REQUIRE(read->getMaterialSlots().size() == 2);
    REQUIRE(read->getMaterialSlots()[0].name == "Body");
    REQUIRE(read->getMaterialSlots()[0].materialId == bodyId);
    REQUIRE(read->getMaterialSlots()[1].materialId.isNull());
    REQUIRE(read->getSkeletonId() == skeletonId);
    REQUIRE(read->getNodes()[0].bone == 0);
    REQUIRE(read->getNodes()[1].bone == 1);
    REQUIRE(read->getNodes()[2].bone == ModelNode::NO_BONE);
    REQUIRE_FALSE(read->getMeshes()[0]->hasSkin());
    const MeshSkin& readSkin = read->getMeshes()[1]->getSkin();
    REQUIRE(readSkin.boneNodes == skin.boneNodes);
    REQUIRE(readSkin.inverseBindMatrices[1].nearEqual(skin.inverseBindMatrices[1]));
    REQUIRE(readSkin.vertices.size() == 3);
    REQUIRE(readSkin.vertices[1].bones[1] == skin.vertices[1].bones[1]);
    REQUIRE(readSkin.vertices[1].weights[1] == skin.vertices[1].weights[1]);

    // Saving what was read gives the same bytes.
    MemoryDataStream again;
    ModelSerializer::write(again, *read);
    REQUIRE(readAllBytes(again) == bytes);
  }

  SECTION("A cut file is rejected")
  {
    for (const SIZE_T size : {SIZE_T(0), SIZE_T(10), bytes.size() / 2, bytes.size() - 1}) {
      MemoryDataStream cut;
      cut.write(bytes.data(), size);
      cut.seek(0);
      REQUIRE_FALSE(ModelSerializer::read(cut));
    }
  }

  SECTION("A skin bone past the nodes is rejected")
  {
    MeshSkin broken = skin;
    broken.boneNodes[1] = 7;
    second->setSkin(broken);
    MemoryDataStream other;
    ModelSerializer::write(other, *model);
    other.seek(0);
    REQUIRE_FALSE(ModelSerializer::read(other));
  }

  SECTION("A newer version is rejected")
  {
    Vector<uint8> newer = bytes;
    const uint32 version = ModelHeader::VERSION + 1;
    std::memcpy(newer.data(), &version, sizeof(version));
    MemoryDataStream other;
    other.write(newer.data(), newer.size());
    other.seek(0);
    REQUIRE_FALSE(ModelSerializer::read(other));
  }
}

TEST_CASE("chCore - Mesh skin weights")
{
  SECTION("Weights are scaled to 255 and sorted, strongest first")
  {
    const VertexSkin vertex =
        MeshSkin::packVertex(Vector<BoneInfluence>{{3, 0.25f}, {7, 0.75f}});
    REQUIRE(vertex.bones[0] == 7);
    REQUIRE(vertex.bones[1] == 3);
    REQUIRE(vertex.weights[0] == 191);
    REQUIRE(vertex.weights[1] == 64);
    REQUIRE(vertex.weights[2] == 0);
    REQUIRE(vertex.weights[3] == 0);
  }

  SECTION("Only the four strongest stay, and the sum is still 255")
  {
    const VertexSkin vertex = MeshSkin::packVertex(Vector<BoneInfluence>{
        {0, 0.1f}, {1, 0.3f}, {2, 0.05f}, {3, 0.2f}, {4, 0.25f}, {5, 0.1f}});
    REQUIRE(vertex.bones[0] == 1);
    REQUIRE(vertex.bones[1] == 4);
    REQUIRE(vertex.bones[2] == 3);
    REQUIRE((vertex.bones[3] == 0 || vertex.bones[3] == 5));
    uint32 sum = 0;
    for (const uint8 weight : vertex.weights) {
      sum += weight;
    }
    REQUIRE(sum == 255);
  }

  SECTION("Three equal thirds round to 255")
  {
    const VertexSkin vertex =
        MeshSkin::packVertex(Vector<BoneInfluence>{{0, 1.0f}, {1, 1.0f}, {2, 1.0f}});
    REQUIRE(vertex.weights[0] + vertex.weights[1] + vertex.weights[2] == 255);
  }

  SECTION("No weight, zero or NaN gives the first bone everything")
  {
    const float notANumber = std::numeric_limits<float>::quiet_NaN();
    for (const VertexSkin& vertex : {MeshSkin::packVertex({}),
                                     MeshSkin::packVertex(Vector<BoneInfluence>{{4, 0.0f}}),
                                     MeshSkin::packVertex(
                                         Vector<BoneInfluence>{{4, notANumber}})}) {
      REQUIRE(vertex.bones[0] == 0);
      REQUIRE(vertex.weights[0] == 255);
      REQUIRE(vertex.weights[1] == 0);
    }
  }
}

TEST_CASE("chCore - Model skeleton from joints")
{
  // Scene > Armature > Hips > Spine > Head, Hips > Leg, Scene > Prop, and a second root.
  Model model;
  const auto add = [&model](StringView name, uint32 parent) {
    return model.addNode(name, parent, Vector3::ZERO, Quaternion::IDENTITY, Vector3::UNIT,
                         Span<const uint32>());
  };
  const uint32 scene = add("Scene", ModelNode::NO_PARENT);
  const uint32 armature = add("Armature", scene);
  const uint32 hips = add("Hips", armature);
  const uint32 spine = add("Spine", hips);
  const uint32 head = add("Head", spine);
  const uint32 leg = add("Leg", hips);
  const uint32 prop = add("Prop", scene);
  const uint32 other = add("Other", ModelNode::NO_PARENT);
  const Vector<ModelNode>& nodes = model.getNodes();

  SECTION("Nodes above the lowest common one stay plain nodes")
  {
    const uint32 joints[] = {spine, leg};
    const SPtr<Skeleton> skeleton = model.buildSkeleton(joints);
    REQUIRE(skeleton);
    REQUIRE(skeleton->getBoneCount() == 3);
    REQUIRE(nodes[scene].bone == ModelNode::NO_BONE);
    REQUIRE(nodes[armature].bone == ModelNode::NO_BONE);
    REQUIRE(nodes[head].bone == ModelNode::NO_BONE);
    REQUIRE(skeleton->getBones()[nodes[hips].bone].name == "Hips");
    REQUIRE(skeleton->getBones()[nodes[hips].bone].parent == SkeletonBone::NO_PARENT);
    REQUIRE(skeleton->getBones()[nodes[spine].bone].parent == nodes[hips].bone);
    REQUIRE(skeleton->getBones()[nodes[leg].bone].parent == nodes[hips].bone);

    // Built again, the nodes that are no longer bones lose their bone.
    const uint32 headOnly[] = {head};
    const SPtr<Skeleton> single = model.buildSkeleton(headOnly);
    REQUIRE(single->getBoneCount() == 1);
    REQUIRE(nodes[head].bone == 0);
    REQUIRE(nodes[hips].bone == ModelNode::NO_BONE);
  }

  SECTION("Joints in other branches reach their common node")
  {
    const uint32 joints[] = {head, prop};
    const SPtr<Skeleton> skeleton = model.buildSkeleton(joints);
    REQUIRE(skeleton->getBoneCount() == 6);
    REQUIRE(nodes[scene].bone == 0);
    REQUIRE(nodes[leg].bone == ModelNode::NO_BONE);
    REQUIRE(skeleton->getBones()[nodes[prop].bone].parent == nodes[scene].bone);
  }

  SECTION("Joints under different roots make every node above them a bone")
  {
    const uint32 joints[] = {head, other};
    const SPtr<Skeleton> skeleton = model.buildSkeleton(joints);
    REQUIRE(skeleton->getBoneCount() == 6);
    REQUIRE(skeleton->getBones()[nodes[scene].bone].parent == SkeletonBone::NO_PARENT);
    REQUIRE(skeleton->getBones()[nodes[other].bone].parent == SkeletonBone::NO_PARENT);
  }

  SECTION("No joints, no skeleton")
  {
    REQUIRE_FALSE(model.buildSkeleton(Span<const uint32>()));
    const uint32 outside[] = {99};
    REQUIRE_FALSE(model.buildSkeleton(outside));
  }
}

TEST_CASE("chCore - Skeleton")
{
  // Hips > Spine > Head, and Hips > Leg.
  Skeleton skeleton;
  const uint32 hips = skeleton.addBone({.name = "Hips",
                                        .parent = SkeletonBone::NO_PARENT,
                                        .position = Vector3(0.0f, 0.0f, 1.0f),
                                        .rotation = Quaternion::IDENTITY,
                                        .scale = Vector3::UNIT});
  const uint32 spine = skeleton.addBone({.name = "Spine", .parent = hips});
  skeleton.addBone({.name = "Head", .parent = spine});
  skeleton.addBone({.name = "Leg",
                    .parent = hips,
                    .position = Vector3::ZERO,
                    .rotation = Quaternion(Vector3::RIGHT, Degree(30.0f)),
                    .scale = Vector3::UNIT});
  REQUIRE(skeleton.getBoneCount() == 4);
  REQUIRE(skeleton.findBone("Head") == 2);
  REQUIRE(skeleton.findBone("Tail") == Skeleton::NO_BONE);

  Vector<uint32> boneMap;
  SECTION("A skeleton fits itself")
  {
    REQUIRE(skeleton.mapBones(skeleton, boneMap));
    REQUIRE(boneMap == Vector<uint32>{0, 1, 2, 3});
  }

  SECTION("Fewer bones in another order fit")
  {
    Skeleton part;
    const uint32 partHips = part.addBone({.name = "Hips"});
    part.addBone({.name = "Leg", .parent = partHips});
    part.addBone({.name = "Spine", .parent = partHips});
    REQUIRE(skeleton.mapBones(part, boneMap));
    REQUIRE(boneMap == Vector<uint32>{0, 3, 1});
  }

  SECTION("A root may hang from a bone the other skeleton lacks")
  {
    Skeleton head;
    head.addBone({.name = "Head"});
    REQUIRE(skeleton.mapBones(head, boneMap));
    REQUIRE(boneMap == Vector<uint32>{2});
  }

  SECTION("A missing bone or another parent does not fit")
  {
    Skeleton other;
    other.addBone({.name = "Hips"});
    other.addBone({.name = "Tail", .parent = 0});
    REQUIRE_FALSE(skeleton.mapBones(other, boneMap));

    Skeleton moved;
    moved.addBone({.name = "Spine"});
    moved.addBone({.name = "Leg", .parent = 0});
    REQUIRE_FALSE(skeleton.mapBones(moved, boneMap));
  }

  SECTION("Save and read")
  {
    MemoryDataStream stream;
    skeleton.serialize(stream);
    const Vector<uint8> bytes = readAllBytes(stream);
    const SPtr<Skeleton> read = Skeleton::deserialize(stream);
    REQUIRE(read);
    REQUIRE(read->getBoneCount() == 4);
    for (uint32 i = 0; i < 4; ++i) {
      const SkeletonBone& expected = skeleton.getBones()[i];
      const SkeletonBone& actual = read->getBones()[i];
      REQUIRE(actual.name == expected.name);
      REQUIRE(actual.parent == expected.parent);
      REQUIRE(actual.position.nearEqual(expected.position));
      REQUIRE(actual.rotation.isRotationEqual(expected.rotation));
      REQUIRE(actual.scale.nearEqual(expected.scale));
    }

    for (const SIZE_T size : {SIZE_T(0), SIZE_T(6), bytes.size() - 1}) {
      MemoryDataStream cut;
      cut.write(bytes.data(), size);
      cut.seek(0);
      REQUIRE_FALSE(Skeleton::deserialize(cut));
    }
  }
}

namespace {
// Three keys, one per second: "Arm" moves along x and turns 90 degrees around Z between the
// first two keys; "Base" never changes.
SPtr<AnimationClip>
createTestClip()
{
  SPtr<AnimationClip> clip = chMakeShared<AnimationClip>(1.0f, 3);
  const Vector<Vector3> armPositions{Vector3(0.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f),
                                     Vector3(4.0f, 0.0f, 0.0f)};
  const Quaternion turned(Vector3::UP, Degree(90.0f));
  const Vector<Quaternion> armRotations{Quaternion::IDENTITY, turned, turned};
  const Vector<Vector3> units(3, Vector3::UNIT);
  static_cast<void>(clip->addTrack("Arm", armPositions, armRotations, units));
  const Vector<Vector3> basePositions(3, Vector3(0.0f, 0.0f, 1.0f));
  const Vector<Quaternion> baseRotations(3, Quaternion::IDENTITY);
  static_cast<void>(clip->addTrack("Base", basePositions, baseRotations, units));
  return clip;
}

SPtr<AnimationClipAsset>
createTestClipAsset(const SPtr<AnimationClip>& clip)
{
  AssetMetadata metadata;
  metadata.uuid = chEngineSDK::UUID::createRandom();
  metadata.assetType = AssetTypeTraits<AnimationClipAsset>::getTypeId();
  StringUtils::copyToBuffer(metadata.name, "A_Test");
  return chMakeShared<AnimationClipAsset>(metadata, clip);
}
} // namespace

TEST_CASE("chCore - Animation clip")
{
  const SPtr<AnimationClip> clip = createTestClip();
  REQUIRE(clip->getDuration() == 2.0f);
  REQUIRE(clip->getTracks().size() == 2);
  // Channels that never change keep one key.
  const AnimationTrack& arm = clip->getTracks()[0];
  const AnimationTrack& base = clip->getTracks()[1];
  REQUIRE(arm.positionCount == 3);
  REQUIRE(arm.rotationCount == 3);
  REQUIRE(arm.scaleCount == 1);
  REQUIRE(base.positionCount == 1);
  REQUIRE(base.rotationCount == 1);

  Vector<NodeTransform> pose(3);
  const Vector<uint32> targets{2, 0};

  SECTION("Keys are blended at times between them")
  {
    clip->sample(0.5f, targets, pose);
    REQUIRE(pose[2].position.nearEqual(Vector3(1.0f, 0.0f, 0.0f)));
    REQUIRE(pose[2].rotation.isRotationEqual(Quaternion(Vector3::UP, Degree(45.0f)), 1e-6f));
    REQUIRE(pose[0].position.nearEqual(Vector3(0.0f, 0.0f, 1.0f)));
    // A node no track moves is left alone.
    REQUIRE(pose[1].position == Vector3::ZERO);
  }

  SECTION("Times outside the clip hold its ends")
  {
    clip->sample(-1.0f, targets, pose);
    REQUIRE(pose[2].position.nearEqual(Vector3::ZERO));
    clip->sample(10.0f, targets, pose);
    REQUIRE(pose[2].position.nearEqual(Vector3(4.0f, 0.0f, 0.0f)));
  }

  SECTION("Tracks without a target are skipped")
  {
    const Vector<uint32> onlyBase{AnimationClip::NO_TARGET, 1};
    clip->sample(1.0f, onlyBase, pose);
    REQUIRE(pose[2].position == Vector3::ZERO);
    REQUIRE(pose[1].position.nearEqual(Vector3(0.0f, 0.0f, 1.0f)));
  }

  SECTION("Rotation keys stay on one side, so blends take the short way")
  {
    AnimationClip flipped(1.0f, 2);
    const Quaternion turned(Vector3::UP, Degree(20.0f));
    const Vector<Vector3> positions(2, Vector3::ZERO);
    const Vector<Vector3> scales(2, Vector3::UNIT);
    const Vector<Quaternion> rotations{Quaternion::IDENTITY, turned * -1.0f};
    static_cast<void>(flipped.addTrack("Node", positions, rotations, scales));
    const Vector<uint32> target{0};
    flipped.sample(0.5f, target, pose);
    REQUIRE(pose[0].rotation.isRotationEqual(Quaternion(Vector3::UP, Degree(10.0f)), 1e-6f));
  }

  SECTION("Save and read")
  {
    clip->setSkeletonId(chEngineSDK::UUID::createRandom());
    MemoryDataStream stream;
    clip->serialize(stream);
    const Vector<uint8> bytes = readAllBytes(stream);
    const SPtr<AnimationClip> read = AnimationClip::deserialize(stream);
    REQUIRE(read);
    REQUIRE(read->getSkeletonId() == clip->getSkeletonId());
    REQUIRE(read->getFrameCount() == 3);
    REQUIRE(read->getSampleRate() == 1.0f);
    REQUIRE(read->getTracks()[1].nodeName == "Base");
    read->sample(0.5f, targets, pose);
    REQUIRE(pose[2].position.nearEqual(Vector3(1.0f, 0.0f, 0.0f)));

    for (const SIZE_T size : {SIZE_T(0), SIZE_T(10), bytes.size() - 1}) {
      MemoryDataStream cut;
      cut.write(bytes.data(), size);
      cut.seek(0);
      REQUIRE_FALSE(AnimationClip::deserialize(cut));
    }
  }
}

TEST_CASE("chCore - Model animation")
{
  // Base (one mesh) > Arm (one mesh, at x + 1); the clip moves Arm and holds Base at z + 1.
  const SPtr<Model> model = chMakeShared<Model>();
  const uint32 baseNode = addTestNode(*model, "Base", ModelNode::NO_PARENT, Matrix4::IDENTITY,
                                      {model->addMesh(createTriangleModel()->getMeshes()[0])});
  const uint32 armNode =
      addTestNode(*model, "Arm", baseNode, TranslationMatrix(Vector3(1.0f, 0.0f, 0.0f)),
                  {model->addMesh(createTriangleModel()->getMeshes()[0])});

  Scene scene("Test", chEngineSDK::UUID::createRandom());
  const SPtr<GameObject> gameObject = scene.createGameObject("Animated");
  ModelComponent& component =
      gameObject->addComponent<ModelComponent>(createTestModelAsset(model));
  scene.updateTransforms();
  REQUIRE_FALSE(component.isAnimating());
  REQUIRE(component.getNodeMatrix(armNode) == model->getNodeRestMatrix(armNode));

  const auto armItemCenter = [&scene]() {
    // The arm's item is the second one the component added.
    return scene.getRenderItemSlots()[1].worldBounds.center;
  };
  REQUIRE(armItemCenter().nearEqual(Vector3(1.0f, 1.0f, 0.0f)));

  const SPtr<AnimationClipAsset> clipAsset = createTestClipAsset(createTestClip());
  component.setAnimation(clipAsset);
  REQUIRE(component.isAnimating());
  REQUIRE(component.isPlaying());
  // At time 0 the arm stands at the clip's first key, under the base lifted by the clip.
  REQUIRE(armItemCenter().nearEqual(Vector3(0.0f, 1.0f, 1.0f)));

  SECTION("Update moves the nodes and their render items")
  {
    scene.update(1.0f);
    REQUIRE(component.getAnimationTime() == 1.0f);
    // Arm at x + 2, turned 90 degrees: the triangle's (0, 1, 0) center goes to (-1, 0, 0).
    REQUIRE(armItemCenter().nearEqual(Vector3(1.0f, 0.0f, 1.0f), 1e-5f));
    gameObject->getTransform().setLocalPosition(Vector3(0.0f, 10.0f, 0.0f));
    scene.updateTransforms();
    REQUIRE(armItemCenter().nearEqual(Vector3(1.0f, 10.0f, 1.0f), 1e-5f));
  }

  SECTION("Looping wraps the time; without it the clip stops at its end")
  {
    scene.update(2.5f);
    REQUIRE(Math::abs(component.getAnimationTime() - 0.5f) < 1e-5f);
    component.setLooping(false);
    scene.update(5.0f);
    REQUIRE(component.getAnimationTime() == 2.0f);
    REQUIRE_FALSE(component.isPlaying());
    component.setPlaying(true);
    REQUIRE(component.getAnimationTime() == 0.0f);
  }

  SECTION("Paused clips only follow the time they are given")
  {
    component.setPlaying(false);
    scene.update(1.0f);
    REQUIRE(component.getAnimationTime() == 0.0f);
    component.setAnimationTime(1.0f);
    scene.update(0.0f);
    REQUIRE(armItemCenter().nearEqual(Vector3(1.0f, 0.0f, 1.0f), 1e-5f));
  }

  SECTION("Clearing the clip goes back to the rest pose")
  {
    scene.update(1.0f);
    component.setAnimation(nullptr);
    REQUIRE_FALSE(component.isAnimating());
    REQUIRE(armItemCenter().nearEqual(Vector3(1.0f, 1.0f, 0.0f)));
  }

  SECTION("A clip whose tracks name no node moves nothing")
  {
    SPtr<AnimationClip> other = chMakeShared<AnimationClip>(1.0f, 1);
    static_cast<void>(other->addTrack("Missing", Vector<Vector3>{Vector3::UNIT},
                                      Vector<Quaternion>{Quaternion::IDENTITY},
                                      Vector<Vector3>{Vector3::UNIT}));
    component.setAnimation(createTestClipAsset(other));
    REQUIRE_FALSE(component.isAnimating());
    REQUIRE(armItemCenter().nearEqual(Vector3(1.0f, 1.0f, 0.0f)));
  }

  SECTION("The clip and how it plays are saved")
  {
    component.setLooping(false);
    component.setPlaybackSpeed(-0.5f);
    MemoryDataStream stream;
    component.serialize(stream);
    stream.seek(0);
    // Without an AssetManager the clip is not found, but its id is kept.
    ModelComponent read;
    REQUIRE(read.deserialize(stream));
    REQUIRE_FALSE(read.isLooping());
    REQUIRE(read.isPlaying());
    REQUIRE(read.getPlaybackSpeed() == -0.5f);
    Vector<chEngineSDK::UUID> references;
    read.collectReferences(references);
    REQUIRE(Algorithm::contains(references, clipAsset->getUUID()));
  }
}

TEST_CASE("chCore - Skinned model")
{
  // Base > Arm (at x + 1), named like the test clip's tracks. One triangle on Base, skinned:
  // its first two vertices follow Base, the third follows Arm.
  const SPtr<Model> model = chMakeShared<Model>();
  const SPtr<Mesh> mesh = createTriangleModel()->getMeshes()[0];
  const uint32 baseNode = addTestNode(*model, "Base", ModelNode::NO_PARENT, Matrix4::IDENTITY,
                                      {model->addMesh(mesh)});
  const uint32 armNode =
      addTestNode(*model, "Arm", baseNode, TranslationMatrix(Vector3(1.0f, 0.0f, 0.0f)), {});
  MeshSkin skin;
  skin.boneNodes = {baseNode, armNode};
  skin.inverseBindMatrices = {Matrix4::IDENTITY,
                              model->getNodeRestMatrix(armNode).getInverseAffine()};
  const VertexSkin onBase{.bones = {0, 0, 0, 0}, .weights = {255, 0, 0, 0}};
  const VertexSkin onArm{.bones = {1, 0, 0, 0}, .weights = {255, 0, 0, 0}};
  skin.vertices = {onBase, onBase, onArm};
  mesh->setSkin(std::move(skin));

  SECTION("Each bone gets the box of its vertices in its own space")
  {
    const Vector<SkinBoneBounds>& boneBounds = mesh->getSkinBoneBounds();
    REQUIRE(boneBounds.size() == 2);
    REQUIRE(boneBounds[0].bone == 0);
    REQUIRE(boneBounds[0].box.minPoint.nearEqual(Vector3(-1.0f, 0.0f, 0.0f)));
    REQUIRE(boneBounds[0].box.maxPoint.nearEqual(Vector3(1.0f, 0.0f, 0.0f)));
    REQUIRE(boneBounds[1].box.minPoint.nearEqual(Vector3(-1.0f, 2.0f, 0.0f)));
  }

  Scene scene("Test", chEngineSDK::UUID::createRandom());
  const SPtr<GameObject> gameObject = scene.createGameObject("Skinned");
  ModelComponent& component =
      gameObject->addComponent<ModelComponent>(createTestModelAsset(model));
  scene.updateTransforms();
  const RenderItem& item = scene.getRenderItemSlots()[0];
  REQUIRE(item.skinMatrices);

  SECTION("At rest the skin leaves the vertices where they are")
  {
    REQUIRE(item.skinMatrices[0] == Matrix4::IDENTITY);
    REQUIRE(item.skinMatrices[1].nearEqual(Matrix4::IDENTITY));
    REQUIRE(item.worldMatrix == Matrix4::IDENTITY);
  }

  // At time 0 the clip lifts Base by z + 1 and puts Arm at Base's origin, so the third
  // vertex moves from (0, 2, 0) to (-1, 2, 1).
  component.setAnimation(createTestClipAsset(createTestClip()));
  const uint64 posedRevision = item.skinRevision;
  REQUIRE(item.skinMatrices[0].nearEqual(TranslationMatrix(Vector3(0.0f, 0.0f, 1.0f))));
  REQUIRE(item.skinMatrices[1].nearEqual(TranslationMatrix(Vector3(-1.0f, 0.0f, 1.0f))));

  SECTION("Bounds are the posed bone boxes")
  {
    const AABox box = item.worldBounds.getBox();
    REQUIRE(box.minPoint.nearEqual(Vector3(-1.0f, 0.0f, 1.0f), 1e-5f));
    REQUIRE(box.maxPoint.nearEqual(Vector3(1.0f, 2.0f, 1.0f), 1e-5f));
  }

  SECTION("Moving the object moves the item without skinning again")
  {
    gameObject->getTransform().setLocalPosition(Vector3(0.0f, 10.0f, 0.0f));
    scene.updateTransforms();
    REQUIRE(item.skinRevision == posedRevision);
    REQUIRE(item.worldMatrix.nearEqual(TranslationMatrix(Vector3(0.0f, 10.0f, 0.0f))));
    REQUIRE(item.worldBounds.getBox().minPoint.nearEqual(Vector3(-1.0f, 10.0f, 1.0f), 1e-5f));
  }

  SECTION("A new pose gives a new revision")
  {
    scene.update(1.0f);
    REQUIRE(item.skinRevision != posedRevision);
    component.setPlaying(false);
    const uint64 pausedRevision = item.skinRevision;
    scene.update(1.0f);
    REQUIRE(item.skinRevision == pausedRevision);
  }

  SECTION("Picking hits the posed triangle")
  {
    // Inside the posed triangle, outside the one at rest.
    const Ray ray(Vector3(-0.8f, 1.5f, 5.0f), Vector3(0.0f, 0.0f, -1.0f));
    SceneRayHit hit;
    REQUIRE(scene.raycast(ray, hit));
    REQUIRE(Math::abs(hit.distance - 4.0f) < 1e-4f);
    float distance = 0.0f;
    REQUIRE_FALSE(mesh->raycast(ray, distance));
  }

  SECTION("The frame copies the skin matrices")
  {
    RenderFrame frame;
    frame.extract(scene, createTestCamera(), LinearColor::BLACK);
    const RenderItem& copy = frame.getItems()[0];
    REQUIRE(copy.skinMatrices != item.skinMatrices);
    REQUIRE(copy.skinMatrices[1] == item.skinMatrices[1]);
    REQUIRE(copy.sceneSlot == 0);
    REQUIRE(copy.skinnedVertices == nullptr);
  }

  SECTION("A part of a model is never skinned")
  {
    component.setModelPart(createTestModelAsset(model), baseNode, ModelComponent::ALL,
                           Vector3::ZERO);
    for (const RenderItem& slot : scene.getRenderItemSlots()) {
      REQUIRE((!slot.mesh || !slot.skinMatrices));
    }
  }
}

TEST_CASE("chCore - Model expand")
{
  // Root (x + 10, one mesh) > Child (y + 5, two meshes) and Root > Empty > Leaf (z + 3).
  // Every mesh is the test triangle, centered at (0, 1, 0) in its node.
  const auto createMesh = [](uint32 slot) {
    SPtr<Mesh> mesh = createTriangleModel()->getMeshes()[0];
    mesh->setMaterialSlot(slot);
    return mesh;
  };
  const SPtr<Model> model = chMakeShared<Model>();
  const uint32 rootNode =
      addTestNode(*model, "Root", ModelNode::NO_PARENT,
                  TranslationMatrix(Vector3(10.0f, 0.0f, 0.0f)), {model->addMesh(createMesh(0))});
  const uint32 childMesh0 = model->addMesh(createMesh(0));
  const uint32 childMesh1 = model->addMesh(createMesh(1));
  addTestNode(*model, "Child", rootNode, TranslationMatrix(Vector3(0.0f, 5.0f, 0.0f)),
              {childMesh0, childMesh1});
  const uint32 emptyNode = addTestNode(*model, "Empty", rootNode, Matrix4::IDENTITY, {});
  addTestNode(*model, "Leaf", emptyNode, TranslationMatrix(Vector3(0.0f, 0.0f, 3.0f)),
              {model->addMesh(createMesh(1))});

  const SPtr<MaterialAsset> body = createTestMaterialAsset("M_Body");
  const SPtr<MaterialAsset> sword = createTestMaterialAsset("M_Sword");
  model->addMaterialSlot({.name = "Body", .materialId = body->getUUID(), .material = body});
  model->addMaterialSlot({.name = "Sword", .materialId = sword->getUUID(), .material = sword});

  Scene scene("Test", chEngineSDK::UUID::createRandom());
  const SPtr<GameObject> gameObject = scene.createGameObject("Model");
  gameObject->getTransform().setLocalPosition(Vector3(0.0f, 0.0f, 1.0f));
  ModelComponent& whole =
      gameObject->addComponent<ModelComponent>(createTestModelAsset(model));
  REQUIRE(whole.isWholeModel());
  const SPtr<MaterialAsset> gold = createTestMaterialAsset("M_Gold");
  whole.setMaterial(1, gold);
  scene.updateTransforms();
  REQUIRE(scene.getRenderItemCount() == 4);

  AABox wholeBounds(Vector3::ZERO, Vector3::ZERO);
  REQUIRE(gameObject->getHierarchyWorldBounds(wholeBounds));

  const auto findChild = [](const GameObject& parent, const String& name) -> GameObject* {
    for (const SPtr<GameObject>& child : parent.getChildren()) {
      if (child->getName() == name) {
        return child.get();
      }
    }
    return nullptr;
  };
  const auto partCenter = [](const GameObject& part) {
    AABox bounds(Vector3::ZERO, Vector3::ZERO);
    REQUIRE(part.getComponent<ModelComponent>()->getWorldBounds(bounds));
    return bounds.getCenter();
  };

  REQUIRE_FALSE(ModelExpand::expand(scene, *gameObject, ModelExpandMode::Single));

  SECTION("Per node keeps the hierarchy and where every mesh is drawn")
  {
    REQUIRE(ModelExpand::expand(scene, *gameObject, ModelExpandMode::PerNode));
    REQUIRE(gameObject->getComponent<ModelComponent>() == nullptr);
    REQUIRE(scene.getRenderItemCount() == 4);
    scene.updateTransforms();

    REQUIRE(gameObject->getChildren().size() == 1);
    GameObject* root = findChild(*gameObject, "Root");
    REQUIRE(root != nullptr);
    GameObject* child = findChild(*root, "Child");
    GameObject* empty = findChild(*root, "Empty");
    REQUIRE(child != nullptr);
    REQUIRE(empty != nullptr);
    REQUIRE(empty->getComponent<ModelComponent>() == nullptr);
    GameObject* leaf = findChild(*empty, "Leaf");
    REQUIRE(leaf != nullptr);

    REQUIRE(partCenter(*root).nearEqual(Vector3(10.0f, 1.0f, 1.0f)));
    REQUIRE(partCenter(*child).nearEqual(Vector3(10.0f, 6.0f, 1.0f)));
    REQUIRE(partCenter(*leaf).nearEqual(Vector3(10.0f, 1.0f, 4.0f)));

    const ModelComponent& childPart = *child->getComponent<ModelComponent>();
    REQUIRE(childPart.getUsedMaterialSlots().size() == 2);
    REQUIRE(childPart.getMaterial(0) == body);
    REQUIRE(childPart.getMaterial(1) == gold);
    REQUIRE(root->getComponent<ModelComponent>()->getUsedMaterialSlots().size() == 1);

    AABox expandedBounds(Vector3::ZERO, Vector3::ZERO);
    REQUIRE(gameObject->getHierarchyWorldBounds(expandedBounds));
    REQUIRE(expandedBounds.minPoint.nearEqual(wholeBounds.minPoint));
    REQUIRE(expandedBounds.maxPoint.nearEqual(wholeBounds.maxPoint));

    // Already split: nothing left to expand.
    REQUIRE_FALSE(ModelExpand::expand(scene, *gameObject, ModelExpandMode::PerMesh));
  }

  SECTION("Per mesh puts every mesh in its own child, standing at its center")
  {
    REQUIRE(ModelExpand::expand(scene, *gameObject, ModelExpandMode::PerMesh));
    scene.updateTransforms();
    REQUIRE(gameObject->getChildren().size() == 4);
    REQUIRE(scene.getRenderItemCount() == 4);

    GameObject* root = findChild(*gameObject, "Root");
    GameObject* childBody = findChild(*gameObject, "Child - Body");
    GameObject* childSword = findChild(*gameObject, "Child - Sword");
    GameObject* leaf = findChild(*gameObject, "Leaf");
    REQUIRE(root != nullptr);
    REQUIRE(childBody != nullptr);
    REQUIRE(childSword != nullptr);
    REQUIRE(leaf != nullptr);

    REQUIRE(root->getTransform().getWorldPosition().nearEqual(Vector3(10.0f, 1.0f, 1.0f)));
    REQUIRE(partCenter(*root).nearEqual(Vector3(10.0f, 1.0f, 1.0f)));
    REQUIRE(childSword->getTransform().getWorldPosition().nearEqual(
        Vector3(10.0f, 6.0f, 1.0f)));
    REQUIRE(partCenter(*childSword).nearEqual(Vector3(10.0f, 6.0f, 1.0f)));
    REQUIRE(partCenter(*leaf).nearEqual(Vector3(10.0f, 1.0f, 4.0f)));
    REQUIRE(childSword->getComponent<ModelComponent>()->getMaterial(1) == gold);
    REQUIRE(childBody->getComponent<ModelComponent>()->getUsedMaterialSlots().size() == 1);

    // A click picks the part, and moving a part moves only its mesh.
    SceneRayHit hit;
    REQUIRE(scene.raycast(Ray(Vector3(10.0f, 0.5f, -5.0f), Vector3::UP), hit));
    REQUIRE(hit.gameObject == root);
    leaf->getTransform().setLocalPosition(Vector3(0.0f, 0.0f, 20.0f));
    scene.updateTransforms();
    REQUIRE(partCenter(*leaf).nearEqual(Vector3(0.0f, 0.0f, 21.0f)));
    REQUIRE(partCenter(*root).nearEqual(Vector3(10.0f, 1.0f, 1.0f)));
  }

  SECTION("Hidden models stay hidden")
  {
    whole.setEnabled(false);
    REQUIRE(ModelExpand::expand(scene, *gameObject, ModelExpandMode::PerMesh));
    REQUIRE(scene.getRenderItemCount() == 0);
  }
}

namespace chEngineSDK {
/**
 * Exists so the tests have a component type that is not registered unless they register it.
 */
class TestNoteComponent : public Component
{
 public:
  void
  serialize(DataStream& stream) const override
  {
    stream.write(&note, sizeof(note));
  }

  NODISCARD bool
  deserialize(DataStream& stream) override
  {
    return stream.read(&note, sizeof(note)) == sizeof(note);
  }

  uint32 note = 0;
};
DECLARE_TYPE_TRAITS(TestNoteComponent)
} // namespace chEngineSDK

TEST_CASE("chCore - GameObject serializer")
{
  const SPtr<Model> model = createTriangleModel();
  const SPtr<MaterialAsset> body = createTestMaterialAsset("M_Body");
  model->addMaterialSlot({.name = "Body", .materialId = body->getUUID(), .material = body});
  const SPtr<ModelAsset> modelAsset = createTestModelAsset(model);
  const SPtr<MaterialAsset> gold = createTestMaterialAsset("M_Gold");

  Scene source("Source", chEngineSDK::UUID::createRandom());
  const SPtr<GameObject> sun = source.createGameObject("Sun");
  sun->getTransform().setLocalRotation(Quaternion(Rotator(-50.0f, 30.0f, 0.0f)));
  DirectionalLightComponent& sunLight = sun->addComponent<DirectionalLightComponent>();
  sunLight.setColor(LinearColor(1.0f, 0.5f, 0.25f, 1.0f));
  sunLight.setIntensity(4.0f);

  const SPtr<GameObject> parent = source.createGameObject("Parent");
  parent->getTransform().setLocalPosition(Vector3(1.0f, 2.0f, 3.0f));
  parent->getTransform().setLocalScale(Vector3(2.0f, 2.0f, 2.0f));
  // Not registered yet: reading must skip it and still read what follows.
  parent->addComponent<TestNoteComponent>().note = 42;
  ModelComponent& part =
      parent->addComponent<ModelComponent>(modelAsset, 0, 0, Vector3(0.0f, 1.0f, 0.0f));
  part.setMaterial(0, gold);

  const SPtr<GameObject> lamp = source.createGameObject("Lamp", parent.get());
  PointLightComponent& lampLight = lamp->addComponent<PointLightComponent>();
  lampLight.setRange(25.0f);
  lampLight.setEnabled(false);
  const SPtr<GameObject> spot = source.createGameObject("Spot", parent.get());
  SpotLightComponent& spotLight = spot->addComponent<SpotLightComponent>();
  spotLight.setConeAngles(Degree(10.0f), Degree(20.0f));
  spotLight.setCastShadows(true);
  source.createGameObject("Empty", spot.get());
  // A second user of the same model, which must be referenced once.
  source.createGameObject("Copy")->addComponent<ModelComponent>(modelAsset);

  Vector<chEngineSDK::UUID> references;
  GameObjectSerializer::collectReferences(source.getRootGameObjects(), references);
  REQUIRE(references.size() == 2);
  REQUIRE(references[0] == modelAsset->getUUID());
  REQUIRE(references[1] == gold->getUUID());

  MemoryDataStream stream(64 * 1024);
  GameObjectSerializer::write(stream, source.getRootGameObjects());
  const SIZE_T written = stream.tell();

  Scene loaded("Loaded", chEngineSDK::UUID::createRandom());
  stream.seek(0);
  REQUIRE(GameObjectSerializer::read(stream, loaded, nullptr));
  REQUIRE(stream.tell() == written);

  const Vector<SPtr<GameObject>>& roots = loaded.getRootGameObjects();
  REQUIRE(roots.size() == 3);
  REQUIRE(roots[0]->getName() == "Sun");
  REQUIRE(roots[0]->getUUID() == sun->getUUID());
  REQUIRE(roots[0]->getTransform().getLocalRotation() ==
          sun->getTransform().getLocalRotation());
  const DirectionalLightComponent* loadedSun =
      roots[0]->getComponent<DirectionalLightComponent>();
  REQUIRE(loadedSun != nullptr);
  REQUIRE(loadedSun->isRegistered());
  REQUIRE(loadedSun->getColor().g == 0.5f);
  REQUIRE(loadedSun->getIntensity() == 4.0f);
  REQUIRE(loadedSun->getCastShadows());

  const GameObject& loadedParent = *roots[1];
  REQUIRE(loadedParent.getUUID() == parent->getUUID());
  REQUIRE(loadedParent.getTransform().getLocalPosition() == Vector3(1.0f, 2.0f, 3.0f));
  REQUIRE(loadedParent.getTransform().getLocalScale() == Vector3(2.0f, 2.0f, 2.0f));
  REQUIRE(loadedParent.getComponents().size() == 1);
  const ModelComponent* loadedPart = loadedParent.getComponent<ModelComponent>();
  REQUIRE(loadedPart != nullptr);
  REQUIRE(loadedPart->getNodeIndex() == 0);
  REQUIRE(loadedPart->getMeshIndex() == 0);
  REQUIRE(loadedPart->getPivot() == Vector3(0.0f, 1.0f, 0.0f));
  // There is no AssetManager here, so the model is not found, but its reference is kept for
  // the next save.
  REQUIRE(loadedPart->getModelAsset() == nullptr);
  Vector<chEngineSDK::UUID> loadedReferences;
  GameObjectSerializer::collectReferences(roots, loadedReferences);
  REQUIRE(loadedReferences.size() == 1);
  REQUIRE(loadedReferences[0] == modelAsset->getUUID());

  REQUIRE(loadedParent.getChildren().size() == 2);
  const GameObject& loadedLamp = *loadedParent.getChildren()[0];
  REQUIRE(loadedLamp.getName() == "Lamp");
  REQUIRE(loadedLamp.getParent() == &loadedParent);
  const PointLightComponent* loadedLampLight = loadedLamp.getComponent<PointLightComponent>();
  REQUIRE(loadedLampLight != nullptr);
  REQUIRE(loadedLampLight->getRange() == 25.0f);
  REQUIRE_FALSE(loadedLampLight->isEnabled());
  REQUIRE_FALSE(loadedLampLight->isRegistered());

  const GameObject& loadedSpot = *loadedParent.getChildren()[1];
  const SpotLightComponent* loadedSpotLight = loadedSpot.getComponent<SpotLightComponent>();
  REQUIRE(loadedSpotLight != nullptr);
  REQUIRE(loadedSpotLight->getInnerConeAngle().valueDegree() == Approx(10.0f));
  REQUIRE(loadedSpotLight->getOuterConeAngle().valueDegree() == Approx(20.0f));
  REQUIRE(loadedSpotLight->getCastShadows());
  REQUIRE(loadedSpot.getChildren().size() == 1);
  REQUIRE(loadedSpot.getChildren()[0]->getName() == "Empty");
  REQUIRE(loadedSpot.getChildren()[0]->getComponents().empty());
  REQUIRE(roots[2]->getName() == "Copy");
  // The sun and the spot; the lamp is disabled.
  REQUIRE(loaded.getLightCount() == 2);

  // Once its type is registered, the same bytes bring the component back.
  ComponentRegister::registerType<TestNoteComponent>();
  Scene again("Again", chEngineSDK::UUID::createRandom());
  stream.seek(0);
  REQUIRE(GameObjectSerializer::read(stream, again, nullptr));
  const TestNoteComponent* note =
      again.getRootGameObjects()[1]->getComponent<TestNoteComponent>();
  REQUIRE(note != nullptr);
  REQUIRE(note->note == 42);

  // A file cut short fails instead of reading past its end.
  MemoryDataStream cut(stream.getStartPtr(), written - 10, false);
  Scene broken("Broken", chEngineSDK::UUID::createRandom());
  REQUIRE_FALSE(GameObjectSerializer::read(cut, broken, nullptr));

  // So does a version from a newer engine.
  MemoryDataStream future(64);
  const uint32 futureVersion = GameObjectSerializer::SERIALIZATION_VERSION + 1;
  const uint32 noObjects = 0;
  future.write(&futureVersion, sizeof(futureVersion));
  future.write(&noObjects, sizeof(noObjects));
  future.seek(0);
  REQUIRE_FALSE(GameObjectSerializer::read(future, broken, nullptr));
}

TEST_CASE("chCore - Prefabs")
{
  AssetMetadata metadata;
  metadata.uuid = chEngineSDK::UUID::createRandom();
  metadata.assetType = AssetTypeTraits<PrefabAsset>::getTypeId();
  StringUtils::copyToBuffer(metadata.name, "PF_LampPost");
  PrefabAsset prefab(metadata);
  REQUIRE(prefab.getContent().empty());

  // Lamp Post (point light, moved away) > Bulb (spot light)
  Scene source("Source", chEngineSDK::UUID::createRandom());
  const SPtr<GameObject> post = source.createGameObject("Lamp Post");
  post->getTransform().setLocalPosition(Vector3(50.0f, 0.0f, 0.0f));
  post->getTransform().setLocalScale(Vector3(5.0f, 5.0f, 5.0f));
  post->getTransform().setLocalRotation(Quaternion(Rotator(0.0f, 90.0f, 0.0f)));
  post->addComponent<PointLightComponent>().setRange(12.0f);
  const SPtr<GameObject> bulb = source.createGameObject("Bulb", post.get());
  bulb->getTransform().setLocalPosition(Vector3(0.0f, 0.0f, 3.0f));
  bulb->addComponent<SpotLightComponent>();
  REQUIRE(prefab.setContent(*post));
  REQUIRE_FALSE(prefab.getContent().empty());

  Scene scene("Scene", chEngineSDK::UUID::createRandom());
  const SPtr<GameObject> first = GameObjectSerializer::instantiate(prefab, scene, nullptr);
  const SPtr<GameObject> second = GameObjectSerializer::instantiate(prefab, scene, nullptr);
  second->getTransform().setLocalPosition(Vector3(0.0f, 10.0f, 0.0f));
  REQUIRE(first->getName() == "PF_LampPost");
  REQUIRE(first->getPrefab() == prefab.getUUID());
  REQUIRE(first->getOutermostPrefabInstance() == first.get());
  // An instance stands where it is placed, not where the prefab's root was.
  REQUIRE(first->getTransform().getLocalPosition() == Vector3::ZERO);
  // It starts with the rotation and scale of the prefab's root.
  REQUIRE(first->getTransform().getLocalScale() == Vector3(5.0f, 5.0f, 5.0f));
  REQUIRE(first->getTransform().getLocalRotation() ==
          post->getTransform().getLocalRotation());
  const PointLightComponent* firstLight = first->getComponent<PointLightComponent>();
  REQUIRE(firstLight != nullptr);
  REQUIRE(firstLight->getRange() == 12.0f);
  REQUIRE(first->getChildren().size() == 1);
  const SPtr<GameObject>& firstBulb = first->getChildren()[0];
  REQUIRE(firstBulb->getName() == "Bulb");
  REQUIRE(firstBulb->getTransform().getLocalPosition() == Vector3(0.0f, 0.0f, 3.0f));
  REQUIRE(firstBulb->getOutermostPrefabInstance() == first.get());
  REQUIRE(firstBulb->getPrefab().isNull());
  // Every instance gets its own UUIDs.
  REQUIRE(firstBulb->getUUID() != bulb->getUUID());
  REQUIRE(firstBulb->getUUID() != second->getChildren()[0]->getUUID());
  REQUIRE(scene.getLightCount() == 4);

  SECTION("A scene stores an instance as its link alone")
  {
    Vector<chEngineSDK::UUID> references;
    GameObjectSerializer::collectReferences(scene.getRootGameObjects(), references);
    REQUIRE(references.size() == 1);
    REQUIRE(references[0] == prefab.getUUID());

    MemoryDataStream stream;
    GameObjectSerializer::write(stream, scene.getRootGameObjects());
    stream.seek(0);
    Scene loaded("Loaded", chEngineSDK::UUID::createRandom());
    REQUIRE(GameObjectSerializer::read(stream, loaded, nullptr));
    REQUIRE(loaded.getRootGameObjects().size() == 2);
    // Without an AssetManager the prefab is not found: the instance is empty but keeps the
    // link and its own transform.
    const GameObject& loadedSecond = *loaded.getRootGameObjects()[1];
    REQUIRE(loadedSecond.getPrefab() == prefab.getUUID());
    REQUIRE(loadedSecond.getUUID() == second->getUUID());
    REQUIRE(loadedSecond.getTransform().getLocalPosition() == Vector3(0.0f, 10.0f, 0.0f));
    REQUIRE(loadedSecond.getComponents().empty());
    REQUIRE(loadedSecond.getChildren().empty());
  }

  SECTION("The content can be read into a plain object, to edit the prefab")
  {
    const SPtr<GameObject> editable = scene.createGameObject("Editable");
    REQUIRE(GameObjectSerializer::readPrefabContent(prefab, *editable));
    REQUIRE(editable->getTransform().getLocalScale() == Vector3(5.0f, 5.0f, 5.0f));
    REQUIRE(editable->getTransform().getLocalPosition() == Vector3::ZERO);
    REQUIRE(editable->getPrefab().isNull());
    REQUIRE(editable->getOutermostPrefabInstance() == nullptr);
    REQUIRE(editable->getComponent<PointLightComponent>() != nullptr);
    REQUIRE(editable->getChildren().size() == 1);
  }

  SECTION("A prefab cannot hold an instance of itself")
  {
    const SPtr<GameObject> holder = scene.createGameObject("Holder");
    GameObjectSerializer::instantiate(prefab, scene, holder.get());
    const SIZE_T sizeBefore = prefab.getContent().size();
    REQUIRE_FALSE(prefab.setContent(*holder));
    REQUIRE_FALSE(prefab.setContent(*first));
    REQUIRE(prefab.getContent().size() == sizeBefore);
    REQUIRE(prefab.dependsOn(prefab.getUUID()));
  }

  SECTION("Unpacking keeps the content as plain objects")
  {
    first->setPrefab(chEngineSDK::UUID::null());
    REQUIRE(firstBulb->getOutermostPrefabInstance() == nullptr);
    MemoryDataStream stream;
    GameObjectSerializer::write(stream, scene.getRootGameObjects());
    stream.seek(0);
    Scene loaded("Loaded", chEngineSDK::UUID::createRandom());
    REQUIRE(GameObjectSerializer::read(stream, loaded, nullptr));
    REQUIRE(loaded.getRootGameObjects()[0]->getChildren().size() == 1);
    REQUIRE(loaded.getRootGameObjects()[0]->getComponent<PointLightComponent>() != nullptr);
  }
}

TEST_CASE("chCore - Scene history")
{
  Scene scene("Test", chEngineSDK::UUID::createRandom());
  const SPtr<GameObject> a = scene.createGameObject("A");
  a->addComponent<PointLightComponent>().setRange(10.0f);
  const SPtr<GameObject> c = scene.createGameObject("C", a.get());
  const SPtr<GameObject> b = scene.createGameObject("B");
  b->addComponent<DirectionalLightComponent>();
  const SPtr<GameObject> d = scene.createGameObject("D");
  const chEngineSDK::UUID bId = b->getUUID();
  const chEngineSDK::UUID cId = c->getUUID();
  REQUIRE(scene.getLightCount() == 2);

  SceneHistory history;
  chEngineSDK::UUID focus = chEngineSDK::UUID::null();
  REQUIRE_FALSE(history.canUndo());
  REQUIRE_FALSE(history.undo(scene, focus));

  SECTION("A drag is one step, undone in place")
  {
    history.recordChange(*a, "Move");
    a->getTransform().setLocalPosition(Vector3(1.0f, 0.0f, 0.0f));
    history.recordChange(*a, "Move");
    a->getTransform().setLocalPosition(Vector3(2.0f, 0.0f, 0.0f));
    REQUIRE(history.isEntryOpen());
    REQUIRE(history.getUndoName() == "Move A");
    history.commit(scene);
    REQUIRE(history.getEntryCount() == 1);

    REQUIRE(history.undo(scene, focus));
    REQUIRE(focus == a->getUUID());
    // The same object, so whatever holds it keeps a live one.
    REQUIRE(scene.getRootGameObjects()[0] == a);
    REQUIRE(a->getTransform().getLocalPosition() == Vector3::ZERO);
    REQUIRE(a->getComponent<PointLightComponent>()->getRange() == 10.0f);
    REQUIRE(a->getChildren().size() == 1);
    REQUIRE(a->getChildren()[0]->getUUID() == cId);
    REQUIRE(scene.getLightCount() == 2);
    REQUIRE(history.canRedo());
    REQUIRE(history.getRedoName() == "Move A");

    REQUIRE(history.redo(scene, focus));
    REQUIRE(a->getTransform().getLocalPosition() == Vector3(2.0f, 0.0f, 0.0f));
    REQUIRE_FALSE(history.canRedo());
  }

  SECTION("A deleted object comes back with its UUID at its place")
  {
    history.recordChange(*b, "Delete");
    scene.destroyGameObject(*b);
    history.commit(scene);
    REQUIRE(scene.getRootGameObjects().size() == 2);
    REQUIRE(scene.getLightCount() == 1);

    REQUIRE(history.undo(scene, focus));
    REQUIRE(focus == bId);
    REQUIRE(scene.getRootGameObjects().size() == 3);
    REQUIRE(scene.getRootGameObjects()[1]->getUUID() == bId);
    REQUIRE(scene.getRootGameObjects()[2] == d);
    REQUIRE(scene.getRootGameObjects()[1]->getComponent<DirectionalLightComponent>());
    REQUIRE(scene.getLightCount() == 2);
    REQUIRE(scene.findGameObject(bId) == scene.getRootGameObjects()[1]);

    REQUIRE(history.redo(scene, focus));
    REQUIRE(scene.findGameObject(bId) == nullptr);
    REQUIRE(scene.getLightCount() == 1);
  }

  SECTION("A created object goes away and comes back")
  {
    const SPtr<GameObject> e = scene.createGameObject("E", a.get());
    e->addComponent<SpotLightComponent>();
    history.recordCreated(*e, "Create");
    // Recorded already through its creation, so this adds nothing.
    history.recordChange(*e, "Move");
    e->getTransform().setLocalPosition(Vector3(0.0f, 0.0f, 5.0f));
    history.commit(scene);
    const chEngineSDK::UUID eId = e->getUUID();

    REQUIRE(history.undo(scene, focus));
    REQUIRE(a->getChildren().size() == 1);
    REQUIRE(scene.findGameObject(eId) == nullptr);
    REQUIRE(scene.getLightCount() == 2);

    REQUIRE(history.redo(scene, focus));
    const SPtr<GameObject> again = scene.findGameObject(eId);
    REQUIRE(again != nullptr);
    REQUIRE(again->getParent() == a.get());
    REQUIRE(again->getTransform().getLocalPosition() == Vector3(0.0f, 0.0f, 5.0f));
    REQUIRE(again->getComponent<SpotLightComponent>());
    REQUIRE(scene.getLightCount() == 3);
  }

  SECTION("A parent recorded after its child undoes both")
  {
    history.recordChange(*c, "Rename");
    c->setName("Renamed");
    history.recordChange(*a, "Delete Child");
    scene.destroyGameObject(*c);
    history.commit(scene);
    REQUIRE(a->getChildren().empty());

    REQUIRE(history.undo(scene, focus));
    REQUIRE(focus == cId);
    REQUIRE(a->getChildren().size() == 1);
    REQUIRE(a->getChildren()[0]->getUUID() == cId);
    REQUIRE(a->getChildren()[0]->getName() == "C");

    REQUIRE(history.redo(scene, focus));
    REQUIRE(a->getChildren().empty());
  }

  SECTION("Nothing changed, nothing to undo")
  {
    history.recordChange(*d, "Move");
    history.commit(scene);
    REQUIRE(history.getEntryCount() == 0);
    REQUIRE_FALSE(history.canUndo());
  }

  SECTION("Saved state")
  {
    REQUIRE(history.isAtSavedState());
    history.recordChange(*d, "Rename");
    d->setName("D2");
    REQUIRE_FALSE(history.isAtSavedState());
    history.commit(scene);
    history.markSaved();
    REQUIRE(history.isAtSavedState());

    REQUIRE(history.undo(scene, focus));
    REQUIRE(d->getName() == "D");
    REQUIRE_FALSE(history.isAtSavedState());
    REQUIRE(history.redo(scene, focus));
    REQUIRE(history.isAtSavedState());

    // A new step after an undo makes the saved state unreachable.
    REQUIRE(history.undo(scene, focus));
    history.recordChange(*d, "Rename");
    d->setName("D3");
    history.commit(scene);
    REQUIRE_FALSE(history.canRedo());
    REQUIRE_FALSE(history.isAtSavedState());
  }

  SECTION("The oldest steps are dropped past the limit")
  {
    SceneHistory shortHistory(2);
    for (uint32 i = 1; i <= 3; ++i) {
      shortHistory.recordChange(*d, "Move");
      d->getTransform().setLocalPosition(Vector3(static_cast<float>(i), 0.0f, 0.0f));
      shortHistory.commit(scene);
    }
    REQUIRE(shortHistory.getEntryCount() == 2);
    REQUIRE(shortHistory.undo(scene, focus));
    REQUIRE(shortHistory.undo(scene, focus));
    REQUIRE_FALSE(shortHistory.undo(scene, focus));
    REQUIRE(d->getTransform().getLocalPosition() == Vector3(1.0f, 0.0f, 0.0f));
  }
}

TEST_CASE("chCore - Scene reparent and duplicate")
{
  Scene scene("Test", chEngineSDK::UUID::createRandom());
  const SPtr<GameObject> parent = scene.createGameObject("Parent");
  parent->getTransform().setLocalPosition(Vector3(10.0f, 0.0f, 0.0f));
  parent->getTransform().setLocalRotation(Quaternion(Rotator(0.0f, 90.0f, 0.0f)));
  parent->getTransform().setLocalScale(Vector3(2.0f, 2.0f, 2.0f));
  const SPtr<GameObject> lamp = scene.createGameObject("Lamp");
  lamp->getTransform().setLocalPosition(Vector3(0.0f, 4.0f, 1.0f));
  lamp->addComponent<PointLightComponent>().setRange(7.0f);
  const SPtr<GameObject> bulb = scene.createGameObject("Bulb", lamp.get());
  const SPtr<GameObject> last = scene.createGameObject("Last");
  scene.updateTransforms();

  SECTION("Reparenting keeps the world placement")
  {
    REQUIRE(scene.setParent(*lamp, parent.get()));
    REQUIRE(lamp->getParent() == parent.get());
    REQUIRE(parent->getChildren().back() == lamp);
    REQUIRE(scene.getRootGameObjects().size() == 2);
    REQUIRE(lamp->getTransform().getLocalScale().nearEqual(Vector3(0.5f, 0.5f, 0.5f), 1e-5f));
    scene.updateTransforms();
    REQUIRE(lamp->getTransform().getWorldPosition().nearEqual(Vector3(0.0f, 4.0f, 1.0f),
                                                              1e-4f));
    REQUIRE(bulb->getParent() == lamp.get());

    // Back to the root, last among the roots.
    REQUIRE(scene.setParent(*lamp, nullptr));
    REQUIRE(scene.getRootGameObjects().back() == lamp);
    scene.updateTransforms();
    REQUIRE(lamp->getTransform().getWorldPosition().nearEqual(Vector3(0.0f, 4.0f, 1.0f),
                                                              1e-4f));
  }

  SECTION("An object cannot go under itself or its children")
  {
    REQUIRE_FALSE(scene.setParent(*lamp, lamp.get()));
    REQUIRE_FALSE(scene.setParent(*lamp, bulb.get()));
    Scene other("Other", chEngineSDK::UUID::createRandom());
    const SPtr<GameObject> stranger = other.createGameObject("Stranger");
    REQUIRE_FALSE(scene.setParent(*lamp, stranger.get()));
    REQUIRE(lamp->getParent() == nullptr);
    REQUIRE(scene.getSiblingIndex(*lamp) == 1);
  }

  SECTION("Undo puts a reparented object back in its place")
  {
    SceneHistory history;
    history.recordChange(*lamp, "Reparent");
    REQUIRE(scene.setParent(*lamp, parent.get()));
    history.commit(scene);
    chEngineSDK::UUID focus = chEngineSDK::UUID::null();
    REQUIRE(history.undo(scene, focus));
    const SPtr<GameObject> restored = scene.findGameObject(focus);
    REQUIRE(restored != nullptr);
    REQUIRE(restored->getParent() == nullptr);
    REQUIRE(scene.getSiblingIndex(*restored) == 1);
    REQUIRE(restored->getTransform().getLocalPosition() == Vector3(0.0f, 4.0f, 1.0f));
    REQUIRE(parent->getChildren().empty());
    REQUIRE(history.redo(scene, focus));
    REQUIRE(scene.findGameObject(focus)->getParent() == parent.get());
  }

  SECTION("A duplicate goes right after its source, with new UUIDs")
  {
    const SPtr<GameObject> copy = GameObjectSerializer::duplicate(*lamp);
    REQUIRE(copy != nullptr);
    REQUIRE(scene.getRootGameObjects().size() == 4);
    REQUIRE(scene.getRootGameObjects()[2] == copy);
    REQUIRE(scene.getRootGameObjects()[3] == last);
    REQUIRE(copy->getName() == "Lamp");
    REQUIRE(copy->getUUID() != lamp->getUUID());
    REQUIRE(copy->getTransform().getLocalPosition() == Vector3(0.0f, 4.0f, 1.0f));
    const PointLightComponent* light = copy->getComponent<PointLightComponent>();
    REQUIRE(light != nullptr);
    REQUIRE(light->getRange() == 7.0f);
    REQUIRE(scene.getLightCount() == 2);
    REQUIRE(copy->getChildren().size() == 1);
    REQUIRE(copy->getChildren()[0]->getUUID() != bulb->getUUID());

    // A child is copied under the same parent.
    const SPtr<GameObject> bulbCopy = GameObjectSerializer::duplicate(*bulb);
    REQUIRE(bulbCopy != nullptr);
    REQUIRE(bulbCopy->getParent() == lamp.get());
    REQUIRE(lamp->getChildren().size() == 2);
  }

  SECTION("Copies read back get new UUIDs, as a paste")
  {
    MemoryDataStream stream;
    const SPtr<GameObject> copied[] = {lamp, last};
    GameObjectSerializer::write(stream, Span<const SPtr<GameObject>>(copied));
    stream.seek(0);
    REQUIRE(GameObjectSerializer::readCopies(stream, scene, parent.get()));
    REQUIRE(parent->getChildren().size() == 2);
    REQUIRE(parent->getChildren()[0]->getName() == "Lamp");
    REQUIRE(parent->getChildren()[0]->getUUID() != lamp->getUUID());
    REQUIRE(parent->getChildren()[0]->getChildren().size() == 1);
    REQUIRE(parent->getChildren()[1]->getName() == "Last");
    REQUIRE(scene.getLightCount() == 2);
  }

  SECTION("Undo puts reordered siblings back in their order")
  {
    SceneHistory history;
    history.recordChange(*last, "Reparent");
    scene.setSiblingIndex(*last, 0);
    history.recordChange(*lamp, "Reparent");
    scene.setSiblingIndex(*lamp, 0);
    history.setEntryName("Reparent 2 objects");
    REQUIRE(history.getUndoName() == "Reparent 2 objects");
    history.commit(scene);
    REQUIRE(scene.getRootGameObjects()[0] == lamp);
    REQUIRE(scene.getRootGameObjects()[1] == last);

    chEngineSDK::UUID focus = chEngineSDK::UUID::null();
    REQUIRE(history.undo(scene, focus));
    REQUIRE(scene.getRootGameObjects()[0] == parent);
    REQUIRE(scene.getRootGameObjects()[1] == lamp);
    REQUIRE(scene.getRootGameObjects()[2] == last);
    REQUIRE(history.redo(scene, focus));
    REQUIRE(scene.getRootGameObjects()[0] == lamp);
    REQUIRE(scene.getRootGameObjects()[1] == last);
  }
}

TEST_CASE("chCore - Texture mips")
{
  REQUIRE(TextureMips::getMipCount(1, 1) == 1);
  REQUIRE(TextureMips::getMipCount(256, 256) == 9);
  REQUIRE(TextureMips::getMipCount(300, 17) == 9);

  SECTION("1x1 has no levels to add")
  {
    Vector<uint8> pixels = {10, 20, 30, 40};
    REQUIRE(TextureMips::appendChainRGBA8(pixels, 1, 1, true) == 1);
    REQUIRE(pixels.size() == 4);
  }

  SECTION("Sizes of a non square chain")
  {
    Vector<uint8> pixels(4 * 2 * 4, 255);
    REQUIRE(TextureMips::appendChainRGBA8(pixels, 4, 2, false) == 3);
    // 4x2, 2x1, 1x1.
    REQUIRE(pixels.size() == (8 + 2 + 1) * 4);
    for (uint8 value : pixels) {
      REQUIRE(value == 255);
    }
  }

  // A black and a white pixel, alpha 0 and 255.
  const Vector<uint8> blackAndWhite = {0, 0, 0, 0, 255, 255, 255, 255};

  SECTION("Linear data is averaged as is")
  {
    Vector<uint8> pixels = blackAndWhite;
    REQUIRE(TextureMips::appendChainRGBA8(pixels, 2, 1, false) == 2);
    REQUIRE(pixels.size() == 12);
    for (SIZE_T i = 8; i < 12; ++i) {
      REQUIRE(pixels[i] == 128);
    }
  }

  SECTION("sRGB colors are averaged as linear light, alpha as is")
  {
    Vector<uint8> pixels = blackAndWhite;
    REQUIRE(TextureMips::appendChainRGBA8(pixels, 2, 1, true) == 2);
    // Half the light is 0.735 in sRGB, not 0.5.
    REQUIRE(pixels[8] == 188);
    REQUIRE(pixels[9] == 188);
    REQUIRE(pixels[10] == 188);
    REQUIRE(pixels[11] == 128);
  }

  SECTION("HDR chains keep values above 1 as half floats")
  {
    // 2x1: a bright pixel (light above 1) and a black one.
    const Vector<float> hdr = {16.0f, 4.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    const auto readHalf = [](const Vector<uint8>& data, SIZE_T index) {
      uint16 half = 0;
      std::memcpy(&half, data.data() + index * sizeof(uint16), sizeof(half));
      return Float16::toFloat(half);
    };

    Vector<uint8> data;
    REQUIRE(TextureMips::buildChainRGBA16F(hdr, 2, 1, false, data) == 1);
    REQUIRE(data.size() == 2 * 4 * sizeof(uint16));
    REQUIRE(readHalf(data, 0) == 16.0f);
    REQUIRE(readHalf(data, 2) == 0.5f);

    REQUIRE(TextureMips::buildChainRGBA16F(hdr, 2, 1, true, data) == 2);
    REQUIRE(data.size() == 3 * 4 * sizeof(uint16));
    // Light is averaged as is, no sRGB.
    REQUIRE(readHalf(data, 8) == 8.0f);
    REQUIRE(readHalf(data, 9) == 2.0f);
    REQUIRE(readHalf(data, 10) == 0.25f);
    REQUIRE(readHalf(data, 11) == 1.0f);
  }
}

namespace chEngineSDK {
/**
 * Exists so the tests can count which updates a scene runs.
 */
class TestCounterComponent : public Component
{
 public:
  explicit TestCounterComponent(bool bUpdatesWhileEditing = false)
  {
    setUpdatesWhileEditing(bUpdatesWhileEditing);
  }

  void
  fixedUpdate(float deltaTime) override
  {
    CH_PARAMETER_UNUSED(deltaTime);
    ++fixedUpdates;
  }

  void
  update(float deltaTime) override
  {
    CH_PARAMETER_UNUSED(deltaTime);
    ++updates;
  }

  uint32 fixedUpdates = 0;
  uint32 updates = 0;
};
DECLARE_TYPE_TRAITS(TestCounterComponent)
} // namespace chEngineSDK

TEST_CASE("chCore - Scene play")
{
  SECTION("A scene that does not play only updates what updates while editing")
  {
    Scene scene("Edit", chEngineSDK::UUID::createRandom());
    REQUIRE_FALSE(scene.isPlaying());
    const SPtr<GameObject> parent = scene.createGameObject("Parent");
    const SPtr<GameObject> child = scene.createGameObject("Child", parent.get());
    TestCounterComponent& game = child->addComponent<TestCounterComponent>();
    TestCounterComponent& preview = parent->addComponent<TestCounterComponent>(true);

    scene.fixedUpdate(1.0f);
    scene.update(1.0f);
    REQUIRE(game.updates == 0);
    REQUIRE(game.fixedUpdates == 0);
    REQUIRE(preview.updates == 1);
    REQUIRE(preview.fixedUpdates == 1);

    scene.setPlaying(true);
    scene.fixedUpdate(1.0f);
    scene.update(1.0f);
    REQUIRE(game.updates == 1);
    REQUIRE(game.fixedUpdates == 1);
    REQUIRE(preview.updates == 2);

    // Disabled components never update, playing or not.
    game.setEnabled(false);
    scene.update(1.0f);
    REQUIRE(game.updates == 1);
  }

  SECTION("Models animate while editing")
  {
    const ModelComponent model;
    REQUIRE(model.updatesWhileEditing());
    const TestCounterComponent counter;
    REQUIRE_FALSE(counter.updatesWhileEditing());
  }

  SECTION("The copy has the same objects and changing it leaves the source alone")
  {
    Scene source("Source", chEngineSDK::UUID::createRandom());
    const SPtr<GameObject> sun = source.createGameObject("Sun");
    sun->addComponent<DirectionalLightComponent>().setIntensity(4.0f);
    const SPtr<GameObject> parent = source.createGameObject("Parent");
    parent->getTransform().setLocalPosition(Vector3(1.0f, 2.0f, 3.0f));
    const SPtr<GameObject> child = source.createGameObject("Child", parent.get());
    child->addComponent<PointLightComponent>().setRange(7.0f);

    Scene copy("Source", chEngineSDK::UUID::createRandom());
    REQUIRE(GameObjectSerializer::copyScene(source, copy));
    REQUIRE(copy.getRootGameObjects().size() == 2);
    REQUIRE(copy.getLightCount() == 2);

    const SPtr<GameObject> copiedParent = copy.findGameObject(parent->getUUID());
    REQUIRE(copiedParent);
    REQUIRE(copiedParent != parent);
    REQUIRE(copiedParent->getName() == "Parent");
    REQUIRE(copiedParent->getTransform().getLocalPosition().nearEqual(
        Vector3(1.0f, 2.0f, 3.0f)));
    const SPtr<GameObject> copiedChild = copy.findGameObject(child->getUUID());
    REQUIRE(copiedChild);
    REQUIRE(copiedChild->getParent() == copiedParent.get());
    const PointLightComponent* light = copiedChild->getComponent<PointLightComponent>();
    REQUIRE(light);
    REQUIRE(light->getRange() == 7.0f);
    REQUIRE(copy.findGameObject(sun->getUUID())
                ->getComponent<DirectionalLightComponent>()
                ->getIntensity() == 4.0f);

    copiedParent->getTransform().setLocalPosition(Vector3(9.0f, 9.0f, 9.0f));
    copy.destroyGameObject(*copy.findGameObject(sun->getUUID()));
    REQUIRE(copy.getLightCount() == 1);
    REQUIRE(source.getLightCount() == 2);
    REQUIRE(source.getRootGameObjects().size() == 2);
    REQUIRE(parent->getTransform().getLocalPosition().nearEqual(Vector3(1.0f, 2.0f, 3.0f)));
  }

  SECTION("The scene manager keeps a copy until it is unloaded")
  {
    SceneManager::startUp();
    SceneManager& sceneManager = SceneManager::instance();
    const SPtr<Scene> copy = chMakeShared<Scene>("Copy", chEngineSDK::UUID::createRandom());
    REQUIRE(sceneManager.addScene(copy));
    REQUIRE_FALSE(sceneManager.addScene(copy));
    REQUIRE(sceneManager.setActiveScene(copy));
    REQUIRE(sceneManager.getActiveScene().lock() == copy);
    sceneManager.unloadScene(*copy);
    REQUIRE(sceneManager.getActiveScene().expired());
    SceneManager::shutDown();
  }
}

TEST_CASE("chCore - Sky lights")
{
  Scene scene("Test", chEngineSDK::UUID::createRandom());
  REQUIRE(scene.getActiveSkyLight() == nullptr);

  const SPtr<GameObject> first = scene.createGameObject("First Sky");
  SkyLightComponent& firstSky = first->addComponent<SkyLightComponent>();
  scene.updateTransforms();
  const SkyLightItem* active = scene.getActiveSkyLight();
  REQUIRE(active != nullptr);
  REQUIRE(active->owner == first.get());
  REQUIRE(active->texture == nullptr);
  REQUIRE(active->intensity == 1.0f);
  REQUIRE(active->bShowSky);

  SECTION("The last sky light registered is used")
  {
    const SPtr<GameObject> second = scene.createGameObject("Second Sky");
    second->addComponent<SkyLightComponent>();
    REQUIRE(scene.getActiveSkyLight()->owner == second.get());

    scene.destroyGameObject(*second);
    REQUIRE(scene.getActiveSkyLight()->owner == first.get());

    firstSky.setEnabled(false);
    REQUIRE(scene.getActiveSkyLight() == nullptr);
  }

  SECTION("Turning the object turns the image")
  {
    // Yaw 90 turns local forward (X) to world right (Y), so looking right sees the part of
    // the image that was in front.
    first->getTransform().setLocalRotation(Quaternion(Rotator(0.0f, 90.0f, 0.0f)));
    first->getTransform().setLocalScale(Vector3(3.0f, 3.0f, 3.0f));
    scene.updateTransforms();
    const Matrix4& worldToSky = scene.getActiveSkyLight()->worldToSky;
    REQUIRE(worldToSky.transformVector(Vector3::RIGHT).nearEqual(Vector4(1.0f, 0.0f, 0.0f, 0.0f)));
    REQUIRE(worldToSky.transformVector(Vector3::UP).nearEqual(Vector4(0.0f, 0.0f, 1.0f, 0.0f)));
  }

  SECTION("Values are saved, and a missing image keeps its id")
  {
    firstSky.setIntensity(2.5f);
    firstSky.setShowSky(false);
    REQUIRE(scene.getActiveSkyLight()->intensity == 2.5f);
    REQUIRE_FALSE(scene.getActiveSkyLight()->bShowSky);

    MemoryDataStream stream(256);
    firstSky.serialize(stream);
    stream.seek(0);
    SkyLightComponent loaded;
    REQUIRE(loaded.deserialize(stream));
    REQUIRE(loaded.getIntensity() == 2.5f);
    REQUIRE_FALSE(loaded.getShowSky());
    REQUIRE(loaded.getTextureId().isNull());

    // Version 1 data that names an image no asset manager knows.
    const chEngineSDK::UUID imageId = chEngineSDK::UUID::createRandom();
    MemoryDataStream withImage(256);
    const uint32 version = 1;
    const float intensity = 1.0f;
    const uint32 showSky = 1;
    withImage.write(&version, sizeof(version));
    withImage.write(&imageId, sizeof(imageId));
    withImage.write(&intensity, sizeof(intensity));
    withImage.write(&showSky, sizeof(showSky));
    withImage.seek(0);
    SkyLightComponent missing;
    REQUIRE(missing.deserialize(withImage));
    REQUIRE(missing.getTexture() == nullptr);
    REQUIRE(missing.getTextureId() == imageId);
    Vector<chEngineSDK::UUID> references;
    missing.collectReferences(references);
    REQUIRE(references.size() == 1);
    REQUIRE(references[0] == imageId);
  }
}

TEST_CASE("chCore - Camera component")
{
  Scene scene("Test", chEngineSDK::UUID::createRandom());
  REQUIRE(scene.getActiveCamera() == nullptr);

  const SPtr<GameObject> first = scene.createGameObject("First Camera");
  CameraComponent& firstCamera = first->addComponent<CameraComponent>();
  REQUIRE(scene.getActiveCamera() == &firstCamera);

  SECTION("The view follows the object")
  {
    // Yaw 90 turns local forward (X) to world right (Y); the scale must not matter.
    first->getTransform().setLocalPosition(Vector3(1.0f, 2.0f, 3.0f));
    first->getTransform().setLocalRotation(Quaternion(Rotator(0.0f, 90.0f, 0.0f)));
    first->getTransform().setLocalScale(Vector3(2.0f, 2.0f, 2.0f));
    scene.updateTransforms();

    Camera camera;
    firstCamera.fillCamera(camera, 1600.0f, 900.0f);
    REQUIRE(camera.getPosition().nearEqual(Vector3(1.0f, 2.0f, 3.0f)));
    REQUIRE(camera.getForwardVector().nearEqual(Vector3::RIGHT, 1e-4f));
    REQUIRE(camera.getAspectRatio() == Approx(1600.0f / 900.0f));
    REQUIRE(camera.getFieldOfView().valueDegree() == Approx(90.0f));
    REQUIRE(camera.getNearClipPlane() == Approx(0.1f));
    REQUIRE(camera.getFarClipPlane() == Approx(10000.0f));
  }

  SECTION("Looking straight down keeps the yaw and stops at 89 degrees")
  {
    first->getTransform().setLocalRotation(Quaternion(Rotator(-90.0f, 90.0f, 0.0f)));
    scene.updateTransforms();

    Camera camera;
    firstCamera.fillCamera(camera, 800.0f, 600.0f);
    const Vector3 forward = camera.getForwardVector();
    REQUIRE(forward.z == Approx(-Math::sin(Radian(Degree(89.0f)))).margin(1e-4f));
    REQUIRE(forward.y > 0.0f);
    REQUIRE(forward.x == Approx(0.0f).margin(1e-4f));
  }

  SECTION("Values are kept in range")
  {
    firstCamera.setFieldOfView(Degree(500.0f));
    REQUIRE(firstCamera.getFieldOfView().valueDegree() == Approx(170.0f));
    firstCamera.setClipPlanes(0.0f, 0.0f);
    REQUIRE(firstCamera.getNearClipPlane() == Approx(0.001f));
    REQUIRE(firstCamera.getFarClipPlane() == Approx(0.002f));
    firstCamera.setOrthographicSize(-1.0f);
    REQUIRE(firstCamera.getOrthographicSize() > 0.0f);
  }

  SECTION("The last camera registered is used")
  {
    const SPtr<GameObject> second = scene.createGameObject("Second Camera");
    CameraComponent& secondCamera = second->addComponent<CameraComponent>();
    REQUIRE(scene.getActiveCamera() == &secondCamera);

    secondCamera.setEnabled(false);
    REQUIRE(scene.getActiveCamera() == &firstCamera);

    secondCamera.setEnabled(true);
    REQUIRE(scene.getActiveCamera() == &secondCamera);

    scene.destroyGameObject(*second);
    REQUIRE(scene.getActiveCamera() == &firstCamera);

    scene.destroyGameObject(*first);
    REQUIRE(scene.getActiveCamera() == nullptr);
  }

  SECTION("Values are saved")
  {
    firstCamera.setProjectionType(CameraProjectionType::Orthographic);
    firstCamera.setFieldOfView(Degree(60.0f));
    firstCamera.setClipPlanes(0.5f, 250.0f);
    firstCamera.setOrthographicSize(12.0f);

    MemoryDataStream stream(256);
    firstCamera.serialize(stream);
    stream.seek(0);
    CameraComponent loaded;
    REQUIRE(loaded.deserialize(stream));
    REQUIRE(loaded.getProjectionType() == CameraProjectionType::Orthographic);
    REQUIRE(loaded.getFieldOfView().valueDegree() == Approx(60.0f));
    REQUIRE(loaded.getNearClipPlane() == Approx(0.5f));
    REQUIRE(loaded.getFarClipPlane() == Approx(250.0f));
    REQUIRE(loaded.getOrthographicSize() == Approx(12.0f));

    // A cut file and an unknown projection are refused.
    MemoryDataStream cut(256);
    const uint32 version = CameraComponent::SERIALIZATION_VERSION;
    cut.write(&version, sizeof(version));
    cut.seek(0);
    CameraComponent fromCut;
    REQUIRE_FALSE(fromCut.deserialize(cut));

    MemoryDataStream badProjection(256);
    const uint32 projection = 7;
    const float values[4] = {90.0f, 0.1f, 100.0f, 5.0f};
    badProjection.write(&version, sizeof(version));
    badProjection.write(&projection, sizeof(projection));
    badProjection.write(values, sizeof(values));
    badProjection.seek(0);
    CameraComponent fromBad;
    REQUIRE_FALSE(fromBad.deserialize(badProjection));
  }

  SECTION("Made from its saved type id")
  {
    const UniquePtr<Component> created =
        ComponentRegister::create(TypeTraits<CameraComponent>::getTypeId());
    REQUIRE(created != nullptr);
    REQUIRE(StringView(ComponentRegister::getTypeName(
                TypeTraits<CameraComponent>::getTypeId())) == "CameraComponent");
  }
}

namespace {
/*
 * Checks what every generated shape must hold: valid indices, triangles that show their
 * front to the outside (clockwise, away from center, or up for the plane), unit normals,
 * tangents along U and bitangents up the image, and a texture that is not mirrored.
 */
void
checkGeneratedMesh(const Mesh& mesh, const ShapeDesc& desc)
{
  const Vector<VertexStatic> vertices = mesh.getVertexData<VertexStatic>();
  Vector<uint32> indices;
  REQUIRE(mesh.readIndices(indices));
  REQUIRE_FALSE(vertices.empty());
  REQUIRE(indices.size() % 3 == 0);
  REQUIRE(indices.size() >= 3);
  REQUIRE(mesh.getIndexType() ==
          (vertices.size() <= 65535 ? IndexType::UInt16 : IndexType::UInt32));

  for (const VertexStatic& vertex : vertices) {
    REQUIRE(Math::abs(vertex.normal.magnitude() - 1.0f) < 1e-4f);
    const Vector3 tangent(vertex.tangent.x, vertex.tangent.y, vertex.tangent.z);
    REQUIRE(Math::abs(tangent.magnitude() - 1.0f) < 1e-4f);
    REQUIRE(Math::abs(tangent.dot(vertex.normal)) < 1e-4f);
    // Seen from outside, right is cross(normal, up), so an unmirrored map has w = -1.
    REQUIRE(vertex.tangent.w == -1.0f);
  }

  for (SIZE_T i = 0; i < indices.size(); i += 3) {
    REQUIRE(indices[i] < vertices.size());
    REQUIRE(indices[i + 1] < vertices.size());
    REQUIRE(indices[i + 2] < vertices.size());
    const VertexStatic& a = vertices[indices[i]];
    const VertexStatic& b = vertices[indices[i + 1]];
    const VertexStatic& c = vertices[indices[i + 2]];
    const Vector3 edge1 = b.position - a.position;
    const Vector3 edge2 = c.position - a.position;
    const Vector3 faceNormal = edge1.cross(edge2);
    REQUIRE(faceNormal.sqrMagnitude() > 0.0f);

    const Vector3 centroid = (a.position + b.position + c.position) / 3.0f;
    if (desc.type == ShapeType::Plane) {
      REQUIRE(faceNormal.z > 0.0f);
    }
    else {
      // Every other shape is convex around the origin.
      REQUIRE(faceNormal.dot(centroid) > 0.0f);
    }

    // Where U and V grow on the triangle must match the tangent frame of its vertices.
    const float du1 = b.texCoord.x - a.texCoord.x;
    const float dv1 = b.texCoord.y - a.texCoord.y;
    const float du2 = c.texCoord.x - a.texCoord.x;
    const float dv2 = c.texCoord.y - a.texCoord.y;
    const float determinant = du1 * dv2 - du2 * dv1;
    if (Math::abs(determinant) < 1e-8f) {
      continue;
    }
    const Vector3 alongU = (edge1 * dv2 - edge2 * dv1) / determinant;
    const Vector3 alongV = (edge2 * du1 - edge1 * du2) / determinant;
    for (const VertexStatic* vertex : {&a, &b, &c}) {
      const Vector3 tangent(vertex->tangent.x, vertex->tangent.y, vertex->tangent.z);
      const Vector3 imageUp = vertex->normal.cross(tangent) * vertex->tangent.w;
      REQUIRE(alongU.dot(tangent) > 0.0f);
      REQUIRE((-alongV).dot(imageUp) > 0.0f);
    }
  }
}
} // namespace

TEST_CASE("chCore - Render frame")
{
  Scene scene("Test", chEngineSDK::UUID::createRandom());
  const SPtr<Model> model = createTriangleModel();
  const SPtr<GameObject> first = scene.createGameObject("First");
  first->addComponent<ModelComponent>(createTestModelAsset(model));
  const SPtr<GameObject> second = scene.createGameObject("Second");
  second->addComponent<ModelComponent>(createTestModelAsset(model));
  const SPtr<GameObject> sun = scene.createGameObject("Sun");
  sun->addComponent<DirectionalLightComponent>();
  const SPtr<GameObject> sky = scene.createGameObject("Sky");
  SkyLightComponent& skyLight = sky->addComponent<SkyLightComponent>();
  skyLight.setIntensity(2.0f);
  scene.updateTransforms();

  // A free slot in the middle of the scene's items.
  scene.destroyGameObject(*first);
  REQUIRE(scene.getRenderItemSlots().size() == 2);

  const Camera camera = createTestCamera();
  const LinearColor clearColor(0.1f, 0.2f, 0.3f, 1.0f);
  RenderFrame frame;
  frame.extract(scene, camera, clearColor);
  REQUIRE(frame.getItems().size() == 1);
  REQUIRE(frame.getItems()[0].owner == second.get());
  REQUIRE(frame.getLights().size() == 1);
  REQUIRE(frame.getSkyLight() != nullptr);
  REQUIRE(frame.getSkyLight()->intensity == 2.0f);
  REQUIRE(frame.getClearColor().b == 0.3f);
  REQUIRE(frame.getCamera().getPosition() == camera.getPosition());

  Vector<const RenderItem*> visibleItems;
  frame.gatherItems(frame.getCamera().getFrustum(), visibleItems);
  REQUIRE(visibleItems.size() == 1);
  REQUIRE(visibleItems[0] == &frame.getItems()[0]);
  Vector<const LightItem*> visibleLights;
  frame.gatherLights(frame.getCamera().getFrustum(), visibleLights);
  REQUIRE(visibleLights.size() == 1);

  SECTION("Changes to the scene wait for the next extract")
  {
    second->getTransform().setLocalPosition(Vector3(-20.0f, 0.0f, 0.0f));
    scene.updateTransforms();
    sky->removeComponent(skyLight);
    REQUIRE(frame.getItems()[0].worldBounds.center.nearEqual(Vector3(0.0f, 1.0f, 0.0f)));
    REQUIRE(frame.getSkyLight() != nullptr);

    frame.extract(scene, camera, clearColor);
    frame.gatherItems(frame.getCamera().getFrustum(), visibleItems);
    REQUIRE(visibleItems.empty());
    REQUIRE(frame.getSkyLight() == nullptr);
  }
}

TEST_CASE("chCore - Mesh generator")
{
  SECTION("Every shape faces out with a correct tangent frame")
  {
    const ShapeDesc shapes[] = {
        {.type = ShapeType::Plane},
        {.type = ShapeType::Plane, .size = Vector3(4.0f, 2.0f, 1.0f), .subdivisions = 3},
        {.type = ShapeType::Box, .size = Vector3(2.0f, 3.0f, 4.0f)},
        {.type = ShapeType::Sphere},
        {.type = ShapeType::Sphere, .segments = 3, .rings = 2},
        {.type = ShapeType::Capsule, .radius = 0.5f, .height = 3.0f},
        // No straight part: the two halves meet at the equator.
        {.type = ShapeType::Capsule, .radius = 0.5f, .height = 1.0f, .rings = 5},
        {.type = ShapeType::Cylinder, .radius = 1.0f, .height = 2.0f, .segments = 5},
        {.type = ShapeType::Cone},
        {.type = ShapeType::Cone, .radius = 2.0f, .height = 0.5f, .segments = 3}};
    for (const ShapeDesc& desc : shapes) {
      INFO(MeshGenerator::getName(desc.type));
      Mesh mesh;
      MeshGenerator::generate(desc, mesh);
      checkGeneratedMesh(mesh, desc);
    }
  }

  SECTION("Bounds follow the sizes, centered on the origin")
  {
    const auto boundsOf = [](const ShapeDesc& desc) {
      Mesh mesh;
      MeshGenerator::generate(desc, mesh);
      return mesh.getBounds().getBox();
    };
    const AABox box = boundsOf({.type = ShapeType::Box, .size = Vector3(2.0f, 3.0f, 4.0f)});
    REQUIRE(box.minPoint.nearEqual(Vector3(-1.0f, -1.5f, -2.0f)));
    REQUIRE(box.maxPoint.nearEqual(Vector3(1.0f, 1.5f, 2.0f)));

    const AABox plane =
        boundsOf({.type = ShapeType::Plane, .size = Vector3(4.0f, 2.0f, 9.0f)});
    REQUIRE(plane.minPoint.nearEqual(Vector3(-2.0f, -1.0f, 0.0f)));
    REQUIRE(plane.maxPoint.nearEqual(Vector3(2.0f, 1.0f, 0.0f)));

    const AABox sphere =
        boundsOf({.type = ShapeType::Sphere, .radius = 2.0f, .segments = 4});
    REQUIRE(sphere.maxPoint.nearEqual(Vector3(2.0f, 2.0f, 2.0f), 1e-4f));
    REQUIRE(sphere.minPoint.nearEqual(Vector3(-2.0f, -2.0f, -2.0f), 1e-4f));

    const AABox capsule = boundsOf(
        {.type = ShapeType::Capsule, .radius = 0.5f, .height = 3.0f, .segments = 4});
    REQUIRE(capsule.maxPoint.nearEqual(Vector3(0.5f, 0.5f, 1.5f), 1e-4f));

    const AABox cone = boundsOf(
        {.type = ShapeType::Cone, .radius = 1.0f, .height = 2.0f, .segments = 4});
    REQUIRE(Math::abs(cone.maxPoint.z - 1.0f) < 1e-5f);
    REQUIRE(Math::abs(cone.minPoint.z + 1.0f) < 1e-5f);
  }

  SECTION("Big meshes use 32-bit indices")
  {
    Mesh mesh;
    MeshGenerator::generate({.type = ShapeType::Plane, .subdivisions = 256}, mesh);
    REQUIRE(mesh.getVertexCount() == 257 * 257);
    REQUIRE(mesh.getIndexType() == IndexType::UInt32);
    REQUIRE(mesh.getIndexCount() == 256 * 256 * 6);
  }

  SECTION("Clamped and canonical descriptions")
  {
    const ShapeDesc clamped = MeshGenerator::getClamped({.type = ShapeType::Capsule,
                                                         .radius = 1.0f,
                                                         .height = 0.5f,
                                                         .segments = 1,
                                                         .rings = 1000});
    REQUIRE(clamped.height == 2.0f);
    REQUIRE(clamped.segments == MeshGenerator::MIN_SEGMENTS);
    REQUIRE(clamped.rings == MeshGenerator::MAX_RINGS);

    // Fields a type does not read do not tell two meshes apart.
    const ShapeDesc boxA{.type = ShapeType::Box, .radius = 3.0f, .segments = 7};
    const ShapeDesc boxB{.type = ShapeType::Box};
    REQUIRE_FALSE(boxA == boxB);
    REQUIRE(MeshGenerator::getCanonical(boxA) == MeshGenerator::getCanonical(boxB));
    REQUIRE(MeshGenerator::getCanonical(boxA).getHash() ==
            MeshGenerator::getCanonical(boxB).getHash());
    const ShapeDesc sphere{.type = ShapeType::Sphere, .radius = 3.0f};
    REQUIRE_FALSE(MeshGenerator::getCanonical(sphere) == MeshGenerator::getCanonical(boxB));
  }
}

TEST_CASE("chCore - Shape component")
{
  Scene scene("Test", chEngineSDK::UUID::createRandom());
  const SPtr<GameObject> gameObject = scene.createGameObject("Sphere");
  ShapeComponent& shape =
      gameObject->addComponent<ShapeComponent>(ShapeDesc{.type = ShapeType::Sphere});
  gameObject->getTransform().setLocalPosition(Vector3(10.0f, 0.0f, 0.0f));
  scene.updateTransforms();
  REQUIRE(scene.getRenderItemCount() == 1);

  AABox bounds(Vector3::ZERO, Vector3::ZERO);
  REQUIRE(shape.getWorldBounds(bounds));
  REQUIRE(bounds.getCenter().nearEqual(Vector3(10.0f, 0.0f, 0.0f), 1e-4f));

  SECTION("A ray picks it")
  {
    SceneRayHit hit;
    REQUIRE(scene.raycast(Ray(Vector3::ZERO, Vector3::FORWARD), hit));
    REQUIRE(hit.gameObject == gameObject.get());
    REQUIRE(Math::abs(hit.distance - 9.5f) < 0.01f);
  }

  SECTION("Equal shapes share one mesh")
  {
    const SPtr<GameObject> other = scene.createGameObject("Other Sphere");
    ShapeComponent& otherShape =
        other->addComponent<ShapeComponent>(ShapeDesc{.type = ShapeType::Sphere});
    REQUIRE(otherShape.getMesh() == shape.getMesh());

    otherShape.setShape({.type = ShapeType::Sphere, .radius = 2.0f});
    REQUIRE(otherShape.getMesh() != shape.getMesh());
    scene.updateTransforms();
    REQUIRE(other->getHierarchyWorldBounds(bounds));
    REQUIRE(bounds.maxPoint.nearEqual(Vector3(2.0f, 2.0f, 2.0f), 1e-3f));
  }

  SECTION("Changing the shape changes the drawn mesh")
  {
    const Mesh* sphereMesh = shape.getMesh().get();
    shape.setShape({.type = ShapeType::Box, .size = Vector3(4.0f, 4.0f, 4.0f)});
    REQUIRE(shape.getMesh().get() != sphereMesh);
    REQUIRE(shape.getWorldBounds(bounds));
    REQUIRE(bounds.maxPoint.nearEqual(Vector3(12.0f, 2.0f, 2.0f), 1e-4f));
  }

  SECTION("Parents frame their shapes")
  {
    const SPtr<GameObject> child = scene.createGameObject("Child", gameObject.get());
    child->addComponent<ShapeComponent>(ShapeDesc{.type = ShapeType::Box});
    child->getTransform().setLocalPosition(Vector3(0.0f, 5.0f, 0.0f));
    scene.updateTransforms();
    REQUIRE(gameObject->getHierarchyWorldBounds(bounds));
    REQUIRE(bounds.minPoint.nearEqual(Vector3(9.5f, -0.5f, -0.5f), 1e-4f));
    REQUIRE(bounds.maxPoint.nearEqual(Vector3(10.5f, 5.5f, 0.5f), 1e-4f));
  }

  SECTION("Disabling removes the render item")
  {
    shape.setEnabled(false);
    REQUIRE(scene.getRenderItemCount() == 0);
    REQUIRE_FALSE(shape.getWorldBounds(bounds));
  }

  SECTION("Saved with its sizes, and a missing material keeps its id")
  {
    shape.setShape(
        {.type = ShapeType::Cone, .radius = 2.0f, .height = 3.0f, .segments = 12});
    MemoryDataStream stream(256);
    shape.serialize(stream);
    stream.seek(0);
    ShapeComponent loaded;
    REQUIRE(loaded.deserialize(stream));
    REQUIRE(loaded.getShape() == shape.getShape());
    REQUIRE(loaded.getMaterialId().isNull());

    const chEngineSDK::UUID materialId = chEngineSDK::UUID::createRandom();
    const uint32 version = 1;
    const float values[5] = {1.0f, 1.0f, 1.0f, 0.25f, 2.0f};
    const uint32 counts[3] = {8, 4, 1};
    const auto writeShape = [&](MemoryDataStream& out, uint32 type) {
      out.write(&version, sizeof(version));
      out.write(&materialId, sizeof(materialId));
      out.write(&type, sizeof(type));
      out.write(values, sizeof(values));
      out.write(counts, sizeof(counts));
      out.seek(0);
    };
    MemoryDataStream withMaterial(256);
    writeShape(withMaterial, static_cast<uint32>(ShapeType::Cylinder));
    ShapeComponent missing;
    REQUIRE(missing.deserialize(withMaterial));
    REQUIRE(missing.getShape().type == ShapeType::Cylinder);
    REQUIRE(missing.getShape().radius == 0.25f);
    REQUIRE(missing.getShape().segments == 8);
    REQUIRE(missing.getMaterial() == nullptr);
    REQUIRE(missing.getMaterialId() == materialId);
    Vector<chEngineSDK::UUID> references;
    missing.collectReferences(references);
    REQUIRE(references.size() == 1);
    REQUIRE(references[0] == materialId);

    // A type the engine does not know is refused.
    MemoryDataStream badType(256);
    writeShape(badType, static_cast<uint32>(ShapeType::COUNT));
    ShapeComponent refused;
    REQUIRE_FALSE(refused.deserialize(badType));
  }

  SECTION("Made from its saved type id")
  {
    const UniquePtr<Component> created =
        ComponentRegister::create(TypeTraits<ShapeComponent>::getTypeId());
    REQUIRE(created != nullptr);
    REQUIRE(StringView(ComponentRegister::getTypeName(
                TypeTraits<ShapeComponent>::getTypeId())) == "ShapeComponent");
  }
}

#if USING(CH_DEBUG_DRAW)
TEST_CASE("chCore - Debug draw")
{
  DebugDraw debugDraw;
  REQUIRE(debugDraw.getLineCount() == 0);

  SECTION("Each shape adds its lines")
  {
    debugDraw.box(AABox(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f)),
                  LinearColor::RED);
    REQUIRE(debugDraw.getLineCount() == 12);
    debugDraw.circle(Vector3::ZERO, Vector3::UP, 1.0f, LinearColor::RED, DebugDepth::Tested,
                     16);
    REQUIRE(debugDraw.getLineCount() == 28);
    debugDraw.sphere(Sphere(Vector3::ZERO, 1.0f), LinearColor::RED, DebugDepth::Tested, 8);
    REQUIRE(debugDraw.getLineCount() == 52);
    // Two circles, four side lines and four half circles.
    debugDraw.capsule(Capsule(Vector3::ZERO, Vector3::UP, 0.5f), LinearColor::RED,
                      DebugDepth::Tested, 8);
    REQUIRE(debugDraw.getLineCount() == 52 + 16 + 4 + 16);
    debugDraw.axes(Matrix4::IDENTITY, 1.0f);
    REQUIRE(debugDraw.getLineCount() == 91);
    REQUIRE(debugDraw.getVertices(DebugDepth::OnTop).empty());

    debugDraw.clear();
    REQUIRE(debugDraw.getLineCount() == 0);
  }

  SECTION("Lines on top are kept apart, with their color packed")
  {
    debugDraw.line(Vector3::ZERO, Vector3::UP, LinearColor(1.0f, 0.5f, 0.0f, 1.0f),
                   DebugDepth::OnTop);
    REQUIRE(debugDraw.getVertices(DebugDepth::Tested).empty());
    const Span<const DebugLineVertex> vertices = debugDraw.getVertices(DebugDepth::OnTop);
    REQUIRE(vertices.size() == 2);
    REQUIRE(vertices[1].position == Vector3::UP);
    REQUIRE(vertices[0].color == 0xFF0080FFu);
  }

  SECTION("An oriented box turns with its rotation")
  {
    debugDraw.orientedBox(OrientedBox(Vector3(0.0f, 0.0f, 5.0f), Vector3(2.0f, 1.0f, 1.0f),
                                      Quaternion(Rotator(0.0f, 90.0f, 0.0f))),
                          LinearColor::GREEN);
    REQUIRE(debugDraw.getLineCount() == 12);
    for (const DebugLineVertex& vertex : debugDraw.getVertices(DebugDepth::Tested)) {
      REQUIRE(Math::abs(Math::abs(vertex.position.x) - 1.0f) < 1e-4f);
      REQUIRE(Math::abs(Math::abs(vertex.position.y) - 2.0f) < 1e-4f);
      REQUIRE(Math::abs(Math::abs(vertex.position.z - 5.0f) - 1.0f) < 1e-4f);
    }
  }

  SECTION("A frustum joins the corners of what the camera sees")
  {
    const Camera camera(Vector3::ZERO, Vector3::FORWARD, 1280.0f, 720.0f);
    debugDraw.frustum(camera.getViewProjectionMatrix(), LinearColor::WHITE);
    REQUIRE(debugDraw.getLineCount() == 12);
    for (const DebugLineVertex& vertex : debugDraw.getVertices(DebugDepth::Tested)) {
      // In front of the camera, between its near and far planes.
      REQUIRE(vertex.position.x >= camera.getNearClipPlane() * 0.99f);
      REQUIRE(vertex.position.x <= camera.getFarClipPlane() * 1.01f);
    }
  }

  SECTION("Lines past the limit are dropped")
  {
    g_cvarDebugLinesMax.set(10, ConsoleVariableSource::Console);
    debugDraw.box(AABox(Vector3::ZERO, Vector3::UNIT), LinearColor::RED);
    REQUIRE(debugDraw.getLineCount() == 10);
    g_cvarDebugLinesMax.set(g_cvarDebugLinesMax.getDefault(), ConsoleVariableSource::Console);
  }

  SECTION("A scene forgets its lines when it updates")
  {
    Scene scene("Test", chEngineSDK::UUID::createRandom());
    scene.getDebugDraw().line(Vector3::ZERO, Vector3::UP, LinearColor::RED);
    REQUIRE(scene.getDebugDraw().getLineCount() == 1);
    scene.update(0.016f);
    REQUIRE(scene.getDebugDraw().getLineCount() == 0);
  }

  SECTION("Lines of the fixed step last until the next fixed step")
  {
    Scene scene("Test", chEngineSDK::UUID::createRandom());
    scene.getFixedDebugDraw().line(Vector3::ZERO, Vector3::UP, LinearColor::RED);
    scene.update(0.016f);
    REQUIRE(scene.getFixedDebugDraw().getLineCount() == 1);
    scene.fixedUpdate(0.016f);
    REQUIRE(scene.getFixedDebugDraw().getLineCount() == 0);
  }

  SECTION("A render frame copies every list")
  {
    Scene scene("Test", chEngineSDK::UUID::createRandom());
    scene.getDebugDraw().line(Vector3::ZERO, Vector3::UP, LinearColor::RED);
    scene.getFixedDebugDraw().line(Vector3::ZERO, Vector3::RIGHT, LinearColor::RED);
    DebugDraw extraLines;
    extraLines.axes(Matrix4::IDENTITY, 1.0f);
    RenderFrame frame;
    frame.extract(scene, createTestCamera(), LinearColor::BLACK, &extraLines);
    scene.update(0.016f);
    const Span<const DebugDraw* const> lists = frame.getDebugLines();
    REQUIRE(lists.size() == 3);
    REQUIRE(lists[0]->getLineCount() == 1);
    REQUIRE(lists[1]->getLineCount() == 1);
    REQUIRE(lists[2]->getLineCount() == 3);
    frame.extract(scene, createTestCamera(), LinearColor::BLACK);
    REQUIRE(lists[0]->getLineCount() == 0);
    REQUIRE(lists[2]->getLineCount() == 0);
  }
}
#else
TEST_CASE("chCore - Debug draw")
{
  // Built without the debug lines: every call compiles and keeps nothing.
  DebugDraw debugDraw;
  debugDraw.line(Vector3::ZERO, Vector3::UP, LinearColor::RED);
  debugDraw.box(AABox(Vector3::ZERO, Vector3::UNIT), LinearColor::RED, DebugDepth::OnTop);
  debugDraw.sphere(Sphere(Vector3::ZERO, 1.0f), LinearColor::RED);
  debugDraw.capsule(Capsule(Vector3::ZERO, Vector3::UP, 0.5f), LinearColor::RED);
  debugDraw.frustum(Matrix4::IDENTITY, LinearColor::RED);
  debugDraw.axes(Matrix4::IDENTITY, 1.0f);
  REQUIRE(debugDraw.getLineCount() == 0);
  REQUIRE(debugDraw.getVertices(DebugDepth::Tested).empty());
}
#endif // USING(CH_DEBUG_DRAW)

TEST_CASE("chCore - Input keys")
{
  for (uint32 i = 1; i < InputKeyUtils::KEY_COUNT; ++i) {
    const InputKey key = static_cast<InputKey>(i);
    const StringView name = InputKeyUtils::getName(key);
    REQUIRE_FALSE(name.empty());
    REQUIRE(InputKeyUtils::fromName(name) == key);
    REQUIRE(InputKeyUtils::getDevice(key) != InputDevice::None);
    REQUIRE(InputKeyUtils::getType(key) != InputKeyType::None);
  }
  REQUIRE(InputKeyUtils::fromName("leftshift") == InputKey::LeftShift);
  REQUIRE(InputKeyUtils::fromName("NotAKey") == InputKey::None);
  REQUIRE(InputKeyUtils::fromName("") == InputKey::None);
  REQUIRE(InputKeyUtils::getName(InputKey::None).empty());

  REQUIRE(InputKeyUtils::getDevice(InputKey::Numpad9) == InputDevice::Keyboard);
  REQUIRE(InputKeyUtils::getDevice(InputKey::MouseWheelUp) == InputDevice::Mouse);
  REQUIRE(InputKeyUtils::getDevice(InputKey::GamepadSouth) == InputDevice::Gamepad);
  REQUIRE(InputKeyUtils::getType(InputKey::MouseWheelUp) == InputKeyType::Button);
  REQUIRE(InputKeyUtils::getType(InputKey::MouseY) == InputKeyType::Axis1D);
  REQUIRE(InputKeyUtils::getType(InputKey::MouseDelta) == InputKeyType::Axis2D);
  REQUIRE(InputKeyUtils::getType(InputKey::GamepadRightTrigger) == InputKeyType::Axis1D);
  REQUIRE(InputKeyUtils::getType(InputKey::GamepadLeftStick) == InputKeyType::Axis2D);
  REQUIRE(InputKeyUtils::getGamepadButton(0) == InputKey::GamepadSouth);
  REQUIRE(InputKeyUtils::getGamepadAxis(5) == InputKey::GamepadRightTrigger);
  REQUIRE(InputKeyUtils::GAMEPAD_BUTTON_COUNT == 21);
  REQUIRE(InputKeyUtils::GAMEPAD_AXIS_COUNT == 6);
}

TEST_CASE("chCore - Input state")
{
  InputState state;

  SECTION("Down, pressed and released by frame")
  {
    state.beginFrame();
    state.setKey(InputKey::W, true);
    REQUIRE(state.isDown(InputKey::W));
    REQUIRE(state.wasPressed(InputKey::W));
    REQUIRE_FALSE(state.wasReleased(InputKey::W));
    REQUIRE(state.getFirstPressedKey() == InputKey::W);

    state.beginFrame();
    state.setKey(InputKey::W, true);
    REQUIRE(state.isDown(InputKey::W));
    REQUIRE_FALSE(state.wasPressed(InputKey::W));
    REQUIRE(state.getFirstPressedKey() == InputKey::None);

    state.beginFrame();
    state.setKey(InputKey::W, false);
    REQUIRE_FALSE(state.isDown(InputKey::W));
    REQUIRE(state.wasReleased(InputKey::W));
  }

  SECTION("A tap inside one frame is pressed and released")
  {
    state.beginFrame();
    state.setKey(InputKey::Space, true);
    state.setKey(InputKey::Space, false);
    REQUIRE_FALSE(state.isDown(InputKey::Space));
    REQUIRE(state.wasPressed(InputKey::Space));
    REQUIRE(state.wasReleased(InputKey::Space));
  }

  SECTION("Fixed steps see a press once, also from a frame without a step")
  {
    state.beginFrame();
    state.setKey(InputKey::E, true);
    state.beginFrame();
    state.beginFixedStep();
    REQUIRE(state.wasPressed(InputKey::E));
    state.endFixedStep();
    state.beginFixedStep();
    REQUIRE_FALSE(state.wasPressed(InputKey::E));
    REQUIRE(state.isDown(InputKey::E));
    state.endFixedStep();
    REQUIRE_FALSE(state.isInFixedStep());
  }

  SECTION("Mouse movement and wheel")
  {
    state.beginFrame();
    state.addMouseMove(10.0f, 20.0f, 2.5f, 1.0f);
    state.addMouseMove(11.0f, 22.0f, 0.5f, 2.0f);
    state.addMouseWheel(0.0f, 0.25f);
    REQUIRE(state.getMousePosition() == Vector2(11.0f, 22.0f));
    REQUIRE(state.getMouseDelta() == Vector2(3.0f, 3.0f));
    // Axes point up, window positions down.
    REQUIRE(state.getAxis(InputKey::MouseY) == -3.0f);
    REQUIRE(state.getAxis2D(InputKey::MouseDelta) == Vector2(3.0f, -3.0f));
    REQUIRE(state.getAxis(InputKey::MouseWheel) == 0.25f);
    REQUIRE(state.wasPressed(InputKey::MouseWheelUp));
    REQUIRE(state.wasReleased(InputKey::MouseWheelUp));
    REQUIRE_FALSE(state.isDown(InputKey::MouseWheelUp));
    REQUIRE_FALSE(state.wasPressed(InputKey::MouseWheelDown));

    state.beginFrame();
    REQUIRE(state.getMouseDelta() == Vector2::ZERO);
    REQUIRE(state.getMouseWheel() == Vector2::ZERO);
    REQUIRE_FALSE(state.wasPressed(InputKey::MouseWheelUp));
  }

  SECTION("Gamepads")
  {
    state.setAnalogSettings(
        {.stickDeadZone = 0.2f, .triggerDeadZone = 0.1f, .buttonThreshold = 0.5f});
    state.beginFrame();
    state.setGamepadButton(0, InputKey::GamepadSouth, true);
    REQUIRE_FALSE(state.isDown(InputKey::GamepadSouth));

    state.setGamepadConnected(1, true, GamepadType::PlayStation);
    REQUIRE(state.getConnectedGamepads() == 0b10u);
    REQUIRE(state.getLastGamepad() == 1);
    state.setGamepadButton(1, InputKey::GamepadSouth, true);
    REQUIRE(state.isDown(InputKey::GamepadSouth));
    REQUIRE(state.wasPressed(InputKey::GamepadSouth));
    REQUIRE(state.isDown(InputKey::GamepadSouth, 0b10u));
    REQUIRE_FALSE(state.isDown(InputKey::GamepadSouth, 0b01u));

    // Inside the dead zone.
    state.setGamepadAxis(1, InputKey::GamepadLeftX, 0.1f);
    state.setGamepadAxis(1, InputKey::GamepadLeftY, 0.1f);
    REQUIRE(state.getAxis2D(InputKey::GamepadLeftStick) == Vector2::ZERO);

    // Past it the rest of the travel is stretched back to 0..1 along the same direction.
    state.setGamepadAxis(1, InputKey::GamepadLeftX, 0.6f);
    state.setGamepadAxis(1, InputKey::GamepadLeftY, 0.0f);
    REQUIRE(state.getAxis(InputKey::GamepadLeftX) == Approx(0.5f));
    REQUIRE(state.getAxis2D(InputKey::GamepadLeftStick).y == 0.0f);
    REQUIRE(state.isDown(InputKey::GamepadLeftX));

    state.setGamepadAxis(1, InputKey::GamepadRightTrigger, 0.55f);
    REQUIRE(state.getAxis(InputKey::GamepadRightTrigger) == Approx(0.5f));

    // The gamepad that moves most wins.
    state.setGamepadConnected(2, true, GamepadType::Xbox);
    state.setGamepadAxis(2, InputKey::GamepadLeftX, -1.0f);
    REQUIRE(state.getAxis(InputKey::GamepadLeftX) == Approx(-1.0f));
    REQUIRE(state.getAxis(InputKey::GamepadLeftX, 0b10u) == Approx(0.5f));

    state.beginFrame();
    state.setGamepadConnected(1, false, GamepadType::Unknown);
    REQUIRE(state.wasReleased(InputKey::GamepadSouth));
    REQUIRE_FALSE(state.isDown(InputKey::GamepadSouth));
    REQUIRE(state.getAxis(InputKey::GamepadRightTrigger) == 0.0f);
    REQUIRE(state.getConnectedGamepads() == 0b100u);
  }

  SECTION("Losing the focus releases everything")
  {
    state.beginFrame();
    state.setKey(InputKey::A, true);
    state.setKey(InputKey::MouseLeft, true);
    state.beginFrame();
    state.releaseAll();
    REQUIRE_FALSE(state.isDown(InputKey::A));
    REQUIRE(state.wasReleased(InputKey::A));
    REQUIRE(state.wasReleased(InputKey::MouseLeft));
  }
}

TEST_CASE("chCore - Input bindings")
{
  InputBinding binding;
  REQUIRE(InputManager::parseBinding("W:+Y", binding));
  REQUIRE(binding == InputBinding{.key = InputKey::W, .target = InputAxisTarget::PositiveY});
  REQUIRE(InputManager::parseBinding(" GamepadRightStick * 4 ", binding));
  REQUIRE(binding == InputBinding{.key = InputKey::GamepadRightStick, .scale = 4.0f});
  REQUIRE(InputManager::parseBinding("s:-", binding));
  REQUIRE(binding == InputBinding{.key = InputKey::S, .target = InputAxisTarget::NegativeX});
  REQUIRE(InputManager::parseBinding("MouseDelta*0.5", binding));
  REQUIRE(binding.scale == 0.5f);

  REQUIRE_FALSE(InputManager::parseBinding("Banana", binding));
  REQUIRE_FALSE(InputManager::parseBinding("W:Z", binding));
  REQUIRE_FALSE(InputManager::parseBinding("W*", binding));
  REQUIRE_FALSE(InputManager::parseBinding("W*2x", binding));
  REQUIRE_FALSE(InputManager::parseBinding("", binding));

  const auto roundTrip = [](StringView text, InputActionType type) {
    InputBinding parsed;
    REQUIRE(InputManager::parseBinding(text, parsed));
    return InputManager::bindingToString(parsed, type);
  };
  REQUIRE(roundTrip("W:+Y", InputActionType::Axis2D) == "W:+Y");
  REQUIRE(roundTrip("S:-", InputActionType::Axis1D) == "S:-");
  REQUIRE(roundTrip("GamepadRightStick*4", InputActionType::Axis2D) == "GamepadRightStick*4");
  REQUIRE(roundTrip("space", InputActionType::Button) == "Space");

  using Type = InputActionType;
  const auto valid = [](StringView text, Type type) {
    InputBinding parsed;
    REQUIRE(InputManager::parseBinding(text, parsed));
    return InputManager::isBindingValid(parsed, type);
  };
  REQUIRE(valid("Space", Type::Button));
  REQUIRE(valid("GamepadLeftTrigger", Type::Button));
  REQUIRE_FALSE(valid("Space:+X", Type::Button));
  REQUIRE(valid("W", Type::Axis1D));
  REQUIRE(valid("MouseWheel", Type::Axis1D));
  REQUIRE_FALSE(valid("MouseDelta", Type::Axis1D));
  REQUIRE_FALSE(valid("W:+Y", Type::Axis1D));
  REQUIRE(valid("W:+Y", Type::Axis2D));
  REQUIRE(valid("MouseX:-Y", Type::Axis2D));
  REQUIRE_FALSE(valid("W", Type::Axis2D));
  REQUIRE(valid("GamepadLeftStick", Type::Axis2D));
  REQUIRE_FALSE(valid("GamepadLeftStick:+X", Type::Axis2D));
}

TEST_CASE("chCore - Input actions")
{
  // A local object, so every section starts clean; the module cannot start twice.
  InputManager input;

  ConfigFile defaults;
  defaults.parse("[Move]\nType=Axis2D\nKeys=W:+Y, S:-Y, D:+X, A:-X, GamepadLeftStick\n"
                 "[Jump]\nKeys=Space, GamepadSouth\n"
                 "[Zoom]\nType=Axis1D\nKeys=MouseWheel, Q:-*2\n"
                 "[Pause]\nKeys=P\nContext=Menu\n"
                 "[Broken]\nType=Axis3D\nKeys=Space\n"
                 "[Partly]\nType=Axis2D\nKeys=W, Banana, D:+X\n");
  input.loadBindings(defaults, ConfigFile());

  const InputActionId move = input.findAction("move");
  const InputActionId jump = input.findAction("Jump");
  const InputActionId zoom = input.findAction("Zoom");
  const InputActionId pause = input.findAction("Pause");
  REQUIRE(input.getActionCount() == 5);
  REQUIRE(input.findAction("Broken") == InputManager::INVALID_ACTION);
  REQUIRE(input.getBindings(input.findAction("Partly")).size() == 1);
  REQUIRE(input.getActionType(move) == InputActionType::Axis2D);
  REQUIRE(input.getActionContext(pause) == "Menu");

  const auto frame = [&input](std::initializer_list<DisplayEvent> events) {
    input.beginFrame();
    for (const DisplayEvent& event : events) {
      input.processEvent(event);
    }
    input.updateActions();
  };
  const auto key = [](InputKey inputKey, bool bDown) {
    return DisplayEvent(KeyEvent{.key = inputKey, .bDown = bDown, .modifiers = {}});
  };

  SECTION("Keys held together are not faster on the diagonal")
  {
    frame({key(InputKey::W, true), key(InputKey::D, true)});
    const Vector2 value = input.getActionValue2D(move);
    REQUIRE(value.magnitude() == Approx(1.0f));
    REQUIRE(value.x == Approx(value.y));
    REQUIRE(input.isActionDown(move));
    REQUIRE(input.wasActionPressed(move));

    frame({key(InputKey::D, false)});
    REQUIRE(input.getActionValue2D(move) == Vector2(0.0f, 1.0f));
    REQUIRE_FALSE(input.wasActionPressed(move));

    frame({key(InputKey::W, false)});
    REQUIRE_FALSE(input.isActionDown(move));
    REQUIRE(input.wasActionReleased(move));
  }

  SECTION("Sticks and scales")
  {
    frame({DisplayEvent(GamepadConnectionEvent{
               .slot = 0, .bConnected = true, .type = GamepadType::Xbox}),
           DisplayEvent(GamepadAxisEvent{
               .slot = 0, .axis = InputKey::GamepadLeftY, .value = 1.0f})});
    REQUIRE(input.getActionValue2D(move).y == Approx(1.0f));

    frame({key(InputKey::Q, true)});
    REQUIRE(input.getActionValue(zoom) == Approx(-2.0f));
    frame({DisplayEvent(MouseWheelEvent{.deltaX = 0.0f, .deltaY = 1.0f})});
    REQUIRE(input.getActionValue(zoom) == Approx(-1.0f));
  }

  SECTION("A tap inside a frame presses and releases a button")
  {
    frame({key(InputKey::Space, true), key(InputKey::Space, false)});
    REQUIRE_FALSE(input.isActionDown(jump));
    REQUIRE(input.wasActionPressed(jump));
    REQUIRE(input.wasActionReleased(jump));
  }

  SECTION("Fixed steps see an action press once")
  {
    frame({key(InputKey::Space, true)});
    frame({});
    input.beginFixedStep();
    REQUIRE(input.wasActionPressed(jump));
    input.endFixedStep();
    input.beginFixedStep();
    REQUIRE_FALSE(input.wasActionPressed(jump));
    REQUIRE(input.isActionDown(jump));
    input.endFixedStep();
  }

  SECTION("Contexts and game input")
  {
    frame({key(InputKey::P, true), key(InputKey::Space, true)});
    REQUIRE(input.isActionDown(pause));

    input.setContextEnabled("menu", false);
    frame({});
    REQUIRE_FALSE(input.isActionDown(pause));
    REQUIRE(input.wasActionReleased(pause));
    REQUIRE(input.isActionDown(jump));

    input.setGameInputEnabled(false);
    frame({});
    REQUIRE_FALSE(input.isActionDown(jump));
    // The device state keeps working for the editor.
    REQUIRE(input.getState().isDown(InputKey::Space));

    input.setGameInputEnabled(true);
    input.resetGameState();
    REQUIRE(input.isContextEnabled("Menu"));
    frame({});
    REQUIRE(input.isActionDown(pause));
  }

  SECTION("Rebinding, saving and loading the keys of the player")
  {
    REQUIRE(input.setBinding(jump, 0, InputBinding{.key = InputKey::Enter}));
    REQUIRE_FALSE(input.setBinding(
        jump, 0,
        InputBinding{.key = InputKey::GamepadLeftStick, .target = InputAxisTarget::PositiveX}));
    REQUIRE_FALSE(input.setBinding(jump, 5, InputBinding{.key = InputKey::E}));
    REQUIRE(input.setBinding(jump, 2, InputBinding{.key = InputKey::MouseLeft}));
    REQUIRE(input.getBindings(jump).size() == 3);

    frame({key(InputKey::Space, true)});
    REQUIRE_FALSE(input.isActionDown(jump));
    frame({key(InputKey::Enter, true)});
    REQUIRE(input.isActionDown(jump));

    Vector<InputActionId> users;
    input.findActionsUsingKey(InputKey::GamepadSouth, users);
    REQUIRE(users == Vector<InputActionId>{jump});

    ConfigFile user;
    input.writeUserBindings(user);
    REQUIRE(user.getSections().size() == 1);
    REQUIRE(*user.getValue("Jump", "Keys") == "Enter, GamepadSouth, MouseLeft");

    input.resetBindings(jump);
    REQUIRE(input.getBindings(jump)[0].key == InputKey::Space);
    ConfigFile unchanged;
    input.writeUserBindings(unchanged);
    REQUIRE(unchanged.empty());

    input.loadBindings(defaults, user);
    REQUIRE(input.getBindings(jump).size() == 3);
    REQUIRE(input.getDefaultBindings(jump).size() == 2);
    REQUIRE(input.removeBinding(jump, 0));
    REQUIRE(input.getBindings(jump)[0].key == InputKey::GamepadSouth);
    input.resetAllBindings();
    REQUIRE(input.getBindings(jump)[0].key == InputKey::Space);
  }

  SECTION("Capturing the next key for a rebinding menu")
  {
    InputKey captured = InputKey::None;
    input.captureNextKey([&captured](InputKey pressed) { captured = pressed; });
    frame({});
    REQUIRE(input.isCapturingKey());
    frame({key(InputKey::Space, true)});
    REQUIRE(captured == InputKey::Space);
    REQUIRE_FALSE(input.isCapturingKey());
    // The key went to the menu, not to the game.
    REQUIRE_FALSE(input.isActionDown(jump));
    // Nor later while it is held, so it does not trigger what it was just bound to.
    frame({});
    REQUIRE_FALSE(input.isActionDown(jump));
    frame({key(InputKey::Space, false)});
    frame({key(InputKey::Space, true)});
    REQUIRE(input.isActionDown(jump));
  }

  SECTION("Players read only their devices")
  {
    input.setPlayerCount(2);
    input.setPlayerDevices(0, true, 0b01u);
    input.setPlayerDevices(1, false, 0b10u);
    frame({DisplayEvent(GamepadConnectionEvent{
               .slot = 0, .bConnected = true, .type = GamepadType::Xbox}),
           DisplayEvent(GamepadConnectionEvent{
               .slot = 1, .bConnected = true, .type = GamepadType::PlayStation}),
           DisplayEvent(GamepadButtonEvent{
               .slot = 1, .button = InputKey::GamepadSouth, .bDown = true}),
           key(InputKey::W, true)});
    REQUIRE_FALSE(input.isActionDown(jump, 0));
    REQUIRE(input.isActionDown(jump, 1));
    REQUIRE(input.isActionDown(move, 0));
    REQUIRE_FALSE(input.isActionDown(move, 1));
    REQUIRE(input.getKeyDisplayName(InputKey::GamepadSouth, 1) == "Cross");
    REQUIRE(input.getKeyDisplayName(InputKey::GamepadSouth, 0) == "A");
    REQUIRE(input.getKeyDisplayName(InputKey::GamepadLeftShoulder, 0) == "Left Shoulder");
    REQUIRE_FALSE(input.isActionDown(jump, 3));
  }
}

TEST_CASE("chCore - Input key events")
{
  InputManager input;
  uint32 pressed = 0;
  uint32 released = 0;
  HEvent onPressed = input.OnKeyPressed.connect([&pressed](const KeyEvent& event) {
    pressed += event.key == InputKey::S && event.hasControl() ? 1 : 0;
  });
  HEvent onReleased =
      input.OnKeyReleased.connect([&released](const KeyEvent&) { ++released; });

  input.beginFrame();
  input.processEvent(KeyEvent{.key = InputKey::S, .bDown = true,
                              .modifiers = KeyModifier::RightControl});
  REQUIRE(pressed == 1);
  REQUIRE(input.getState().isDown(InputKey::S));
  input.processEvent(KeyEvent{.key = InputKey::S, .bDown = false, .modifiers = {}});
  REQUIRE(released == 1);
  // Window events are not input.
  input.processEvent(ResizeEvent{.width = 10, .height = 10});
  REQUIRE(pressed == 1);
  onPressed.disconnect();
  onReleased.disconnect();
}

namespace {
CH_LOG_DECLARE_STATIC(ScriptTestLog, All);

/*
 * The component tests need the module; it cannot start twice, so it starts once and stays.
 */
ScriptManager&
getTestScriptManager()
{
  if (!ScriptManager::isStarted()) {
    ScriptManager::startUp();
  }
  return ScriptManager::instance();
}

SPtr<ScriptAsset>
createTestScriptAsset(const ANSICHAR* name, const ANSICHAR* source)
{
  AssetMetadata metadata;
  metadata.uuid = chEngineSDK::UUID::createRandom();
  metadata.assetType = AssetTypeTraits<ScriptAsset>::getTypeId();
  StringUtils::copyToBuffer(metadata.name, name);
  StringUtils::copyToBuffer(metadata.assetPath, "/Game/Scripts");
  return chMakeShared<ScriptAsset>(metadata, String(source));
}
} // namespace

TEST_CASE("chCore - Script allocator")
{
  for (const bool bUsePools : {true, false}) {
    ScriptAllocator allocator(bUsePools);
    void* block = ScriptAllocator::allocate(&allocator, nullptr, 0, 24);
    REQUIRE(block != nullptr);
    REQUIRE(reinterpret_cast<SIZE_T>(block) % 16 == 0);
    std::memset(block, 0xAB, 24);
    REQUIRE(allocator.getBytesInUse() == 24);

    // Grows inside its size class, then into another one and past the pools, keeping the
    // bytes it had.
    block = ScriptAllocator::allocate(&allocator, block, 24, 30);
    block = ScriptAllocator::allocate(&allocator, block, 30, 200);
    REQUIRE(static_cast<uint8*>(block)[23] == 0xAB);
    block = ScriptAllocator::allocate(&allocator, block, 200, 1000);
    REQUIRE(static_cast<uint8*>(block)[0] == 0xAB);
    REQUIRE(allocator.getBytesInUse() == 1000);
    block = ScriptAllocator::allocate(&allocator, block, 1000, 16);
    REQUIRE(static_cast<uint8*>(block)[15] == 0xAB);

    REQUIRE(ScriptAllocator::allocate(&allocator, block, 16, 0) == nullptr);
    REQUIRE(allocator.getBytesInUse() == 0);

    if (bUsePools) {
      // A freed block is the next one given in its size class.
      void* const first = ScriptAllocator::allocate(&allocator, nullptr, 0, 40);
      REQUIRE(ScriptAllocator::allocate(&allocator, first, 40, 0) == nullptr);
      void* const second = ScriptAllocator::allocate(&allocator, nullptr, 0, 33);
      REQUIRE(second == first);
      REQUIRE(ScriptAllocator::allocate(&allocator, second, 33, 0) == nullptr);
    }
  }
}

TEST_CASE("chCore - Script binder")
{
  ScriptManager scripts(true);

  SECTION("Vectors")
  {
    REQUIRE(scripts.runString(R"lua(
      local v = Vector3(1, 2, 3)
      assert(v.x == 1 and v.y == 2 and v.z == 3)
      v.y = 5
      assert(v.y == 5)
      local zero = Vector3()
      assert(zero.x == 0 and zero.y == 0 and zero.z == 0)

      local sum = Vector3(1, 0, 0) + Vector3(0, 2, 0)
      assert(sum == Vector3(1, 2, 0))
      assert(Vector3(2, 4, 6) * 0.5 == Vector3(1, 2, 3))
      assert(2 * Vector3(1, 2, 3) == Vector3(2, 4, 6))
      assert(-Vector3(1, 2, 3) == Vector3(-1, -2, -3))
      assert(Vector3(2, 4, 6) / 2 == Vector3(1, 2, 3))
      assert(Vector3(3, 4, 0):length() == 5)
      assert(Vector3(1, 0, 0):dot(Vector3(0, 1, 0)) == 0)
      assert(Vector3(1, 0, 0):cross(Vector3(0, 1, 0)) == Vector3(0, 0, 1))
      assert(Vector3(0, 0, 10):normalized() == Vector3(0, 0, 1))
      assert(tostring(Vector3(1, 2, 3)) == "Vector3(1.000, 2.000, 3.000)")
      assert(Vector3(1, 2, 3) ~= Vector3(1, 2, 4))
      assert(Vector3(1, 2, 3) ~= "Vector3")
    )lua"));
  }

  SECTION("In-place methods change the value and return it")
  {
    REQUIRE(scripts.runString(R"lua(
      local v = Vector3(1, 1, 1)
      local same = v:add(Vector3(1, 0, 0)):scale(2)
      assert(rawequal(same, v))
      assert(v == Vector3(4, 2, 2))
      v:subVectors(Vector3(5, 5, 5), Vector3(1, 2, 3)):normalize()
      assert(v:nearEqual(Vector3(4, 3, 2):normalized()))
      v:set(3, 4, 0):clampLength(1)
      assert(v:nearEqual(Vector3(0.6, 0.8, 0)))
      local c = v:copy()
      c.x = 100
      assert(v.x ~= 100)
    )lua"));
  }

  SECTION("Constants are copies")
  {
    REQUIRE(scripts.runString(R"lua(
      local up = Vector3.UP
      up:scale(5)
      assert(Vector3.UP == Vector3(0, 0, 1))
      assert(not rawequal(Vector3.UP, Vector3.UP))
    )lua"));
  }

  SECTION("Rotations and colors")
  {
    REQUIRE(scripts.runString(R"lua(
      assert(Quaternion() == Quaternion.IDENTITY)
      local yaw = Quaternion.FromEuler(0, 90, 0)
      assert(yaw:rotateVector(Vector3.FORWARD):nearEqual(Vector3.RIGHT))
      local v = Vector3(1, 0, 0)
      v:rotate(yaw)
      assert(v:nearEqual(Vector3.RIGHT))
      local euler = yaw:toEuler()
      assert(math.abs(euler.yaw - 90) < 0.001 and math.abs(euler.pitch) < 0.001)
      assert((yaw * yaw):rotateVector(Vector3.FORWARD):nearEqual(Vector3.BACKWARD))
      local rotator = Rotator(10, 20, 30)
      rotator.yaw = 45
      assert(rotator.yaw == 45)
      assert(Color(1, 0, 0) == Color.RED)
      assert(Color.Lerp(Color.BLACK, Color.WHITE, 0.5) == Color(0.5, 0.5, 0.5, 1))
    )lua"));
  }

  SECTION("Wrong values are script errors, not crashes")
  {
    REQUIRE_FALSE(scripts.runString("local v = Vector3(1, 2, 3); return v:dot(5)"));
    REQUIRE_FALSE(scripts.runString("local v = Vector3(); return v.lenght"));
    REQUIRE_FALSE(scripts.runString("local v = Vector3(); v.w = 1"));
    REQUIRE_FALSE(scripts.runString("return Vector3(1, 2)"));
    REQUIRE_FALSE(scripts.runString("return Vector3.NOPE"));
    REQUIRE_FALSE(scripts.runString("local q = Quaternion(); return Vector3.dot(q, q)"));
    REQUIRE(scripts.runString("assert(getmetatable(Vector3()) == 'Vector3')"));
  }

  SECTION("Sandbox")
  {
    REQUIRE(scripts.runString(R"lua(
      assert(io == nil and os == nil and package == nil and debug == nil and require == nil)
      assert(load == nil and loadfile == nil and dofile == nil)
      assert(string and table and math and utf8 and coroutine)
      print("print reaches the log", 1, true)
      Log.info("Log reaches the log")
    )lua"));
    REQUIRE_FALSE(scripts.runString("this is not lua"));
  }

  SECTION("Time")
  {
    scripts.resetTime();
    scripts.beginFixedUpdate(0.5f);
    REQUIRE(scripts.runString("assert(Time.deltaTime == 0.5 and Time.fixedDeltaTime == 0.5)"));
    scripts.beginUpdate(0.25f);
    scripts.beginUpdate(0.25f);
    REQUIRE(scripts.runString(R"lua(
      assert(Time.deltaTime == 0.25 and Time.time == 0.5 and Time.frameCount == 2)
    )lua"));
  }
}

TEST_CASE("chCore - Script component")
{
  ScriptManager& scripts = getTestScriptManager();
  REQUIRE(scripts.runString("Events = { selves = {}, starts = 0, destroys = 0 }"));
  const SPtr<ScriptAsset> mover = createTestScriptAsset("Mover", R"lua(
    local Mover = { speed = 2 }
    function Mover:onStart()
      self.distance = 0
      Events.starts = Events.starts + 1
      Events.selves[self] = true
    end
    function Mover:update(dt)
      self.distance = self.distance + self.speed * dt
      Events.distance = self.distance
      Events.updates = (Events.updates or 0) + 1
    end
    function Mover:fixedUpdate(dt)
      Events.fixed = (Events.fixed or 0) + 1
    end
    function Mover:onDestroy()
      Events.destroys = Events.destroys + 1
    end
    return Mover
  )lua");

  Scene scene("Scripts", chEngineSDK::UUID::createRandom());
  const SPtr<GameObject> runner = scene.createGameObject("Runner");
  ScriptComponent& component = runner->addComponent<ScriptComponent>(mover);
  REQUIRE(component.getScriptId() == mover->getUUID());

  SECTION("Nothing runs while the scene does not play")
  {
    scene.update(0.5f);
    scene.fixedUpdate(0.5f);
    REQUIRE_FALSE(component.isRunning());
    REQUIRE(scripts.runString("assert(Events.starts == 0 and Events.updates == nil)"));
  }

  SECTION("onStart, update, fixedUpdate, onDestroy")
  {
    scene.setPlaying(true);
    scene.update(0.5f);
    scene.update(0.25f);
    scene.fixedUpdate(0.1f);
    REQUIRE(component.isRunning());
    REQUIRE(scripts.runString(R"lua(
      assert(Events.starts == 1 and Events.updates == 2 and Events.fixed == 1)
      assert(math.abs(Events.distance - 1.5) < 1e-6)
    )lua"));

    component.setEnabled(false);
    REQUIRE_FALSE(component.isRunning());
    REQUIRE(scripts.runString("assert(Events.destroys == 1)"));

    // Registered again it starts over with a new instance.
    component.setEnabled(true);
    scene.update(0.1f);
    REQUIRE(scripts.runString("assert(Events.starts == 2)"));
  }

  SECTION("Each component has its own self")
  {
    const SPtr<GameObject> other = scene.createGameObject("Other");
    other->addComponent<ScriptComponent>(mover);
    scene.setPlaying(true);
    scene.update(0.5f);
    REQUIRE(scripts.runString(R"lua(
      local count = 0
      for self in pairs(Events.selves) do
        count = count + 1
        assert(self.distance == 1 and self.speed == 2)
      end
      assert(count == 2)
    )lua"));
  }

  SECTION("Destroying the object runs onDestroy")
  {
    scene.setPlaying(true);
    scene.update(0.1f);
    scene.destroyGameObject(*runner);
    REQUIRE(scripts.runString("assert(Events.destroys == 1)"));
  }

  SECTION("An error stops only its component")
  {
    const SPtr<ScriptAsset> broken = createTestScriptAsset("Broken", R"lua(
      local Broken = {}
      function Broken:update(dt)
        error("broken on purpose")
      end
      return Broken
    )lua");
    const SPtr<GameObject> other = scene.createGameObject("Other");
    ScriptComponent& brokenComponent = other->addComponent<ScriptComponent>(broken);
    scene.setPlaying(true);
    scene.update(0.1f);
    scene.update(0.1f);
    REQUIRE(brokenComponent.hasFailed());
    REQUIRE_FALSE(brokenComponent.isRunning());
    REQUIRE(component.isRunning());
    REQUIRE(scripts.runString("assert(Events.updates == 2)"));
  }

  SECTION("A script that returns no class never runs")
  {
    const SPtr<ScriptAsset> noClass = createTestScriptAsset("NoClass", "return 5");
    component.setScript(noClass);
    scene.setPlaying(true);
    scene.update(0.1f);
    REQUIRE(component.hasFailed());
  }

  SECTION("Saved with its script, kept when the asset is missing")
  {
    MemoryDataStream stream;
    component.serialize(stream);
    stream.seek(0);
    ScriptComponent read;
    REQUIRE(read.deserialize(stream));
    REQUIRE(read.getScriptId() == mover->getUUID());
    REQUIRE(read.getScript() == nullptr);

    Vector<chEngineSDK::UUID> references;
    read.collectReferences(references);
    REQUIRE(references.size() == 1);
    REQUIRE(references[0] == mover->getUUID());
    REQUIRE(ComponentRegister::create(TypeTraits<ScriptComponent>::getTypeId()) != nullptr);
  }
}

#if USING(CH_EDITOR)
TEST_CASE("chCore - Script hot reload")
{
  ScriptManager& scripts = getTestScriptManager();
  REQUIRE(scripts.runString("Reload = { starts = 0, destroys = 0 }"));
  const SPtr<ScriptAsset> counter = createTestScriptAsset("Counter", R"lua(
    local Counter = { step = 1 }
    function Counter:onStart()
      self.count = 0
      Reload.starts = Reload.starts + 1
    end
    function Counter:update(dt)
      self.count = self.count + self.step
      Reload.count = self.count
    end
    function Counter:onDestroy()
      Reload.destroys = Reload.destroys + 1
    end
    return Counter
  )lua");

  Scene scene("Reload", chEngineSDK::UUID::createRandom());
  scene.setPlaying(true);
  const SPtr<GameObject> object = scene.createGameObject("Object");
  ScriptComponent& component = object->addComponent<ScriptComponent>(counter);
  scene.update(0.1f);
  scene.update(0.1f);
  REQUIRE(scripts.runString("assert(Reload.count == 2)"));

  SECTION("A running instance keeps self and calls the new functions")
  {
    counter->setSource(R"lua(
      local Counter = { step = 10 }
      function Counter:update(dt)
        self.count = self.count + self.step
        Reload.count = self.count
      end
      return Counter
    )lua");
    REQUIRE(scripts.updateClass(*counter));
    scene.update(0.1f);
    REQUIRE(component.isRunning());
    // The count it had goes on, with the step of the new class; onStart did not run again.
    REQUIRE(scripts.runString("assert(Reload.count == 12 and Reload.starts == 1)"));
  }

  SECTION("New text that fails keeps the previous functions")
  {
    counter->setSource("local Counter = {");
    REQUIRE_FALSE(scripts.updateClass(*counter));
    counter->setSource("return 5");
    REQUIRE_FALSE(scripts.updateClass(*counter));
    scene.update(0.1f);
    REQUIRE(component.isRunning());
    REQUIRE(scripts.runString("assert(Reload.count == 3)"));
  }

  SECTION("A component stopped by an error tries the new text")
  {
    counter->setSource(R"lua(
      local Counter = {}
      function Counter:update(dt) error("broken on purpose") end
      return Counter
    )lua");
    REQUIRE(scripts.updateClass(*counter));
    scene.update(0.1f);
    REQUIRE(component.hasFailed());
    scene.update(0.1f);
    REQUIRE(component.hasFailed());

    counter->setSource(R"lua(
      local Counter = {}
      function Counter:onStart() Reload.starts = Reload.starts + 1 end
      function Counter:update(dt) Reload.fixed = true end
      return Counter
    )lua");
    REQUIRE(scripts.updateClass(*counter));
    scene.update(0.1f);
    REQUIRE(component.isRunning());
    REQUIRE(scripts.runString("assert(Reload.fixed and Reload.starts == 2)"));
  }

  SECTION("Restart runs onDestroy and a new onStart")
  {
    REQUIRE(component.restart());
    REQUIRE(scripts.runString("assert(Reload.destroys == 1 and Reload.starts == 2)"));
    scene.update(0.1f);
    REQUIRE(scripts.runString("assert(Reload.count == 1)"));
  }

  scene.destroyGameObject(*object);
  scripts.forgetClass(counter->getUUID());
}
#endif

TEST_CASE("chCore - Script properties")
{
  ScriptManager& scripts = getTestScriptManager();
  REQUIRE(scripts.runString("Props = { starts = 0 }"));
  const SPtr<ScriptAsset> tuner = createTestScriptAsset("Tuner", R"lua(
    local Tuner = {}
    Tuner.properties = {
      { "speed", 5 },
      { "count", 3, type = "integer", min = 0, max = 10, tooltip = "How many" },
      { "label", "hello" },
      { "enabled", true },
      { "offset", Vector3(1, 2, 3) },
      { "tint", type = "Color" },
      { "target", type = "GameObject" },
    }
    function Tuner:onStart()
      Props.starts = Props.starts + 1
      Props[self.gameObject:getName()] = {
        speed = self.speed, count = self.count, label = self.label, enabled = self.enabled,
        offsetX = self.offset.x, tintR = self.tint.r, target = self.target,
        countIsInteger = math.type(self.count) == "integer",
      }
      -- In place: must reach neither the class nor other instances.
      self.offset.x = 100
    end
    return Tuner
  )lua");

  const auto makeNumber = [](const double number) {
    ScriptPropertyValue value = ScriptPropertyValue::makeDefault(ScriptPropertyType::Number);
    value.number = number;
    return value;
  };
  const auto makeObject = [](const chEngineSDK::UUID& id) {
    ScriptPropertyValue value =
        ScriptPropertyValue::makeDefault(ScriptPropertyType::GameObject);
    value.object = id;
    return value;
  };

  SECTION("The list is read in order with its types and defaults")
  {
    const ScriptClassInfo* scriptClass = scripts.getClass(*tuner);
    REQUIRE(scriptClass);
    REQUIRE(scriptClass->bValid);
    const Vector<ScriptProperty>& properties = scriptClass->properties;
    REQUIRE(properties.size() == 7);
    REQUIRE(properties[0].name == "speed");
    REQUIRE(properties[0].defaultValue.type == ScriptPropertyType::Number);
    REQUIRE(properties[0].defaultValue.number == 5.0);
    REQUIRE(properties[1].defaultValue.type == ScriptPropertyType::Integer);
    REQUIRE(properties[1].defaultValue.integer == 3);
    REQUIRE(properties[2].defaultValue.text == "hello");
    REQUIRE(properties[3].defaultValue.boolean);
    REQUIRE(properties[4].defaultValue.type == ScriptPropertyType::Vector3);
    REQUIRE(properties[4].defaultValue.components[2] == 3.0f);
    // A color without a default is white.
    REQUIRE(properties[5].defaultValue.type == ScriptPropertyType::Color);
    REQUIRE(properties[5].defaultValue.components[0] == 1.0f);
    REQUIRE(properties[6].defaultValue.type == ScriptPropertyType::GameObject);
    REQUIRE(properties[6].defaultValue.object.isNull());
#if USING(CH_EDITOR)
    REQUIRE(properties[1].minimum == 0.0);
    REQUIRE(properties[1].maximum == 10.0);
    REQUIRE(properties[1].tooltip == "How many");
#endif
  }

  SECTION("A list written wrong stops the script")
  {
    constexpr const ANSICHAR* kBroken[] = {
        "return { properties = 5 }",
        "return { properties = { 5 } }",
        "return { properties = { { 5, 1 } } }",
        "return { properties = { { \"a\", 1, mni = 0 } } }",
        "return { properties = { { \"a\", 1 }, { \"a\", 2 } } }",
        "return { properties = { { \"update\", 1 } } }",
        "return { properties = { { \"a\" } } }",
        "return { properties = { { \"a\", {} } } }",
        "return { properties = { { \"a\", \"text\", type = \"number\" } } }",
        "return { properties = { { \"a\", 2.5, type = \"integer\" } } }",
        "return { properties = { { \"a\", 1, type = \"Vector9\" } } }",
        "return { properties = { { \"a\", 1, type = \"GameObject\" } } }",
        "return { properties = { { \"a\", 1, min = \"low\" } } }",
    };
    for (const ANSICHAR* source : kBroken) {
      const SPtr<ScriptAsset> broken = createTestScriptAsset("Broken", source);
      const ScriptClassInfo* scriptClass = scripts.getClass(*broken);
      INFO(source);
      REQUIRE(scriptClass);
      REQUIRE_FALSE(scriptClass->bValid);
      scripts.forgetClass(broken->getUUID());
    }
  }

  Scene scene("Properties", chEngineSDK::UUID::createRandom());
  scene.setPlaying(true);
  const SPtr<GameObject> first = scene.createGameObject("First");
  const SPtr<GameObject> second = scene.createGameObject("Second");
  const SPtr<GameObject> enemy = scene.createGameObject("Enemy");
  ScriptComponent& firstScript = first->addComponent<ScriptComponent>(tuner);
  ScriptComponent& secondScript = second->addComponent<ScriptComponent>(tuner);

  SECTION("self holds the overrides or the defaults before onStart")
  {
    firstScript.setOverride("speed", makeNumber(9.0));
    firstScript.setOverride("target", makeObject(enemy->getUUID()));
    // Another type than the script lists: ignored.
    ScriptPropertyValue wrongType = ScriptPropertyValue::makeDefault(ScriptPropertyType::String);
    wrongType.text = "fast";
    secondScript.setOverride("count", wrongType);
    scene.update(0.1f);
    REQUIRE(scripts.runString(R"lua(
      assert(Props.starts == 2)
      local first, second = Props.First, Props.Second
      assert(first.speed == 9 and second.speed == 5)
      assert(first.count == 3 and second.count == 3 and first.countIsInteger)
      assert(first.label == "hello" and first.enabled == true)
      assert(first.offsetX == 1 and second.offsetX == 1)
      assert(first.tintR == 1)
      assert(first.target ~= nil and first.target:getName() == "Enemy")
      assert(second.target == nil)
    )lua"));
  }

#if USING(CH_EDITOR)
  SECTION("Changes while running reach self")
  {
    scene.update(0.1f);
    REQUIRE(firstScript.isRunning());
    firstScript.setOverride("speed", makeNumber(42.0));
    ScriptPropertyValue live;
    REQUIRE(scripts.getInstanceProperty(firstScript.getInstance(), "speed",
                                        ScriptPropertyType::Number, live));
    REQUIRE(live.number == 42.0);
    REQUIRE(firstScript.removeOverride("speed"));
    REQUIRE(scripts.getInstanceProperty(firstScript.getInstance(), "speed",
                                        ScriptPropertyType::Number, live));
    REQUIRE(live.number == 5.0);
    REQUIRE_FALSE(firstScript.removeOverride("speed"));
    // The vector the script changed in place is read back as it is now.
    REQUIRE(scripts.getInstanceProperty(firstScript.getInstance(), "offset",
                                        ScriptPropertyType::Vector3, live));
    REQUIRE(live.components[0] == 100.0f);
    REQUIRE_FALSE(scripts.getInstanceProperty(firstScript.getInstance(), "offset",
                                              ScriptPropertyType::Number, live));
  }
#endif

  SECTION("Overrides are saved, also those the script no longer lists")
  {
    firstScript.setOverride("speed", makeNumber(7.5));
    firstScript.setOverride("gone", makeObject(enemy->getUUID()));
    ScriptPropertyValue color = ScriptPropertyValue::makeDefault(ScriptPropertyType::Color);
    color.components[1] = 0.25f;
    firstScript.setOverride("tint", color);

    MemoryDataStream stream;
    firstScript.serialize(stream);
    MemoryDataStream reader(stream.getStartPtr(), stream.size(), false);
    ScriptComponent loaded;
    REQUIRE(loaded.deserialize(reader));
    REQUIRE(loaded.getScriptId() == tuner->getUUID());
    REQUIRE(loaded.getOverrides().size() == 3);
    REQUIRE(*loaded.findOverride("speed") == makeNumber(7.5));
    REQUIRE(*loaded.findOverride("gone") == makeObject(enemy->getUUID()));
    REQUIRE(*loaded.findOverride("tint") == color);

    // Cut anywhere inside the overrides, the data is refused.
    MemoryDataStream cut(stream.getStartPtr(), stream.size() - 3, false);
    ScriptComponent broken;
    REQUIRE_FALSE(broken.deserialize(cut));
  }

  SECTION("Copies point at the copies of the objects copied with them")
  {
    // Holder > Weapon, Holder's script points at Weapon and at Enemy (not copied).
    const SPtr<GameObject> holder = scene.createGameObject("Holder");
    const SPtr<GameObject> weapon = scene.createGameObject("Weapon", holder.get());
    ScriptComponent& holderScript = holder->addComponent<ScriptComponent>(tuner);
    holderScript.setOverride("target", makeObject(weapon->getUUID()));
    holderScript.setOverride("other", makeObject(enemy->getUUID()));

    const SPtr<GameObject> copy = GameObjectSerializer::duplicate(*holder);
    REQUIRE(copy);
    REQUIRE(copy->getChildren().size() == 1);
    const SPtr<GameObject>& weaponCopy = copy->getChildren()[0];
    REQUIRE(weaponCopy->getUUID() != weapon->getUUID());
    const ScriptComponent* copyScript = copy->getComponent<ScriptComponent>();
    REQUIRE(copyScript);
    REQUIRE(copyScript->findOverride("target")->object == weaponCopy->getUUID());
    REQUIRE(copyScript->findOverride("other")->object == enemy->getUUID());
    // The source keeps its own.
    REQUIRE(holderScript.findOverride("target")->object == weapon->getUUID());

    // Instances of a prefab point inside themselves, also at their root.
    AssetMetadata metadata;
    metadata.uuid = chEngineSDK::UUID::createRandom();
    metadata.assetType = AssetTypeTraits<PrefabAsset>::getTypeId();
    StringUtils::copyToBuffer(metadata.name, "PF_Holder");
    PrefabAsset prefab(metadata);
    holderScript.setOverride("self", makeObject(holder->getUUID()));
    REQUIRE(prefab.setContent(*holder));
    const SPtr<GameObject> instance = GameObjectSerializer::instantiate(prefab, scene, nullptr);
    REQUIRE(instance->getChildren().size() == 1);
    const ScriptComponent* instanceScript = instance->getComponent<ScriptComponent>();
    REQUIRE(instanceScript);
    REQUIRE(instanceScript->findOverride("target")->object ==
            instance->getChildren()[0]->getUUID());
    REQUIRE(instanceScript->findOverride("self")->object == instance->getUUID());
    REQUIRE(instanceScript->findOverride("other")->object == enemy->getUUID());
  }

#if USING(CH_EDITOR)
  SECTION("A property added to a running script reaches its instances")
  {
    scene.update(0.1f);
    tuner->setSource(R"lua(
      local Tuner = {}
      Tuner.properties = { { "speed", 5 }, { "jump", 2.5 } }
      function Tuner:update(dt)
        Props[self.gameObject:getName() .. "Jump"] = self.jump
        Props[self.gameObject:getName() .. "Speed"] = self.speed
      end
      return Tuner
    )lua");
    firstScript.setOverride("speed", makeNumber(8.0));
    REQUIRE(scripts.updateClass(*tuner));
    scene.update(0.1f);
    // The new property arrives; the one self already held keeps its value.
    REQUIRE(scripts.runString(R"lua(
      assert(Props.FirstJump == 2.5 and Props.FirstSpeed == 8)
      assert(Props.SecondJump == 2.5 and Props.SecondSpeed == 5)
    )lua"));
  }
#endif

  scene.destroyGameObject(*first);
  scene.destroyGameObject(*second);
  scripts.forgetClass(tuner->getUUID());
}

/*
 * Not run by default (ctest skips hidden tags); run it in Release:
 *   chCoreTest "chCore - Script steering benchmark"
 * Moves 1000 agents toward a target with seek steering, written three ways.
 */
TEST_CASE("chCore - Script steering benchmark", "[.benchmark]")
{
  constexpr const ANSICHAR* kSetup = R"lua(
    N = 1000
    local maxSpeed, maxForce = 5.0, 10.0
    target = Vector3(0, 0, 0)
    opAgents, ipAgents, numAgents = {}, {}, {}
    for i = 1, N do
      local x, y = math.cos(i) * 50, math.sin(i) * 50
      opAgents[i] = { pos = Vector3(x, y, 0), vel = Vector3() }
      ipAgents[i] = { pos = Vector3(x, y, 0), vel = Vector3(), desired = Vector3(),
                      steer = Vector3() }
      numAgents[i] = { px = x, py = y, pz = 0, vx = 0, vy = 0, vz = 0 }
    end

    function stepOperators(dt)
      local t = target
      for i = 1, N do
        local a = opAgents[i]
        local steer = (t - a.pos):normalized() * maxSpeed - a.vel
        local length = steer:length()
        if length > maxForce then steer = steer * (maxForce / length) end
        a.vel = a.vel + steer * dt
        a.pos = a.pos + a.vel * dt
      end
    end

    function stepInPlace(dt)
      local t = target
      for i = 1, N do
        local a = ipAgents[i]
        a.desired:subVectors(t, a.pos):normalize():scale(maxSpeed)
        a.steer:subVectors(a.desired, a.vel):clampLength(maxForce)
        a.vel:addScaled(a.steer, dt)
        a.pos:addScaled(a.vel, dt)
      end
    end

    function stepNumbers(dt)
      local tx, ty, tz = target.x, target.y, target.z
      local sqrt = math.sqrt
      for i = 1, N do
        local a = numAgents[i]
        local dx, dy, dz = tx - a.px, ty - a.py, tz - a.pz
        local length = sqrt(dx * dx + dy * dy + dz * dz)
        if length > 1e-8 then
          local k = maxSpeed / length
          dx, dy, dz = dx * k, dy * k, dz * k
        end
        local sx, sy, sz = dx - a.vx, dy - a.vy, dz - a.vz
        local steerLength = sqrt(sx * sx + sy * sy + sz * sz)
        if steerLength > maxForce then
          local k = maxForce / steerLength
          sx, sy, sz = sx * k, sy * k, sz * k
        end
        a.vx, a.vy, a.vz = a.vx + sx * dt, a.vy + sy * dt, a.vz + sz * dt
        a.px, a.py, a.pz = a.px + a.vx * dt, a.py + a.vy * dt, a.pz + a.vz * dt
      end
    end
  )lua";
  constexpr uint32 kWarmUpFrames = 30;
  constexpr uint32 kFrames = 300;
  constexpr float kDeltaTime = 1.0f / 60.0f;

  for (const bool bPool : {true, false}) {
    ScriptManager scripts(bPool);
    REQUIRE(scripts.runString(kSetup));
    lua_State* const state = scripts.getState();

    for (const ANSICHAR* style : {"stepOperators", "stepInPlace", "stepNumbers"}) {
      const auto runFrame = [state, style]() {
        lua_getglobal(state, style);
        lua_pushnumber(state, static_cast<lua_Number>(kDeltaTime));
        lua_call(state, 1, 0);
      };
      for (uint32 frame = 0; frame < kWarmUpFrames; ++frame) {
        runFrame();
      }
      scripts.collectGarbage();
      const uint64 allocationsBefore = scripts.getAllocationCount();
      const auto start = std::chrono::steady_clock::now();
      for (uint32 frame = 0; frame < kFrames; ++frame) {
        runFrame();
      }
      const auto end = std::chrono::steady_clock::now();
      const double milliseconds =
          std::chrono::duration<double, std::milli>(end - start).count() / kFrames;
      const double allocations =
          static_cast<double>(scripts.getAllocationCount() - allocationsBefore) / kFrames;
      CH_LOG_INFO(ScriptTestLog, "{} with {}: {:.3f} ms per frame, {:.0f} allocations per frame",
                  style, bPool ? "pools" : "malloc", milliseconds, allocations);
    }

    // The three ways move the agents the same (float vectors against double numbers).
    REQUIRE(scripts.runString(R"lua(
      for i = 1, N, 97 do
        local a, b, c = opAgents[i], ipAgents[i], numAgents[i]
        assert(a.pos:nearEqual(b.pos))
        assert(math.abs(a.pos.x - c.px) < 1e-2 and math.abs(a.pos.y - c.py) < 1e-2)
      end
    )lua"));
  }

  // The same work in C++, for scale.
  struct Agent
  {
    Vector3 position;
    Vector3 velocity;
  };
  Vector<Agent> agents(1000);
  for (uint32 i = 0; i < agents.size(); ++i) {
    const Radian angle(static_cast<float>(i + 1));
    agents[i] = {Vector3(Math::cos(angle) * 50.0f, Math::sin(angle) * 50.0f, 0.0f),
                 Vector3::ZERO};
  }
  const auto start = std::chrono::steady_clock::now();
  for (uint32 frame = 0; frame < kWarmUpFrames + kFrames; ++frame) {
    for (Agent& agent : agents) {
      Vector3 steer =
          (Vector3::ZERO - agent.position).getNormalized() * 5.0f - agent.velocity;
      const float length = steer.magnitude();
      if (length > 10.0f) {
        steer *= 10.0f / length;
      }
      agent.velocity += steer * kDeltaTime;
      agent.position += agent.velocity * kDeltaTime;
    }
  }
  const auto end = std::chrono::steady_clock::now();
  CH_LOG_INFO(ScriptTestLog, "C++: {:.3f} ms per frame",
              std::chrono::duration<double, std::milli>(end - start).count() /
                  (kWarmUpFrames + kFrames));
  REQUIRE(agents[0].position.magnitude() < 50.0f);
}

TEST_CASE("chCore - Script objects")
{
  ScriptManager& scripts = getTestScriptManager();
  REQUIRE(scripts.runString("Found = {}"));
  const SPtr<ScriptAsset> probe = createTestScriptAsset("Probe", R"lua(
    local Probe = {}
    function Probe:onStart()
      Found.self = self
      Found.object = self.gameObject
      Found.transform = self.transform
    end
    return Probe
  )lua");

  Scene scene("Objects", chEngineSDK::UUID::createRandom());
  scene.setPlaying(true);
  const SPtr<GameObject> parent = scene.createGameObject("Parent");
  const SPtr<GameObject> child = scene.createGameObject("Child", parent.get());
  const SPtr<GameObject> grandChild = scene.createGameObject("GrandChild", child.get());
  parent->getTransform().setLocalPosition(Vector3(10.0f, 0.0f, 0.0f));
  parent->getTransform().setLocalRotation(Quaternion(Rotator(0.0f, 90.0f, 0.0f)));
  child->getTransform().setLocalPosition(Vector3(1.0f, 0.0f, 0.0f));
  child->addComponent<ScriptComponent>(probe);
  scene.update(0.1f);
  REQUIRE(scripts.runString("assert(Found.object and Found.transform)"));

  SECTION("Handles are the same value for the same object")
  {
    REQUIRE(scripts.runString(R"lua(
      local object = Found.object
      assert(object == Scene.find("Child"))
      assert(object:getTransform() == Found.transform)
      assert(Found.transform:getGameObject() == object)
      assert(object:getName() == "Child" and object:isValid())
      assert(tostring(object) == "GameObject(Child)")
      assert(object:getParent():getName() == "Parent")
      assert(object:getParent():getParent() == nil)
      assert(object:getChildCount() == 1 and object:getChild(1):getName() == "GrandChild")
      assert(object:getChild(2) == nil)
      assert(Scene.find("Parent"):findChild("GrandChild") == nil)
      assert(Scene.find("Parent"):findDescendant("GrandChild"):getName() == "GrandChild")
      assert(Scene.find("Nobody") == nil)
    )lua"));
  }

  SECTION("Local and world placement")
  {
    // The parent stands at x = 10 turned 90 degrees right, so the child's local +X is world +Y.
    REQUIRE(scripts.runString(R"lua(
      local t = Found.transform
      assert(t:getLocalPosition() == Vector3(1, 0, 0))
      assert(t:getWorldPosition():nearEqual(Vector3(10, 1, 0)))
      assert(t:getForward():nearEqual(Vector3.RIGHT))

      local out = Vector3()
      assert(rawequal(t:getWorldPosition(out), out))
      assert(out:nearEqual(Vector3(10, 1, 0)))

      t:setWorldPosition(10, 5, 2)
      assert(t:getWorldPosition():nearEqual(Vector3(10, 5, 2)))
      assert(t:getLocalPosition():nearEqual(Vector3(5, 0, 2)))

      t:setLocalPosition(Vector3(1, 0, 0))
      t:translateLocal(2, 0, 0)
      assert(t:getWorldPosition():nearEqual(Vector3(10, 3, 0)))
      t:translateWorld(0, 0, 4)
      assert(t:getWorldPosition():nearEqual(Vector3(10, 3, 4)))

      t:setWorldRotation(Quaternion.IDENTITY)
      assert(t:getForward():nearEqual(Vector3.FORWARD))
      t:rotateLocal(0, 90, 0)
      assert(t:getForward():nearEqual(Vector3.RIGHT))

      t:lookAt(Vector3(10, 3, 10))
      assert(t:getForward():nearEqual(Vector3.UP))
      t:lookAt(Vector3(20, 3, 4))
      assert(t:getForward():nearEqual(Vector3.FORWARD))

      t:setLocalScale(2, 2, 2)
      assert(t:getLocalScale() == Vector3(2, 2, 2))
    )lua"));
    scene.updateTransforms();
    REQUIRE(child->getTransform().getWorldPosition().nearEqual(Vector3(10.0f, 3.0f, 4.0f),
                                                               0.001f));
  }

  SECTION("A destroyed object is an error, and isValid says so")
  {
    scene.destroyGameObject(*grandChild);
    REQUIRE(scripts.runString(R"lua(
      Found.gone = Found.object:getChild(1)
    )lua"));
    REQUIRE(scripts.runString("assert(Found.gone == nil)"));

    REQUIRE(scripts.runString("Found.parent = Found.object:getParent()"));
    scene.destroyGameObject(*parent);
    REQUIRE(scripts.runString(R"lua(
      assert(not Found.parent:isValid() and not Found.object:isValid())
      assert(tostring(Found.object) == "GameObject(destroyed)")
    )lua"));
    REQUIRE_FALSE(scripts.runString("return Found.object:getName()"));
    REQUIRE_FALSE(scripts.runString("return Found.transform:getWorldPosition()"));
  }

  SECTION("A handle outlives its object without dangling")
  {
    {
      const SPtr<GameObject> temporary = scene.createGameObject("Temporary");
      ScriptTraits<GameObject>::push(scripts.getState(), *temporary);
      lua_setglobal(scripts.getState(), "Temporary");
      scene.destroyGameObject(*temporary);
    }
    REQUIRE(scripts.runString("assert(not Temporary:isValid())"));
    REQUIRE(scripts.runString("assert(tostring(Temporary) == 'GameObject(destroyed)')"));
    REQUIRE_FALSE(scripts.runString("return Temporary:getName()"));
  }

  SECTION("Scripts reach each other through getScript")
  {
    const SPtr<ScriptAsset> health = createTestScriptAsset("Health", R"lua(
      local Health = { points = 100 }
      function Health:onStart() self.started = true end
      function Health:damage(amount) self.points = self.points - amount end
      return Health
    )lua");
    const SPtr<ScriptAsset> attacker = createTestScriptAsset("Attacker", R"lua(
      local Attacker = {}
      function Attacker:onStart()
        local target = Scene.find("GrandChild"):getScript("Health")
        assert(target and target.started)
        target:damage(30)
        Found.points = target.points
        assert(self.gameObject:getScript("Nope") == nil)
      end
      return Attacker
    )lua");
    // The parent updates before its descendants, so the attacker starts the target itself.
    parent->addComponent<ScriptComponent>(attacker);
    grandChild->addComponent<ScriptComponent>(health);
    scene.update(0.1f);
    REQUIRE(scripts.runString("assert(Found.points == 70)"));

    REQUIRE(scripts.runString(R"lua(
      local holder = Scene.find("GrandChild")
      assert(holder:isScriptEnabled("Health"))
      holder:setScriptEnabled("Health", false)
      assert(not holder:isScriptEnabled("Health") and holder:getScript("Health") == nil)
    )lua"));
    REQUIRE_FALSE(scripts.runString("Scene.find('Parent'):setScriptEnabled('Nope', true)"));
  }
}

TEST_CASE("chCore - Script input")
{
  ScriptManager& scripts = getTestScriptManager();
  // A local object, so every section starts clean; the module cannot start twice.
  InputManager input;
  struct InputScope
  {
    explicit InputScope(ScriptManager& inScripts, InputManager& localInput)
      : scripts(inScripts)
    {
      scripts.setInputManager(&localInput);
    }

    ~InputScope()
    {
      scripts.setInputManager(nullptr);
    }

    ScriptManager& scripts;
  } inputScope(scripts, input);

  ConfigFile defaults;
  defaults.parse("[Move]\nType=Axis2D\nKeys=W:+Y, S:-Y, D:+X, A:-X\n"
                 "[Jump]\nKeys=Space, GamepadSouth\n"
                 "[Zoom]\nType=Axis1D\nKeys=MouseWheel\n"
                 "[Pause]\nKeys=P\nContext=Menu\n");
  input.loadBindings(defaults, ConfigFile());

  const auto frame = [&input](std::initializer_list<DisplayEvent> events) {
    input.beginFrame();
    for (const DisplayEvent& event : events) {
      input.processEvent(event);
    }
    input.updateActions();
  };
  const auto key = [](InputKey inputKey, bool bDown) {
    return DisplayEvent(KeyEvent{.key = inputKey, .bDown = bDown, .modifiers = {}});
  };

  SECTION("Actions by id and by name")
  {
    frame({key(InputKey::W, true), key(InputKey::Space, true)});
    REQUIRE(scripts.runString(R"lua(
      local move = Input.findAction("move")
      local jump = Input.findAction("Jump")
      assert(move and jump and Input.findAction("Nope") == nil)
      assert(Input.getActionCount() == 4)
      assert(Input.getActionName(jump) == "Jump" and Input.getActionType(move) == "Axis2D")
      assert(Input.getActionContext("Pause") == "Menu")
      assert(Input.isActionDown(jump) and Input.wasActionPressed("Jump"))
      assert(Input.isActionDown(jump, 1) and not Input.isActionDown(jump, 2))
      assert(Input.getActionValue(jump) == 1)
      assert(Input.getActionValue2D(move) == Vector2(0, 1))

      local out = Vector2()
      assert(rawequal(Input.getActionValue2D(move, out), out) and out == Vector2(0, 1))
      assert(rawequal(Input.getActionValue2D(move, 1, out), out))
    )lua"));
    REQUIRE_FALSE(scripts.runString("Input.isActionDown('Nope')"));
    REQUIRE_FALSE(scripts.runString("Input.isActionDown(99)"));
    REQUIRE_FALSE(scripts.runString("Input.isActionDown('Jump', 5)"));

    frame({key(InputKey::Space, false)});
    REQUIRE(scripts.runString(R"lua(
      assert(not Input.isActionDown("Jump") and Input.wasActionReleased("Jump"))
    )lua"));
  }

  SECTION("Contexts")
  {
    REQUIRE(scripts.runString(R"lua(
      Input.setContextEnabled("Menu", false)
      assert(not Input.isContextEnabled("Menu") and Input.isContextEnabled("Default"))
    )lua"));
    frame({key(InputKey::P, true)});
    REQUIRE(scripts.runString("assert(not Input.isActionDown('Pause'))"));
  }

  SECTION("Keys read as up while game input is off")
  {
    frame({key(InputKey::A, true),
           DisplayEvent(MouseMoveEvent{.x = 5.0f, .y = 6.0f, .deltaX = 2.0f, .deltaY = 0.0f})});
    REQUIRE(scripts.runString(R"lua(
      assert(Key.A and Key.GamepadSouth and Key.Nope == nil)
      assert(Input.isKeyDown(Key.A) and Input.isKeyDown("a") and Input.wasKeyPressed(Key.A))
      assert(not Input.isKeyDown(Key.B))
      assert(Input.getKeyName(Key.A) == "A")
      assert(Input.getMouseDelta().x == 2 and Input.getMousePosition() == Vector2(5, 6))
      assert(Input.isGameInputEnabled())
    )lua"));
    REQUIRE_FALSE(scripts.runString("Input.isKeyDown('Banana')"));

    input.setGameInputEnabled(false);
    REQUIRE(scripts.runString(R"lua(
      assert(not Input.isKeyDown(Key.A) and not Input.wasKeyPressed(Key.A))
      assert(Input.getMouseDelta() == Vector2(0, 0) and Input.getAxis(Key.MouseX) == 0)
      assert(Input.getMousePosition() == Vector2(5, 6))
    )lua"));
    frame({});
    REQUIRE(scripts.runString("assert(not Input.isActionDown('Move'))"));
  }

  SECTION("Gamepads count from 1")
  {
    frame({DisplayEvent(GamepadConnectionEvent{
               .slot = 1, .bConnected = true, .type = GamepadType::PlayStation}),
           DisplayEvent(GamepadButtonEvent{
               .slot = 1, .button = InputKey::GamepadSouth, .bDown = true})});
    REQUIRE(scripts.runString(R"lua(
      assert(Input.isGamepadConnected(2) and not Input.isGamepadConnected(1))
      assert(Input.getGamepadType(2) == "PlayStation" and Input.getLastGamepad() == 2)
      assert(Input.isKeyDown(Key.GamepadSouth, 2) and not Input.isKeyDown(Key.GamepadSouth, 1))
      assert(Input.isActionDown("Jump"))
    )lua"));
    REQUIRE_FALSE(scripts.runString("Input.isGamepadConnected(9)"));
  }

  SECTION("Rebinding")
  {
    REQUIRE(scripts.runString(R"lua(
      local jump = Input.findAction("Jump")
      local keys = Input.getBindings(jump)
      assert(#keys == 2 and keys[1] == "Space" and keys[2] == "GamepadSouth")
      assert(Input.setBinding(jump, 1, "J"))
      assert(Input.setBinding(jump, 3, "K"))
      assert(#Input.getBindings(jump) == 3 and Input.getBindings(jump)[1] == "J")
      assert(not Input.setBinding("Move", 1, "W"))
      assert(Input.removeBinding(jump, 3) and not Input.removeBinding(jump, 9))
      local users = Input.findActionsUsingKey(Key.J)
      assert(#users == 1 and users[1] == jump)
      Input.resetBindings(jump)
      assert(Input.getBindings(jump)[1] == "Space")
      assert(#Input.getDefaultBindings("Move") == 4)
    )lua"));
    REQUIRE_FALSE(scripts.runString("Input.setBinding('Jump', 1, 'Banana')"));
    REQUIRE_FALSE(scripts.runString("Input.setBinding('Jump', 5, 'J')"));
  }

  SECTION("A captured key goes to the function only")
  {
    REQUIRE(scripts.runString(R"lua(
      Captured = nil
      Input.captureNextKey(function(key) Captured = key end)
      assert(Input.isCapturingKey())
    )lua"));
    frame({key(InputKey::Space, true)});
    REQUIRE(scripts.runString(R"lua(
      assert(Captured == Key.Space and not Input.isCapturingKey())
      assert(not Input.isActionDown("Jump"))
    )lua"));
    // Still held, the captured key does not reach its action until it is released.
    frame({});
    REQUIRE_FALSE(input.isActionDown(input.findAction("Jump")));
    frame({key(InputKey::Space, false)});
    frame({key(InputKey::Space, true)});
    REQUIRE(input.wasActionPressed(input.findAction("Jump")));

    // Stop resets the game state, which drops a capture still waiting.
    REQUIRE(scripts.runString(R"lua(
      Captured = nil
      Input.captureNextKey(function(key) Captured = key end)
    )lua"));
    input.resetGameState();
    frame({key(InputKey::J, true)});
    REQUIRE(scripts.runString("assert(Captured == nil and not Input.isCapturingKey())"));

    // An error in the function is logged and the capture is over.
    REQUIRE(scripts.runString("Input.captureNextKey(function() error('boom') end)"));
    frame({key(InputKey::K, true)});
    REQUIRE_FALSE(input.isCapturingKey());
  }

  SECTION("Cursor and players")
  {
    REQUIRE(scripts.runString(R"lua(
      Input.setCursorMode("Hidden")
      assert(Input.getCursorMode() == "Hidden")
      Input.setPlayerCount(2)
      Input.setPlayerDevices(2, false, {1})
      assert(Input.getPlayerCount() == 2)
    )lua"));
    REQUIRE_FALSE(scripts.runString("Input.setCursorMode('Sideways')"));
    REQUIRE_FALSE(scripts.runString("Input.setPlayerCount(9)"));
    input.setCursorMode(CursorMode::Normal);
  }
}

TEST_CASE("chCore - Script definitions")
{
  ScriptManager scripts(true);
  const ScriptApiDescription* description = scripts.getApiDescription();
#if USING(CH_EDITOR)
  REQUIRE(description);
  const String text = description->writeDefinitions();
  const auto contains = [&text](const ANSICHAR* part) {
    INFO(part);
    CHECK(text.find(part) != String::npos);
  };

  SECTION("Value types")
  {
    contains("---@meta\n");
    contains("---@class Vector3\n");
    contains("---@field x number\n");
    contains("---@field UP Vector3\n");
    contains("---@operator add(Vector3): Vector3\n");
    contains("---@operator mul(number): Vector3\n");
    contains("---@operator unm: Vector3\n");
    contains("---@overload fun(): Vector3\n");
    contains("---@overload fun(x: number, y: number, z: number): Vector3\nVector3 = {}\n");
    contains("---@param other Vector3\n---@return number\nfunction Vector3:dot(other) end\n");
    contains("---@return Vector3\nfunction Vector3:normalize() end\n");
    contains("function Vector3.Lerp(a, b, alpha) end\n");
    contains("---@param degrees number\n---@return Quaternion\n"
             "function Quaternion.FromAxisAngle(axis, degrees) end\n");
    contains("---@field pitch number\n");
    contains("---@class Color\n");
  }

  SECTION("Handles and libraries")
  {
    contains("---@return string\nfunction GameObject:getName() end\n");
    contains("---@return GameObject?\nfunction GameObject:getParent() end\n");
    contains("---@param position Vector3\n"
             "---@overload fun(self: Transform, x: number, y: number, z: number)\n"
             "function Transform:setLocalPosition(position) end\n");
    contains("---@param out? Vector3\n---@return Vector3\n"
             "function Transform:getWorldPosition(out) end\n");
    contains("function Scene.find(name) end\n");
    contains("---@param player? integer\n---@return boolean\n"
             "function Input.isActionDown(action, player) end\n");
    contains("---@param callback fun(key: Key)\nfunction Input.captureNextKey(callback) end\n");
    contains("---@alias InputAction integer|string\n");
    contains("---@enum Key\nKey = {\n");
    contains("  W = ");
    contains("---@field deltaTime number\n");
    contains("---@param ... any\nfunction Log.info(...) end\n");
    contains("---@class Script\n---@field gameObject GameObject\n");
    CHECK(text.find("Script = {}") == String::npos);
  }

  SECTION("Every function has a signature and the file is Lua")
  {
    for (const UniquePtr<ScriptApiType>& type : description->getTypes()) {
      for (const ScriptApiFunction& function : type->functions) {
        INFO(type->name << "." << function.name);
        CHECK_FALSE(function.signatures.empty());
      }
    }
    lua_State* state = scripts.getState();
    REQUIRE(luaL_loadbufferx(state, text.data(), text.size(), "=definitions", "t") == LUA_OK);
    lua_pop(state, 1);
  }

  SECTION("Signatures")
  {
    ScriptApiSignature signature;
    REQUIRE(ScriptApiDescription::parseSignature(
        " fun(a: number, b?: fun(k: Key): string): boolean, integer ", signature));
    CHECK(signature.parameters == "a: number, b?: fun(k: Key): string");
    CHECK(signature.results == "boolean, integer");
    REQUIRE(ScriptApiDescription::parseSignature("fun()", signature));
    CHECK(signature.parameters.empty());
    CHECK(signature.results.empty());
    CHECK_FALSE(ScriptApiDescription::parseSignature("function(a)", signature));
    CHECK_FALSE(ScriptApiDescription::parseSignature("fun(a", signature));
    CHECK_FALSE(ScriptApiDescription::parseSignature("fun(a) number", signature));
  }

  SECTION("LuaLS configuration")
  {
    const String config = ScriptApiDescription::writeLuaLsConfig("Saved/ScriptDefinitions");
    CHECK(config.find("\"workspace.library\": [\"Saved/ScriptDefinitions\"]") !=
          String::npos);
    CHECK(config.find("\"io\": \"disable\"") != String::npos);
  }
#else
  CHECK_FALSE(description);
#endif // USING(CH_EDITOR)
}
