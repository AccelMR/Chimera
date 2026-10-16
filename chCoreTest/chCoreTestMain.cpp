/************************************************************************/
/**
 * @file chCoreTestMain.cpp
 * @author AccelMR
 * @date 2026/10/03
 * @brief Unit tests for chCore code that runs without a window or a GPU.
 */
/************************************************************************/
#include <cstddef>
#include <cstring>

#include "chBox.h"
#include "chCamera.h"
#include "chAngle.h"
#include "chAssetFile.h"
#include "chAssetMeta.h"
#include "chComponentRegister.h"
#include "chConfigFile.h"
#include "chGraphicsTypes.h"
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
#include "chMeshSplit.h"
#include "chModel.h"
#include "chModelAsset.h"
#include "chModelComponent.h"
#include "chModelExpand.h"
#include "chPrefabAsset.h"
#include "chQuaternion.h"
#include "chRay.h"
#include "chRenderGraph.h"
#include "chRenderSettings.h"
#include "chRotator.h"
#include "chScene.h"
#include "chSceneHistory.h"
#include "chShadowRenderer.h"
#include "chShapeOverlap.h"
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
  model->createNode("Root")->addMesh(mesh);
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
  const SphereBoxBounds& bounds = model->getRootNodes()[0]->getMeshes()[0]->getBounds();
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
  REQUIRE(visible[0]->mesh == model->getRootNodes()[0]->getMeshes()[0].get());

  SECTION("Items follow the object and are culled behind the camera")
  {
    gameObject->getTransform().setLocalPosition(Vector3(-20.0f, 0.0f, 0.0f));
    scene.updateTransforms();
    REQUIRE(visible[0]->worldBounds.center.nearEqual(Vector3(-20.0f, 1.0f, 0.0f)));
    scene.gatherRenderItems(camera.getFrustum(), visible);
    REQUIRE(visible.empty());
    REQUIRE(modelComponent.getWorldBounds().getCenter().nearEqual(
        Vector3(-20.0f, 1.0f, 0.0f)));
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

  MemoryDataStream stream(4096);
  AssetFile::writeStart(stream, uuid, assetType, references);
  const SIZE_T referenceCountOffset = stream.tell() - sizeof(references) - sizeof(uint32);
  // Only the identity follows the header; the name and folder come from the file's place.
  REQUIRE(referenceCountOffset == sizeof(AssetFileHeader) + 2 * sizeof(chEngineSDK::UUID));
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
  String text;
  REQUIRE(AssetFile::readString(stream, text, 64));
  REQUIRE(text == "Slot");

  SECTION("A file without the magic number is rejected")
  {
    MemoryDataStream other(64);
    const uint32 junk[2] = {0x12345678, AssetFileHeader::FORMAT_VERSION};
    other.write(junk, sizeof(junk));
    other.seek(0);
    REQUIRE_FALSE(AssetFile::readInfo(other, info));
  }

  SECTION("A newer format version is rejected")
  {
    MemoryDataStream other(64);
    const uint32 header[2] = {AssetFileHeader::MAGIC, AssetFileHeader::FORMAT_VERSION + 1};
    other.write(header, sizeof(header));
    other.seek(0);
    REQUIRE_FALSE(AssetFile::readInfo(other, info));
  }

  SECTION("Format version 1 is still read, with its editor data")
  {
    // The 664-byte metadata block of version 1: two UUIDs, the creation time, then the type
    // name (32), engine version (16), name (64), imported path (256) and folder (256).
    Vector<uint8> legacy(AssetFile::LEGACY_METADATA_SIZE, 0);
    const uint64 creationTime = 1234;
    std::memcpy(legacy.data(), &uuid, sizeof(uuid));
    std::memcpy(legacy.data() + 16, &assetType, sizeof(assetType));
    std::memcpy(legacy.data() + 32, &creationTime, sizeof(creationTime));
    std::memcpy(legacy.data() + 72, "0.1.0", 5);
    std::memcpy(legacy.data() + 152, "C:/Art/Rock.gltf#Stone", 22);

    MemoryDataStream other(1024);
    const uint32 header[2] = {AssetFileHeader::MAGIC, 1};
    other.write(header, sizeof(header));
    other.write(legacy.data(), legacy.size());
    const uint32 noReferences = 0;
    other.write(&noReferences, sizeof(noReferences));
    other.seek(0);

    AssetFileInfo legacyInfo;
    REQUIRE(AssetFile::readInfo(other, legacyInfo));
    REQUIRE(legacyInfo.formatVersion == 1);
    REQUIRE(legacyInfo.uuid == uuid);
    REQUIRE(legacyInfo.assetType == assetType);
    REQUIRE(legacyInfo.legacyCreationTime == creationTime);
    REQUIRE(legacyInfo.legacyEngineVersion == "0.1.0");
    REQUIRE(legacyInfo.legacyImportedPath == "C:/Art/Rock.gltf#Stone");
    REQUIRE(AssetFile::readReferences(other, readReferences));
    REQUIRE(readReferences.empty());
  }

  SECTION("Broken lengths are rejected")
  {
    stream.seek(referenceCountOffset);
    const uint32 tooMany = AssetFile::MAX_REFERENCES + 1;
    stream.write(&tooMany, sizeof(tooMany));
    stream.seek(referenceCountOffset);
    REQUIRE_FALSE(AssetFile::readReferences(stream, readReferences));
    REQUIRE(readReferences.empty());

    stream.seek(referenceCountOffset + sizeof(uint32) + sizeof(references));
    REQUIRE_FALSE(AssetFile::readString(stream, text, 3));
  }
}

#if USING(CH_EDITOR)
TEST_CASE("chCore - Asset meta file")
{
  const Path project = FileSystem::absolutePath(Path("MetaTestProject"));
  ImportSettings settings;
  settings.srgb = false;
  settings.expandMode = ModelExpandMode::PerMesh;
  settings.splitParts = true;
  settings.autoAlphaMask = false;
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

  REQUIRE(AssetMeta::getPath(Path("/Game/Rock.chAss")) == Path("/Game/Rock.chAss.meta"));
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

  SECTION("Version 2 has no alpha data and gets the defaults")
  {
    // Version 2 ends where the alpha data of version 3 starts, so the same bytes with
    // version 2 written over the first ones read like an old file.
    stream.seek(0);
    const uint32 oldVersion = 2;
    stream.write(&oldVersion, sizeof(oldVersion));
    stream.seek(0);
    Material oldMaterial;
    REQUIRE(oldMaterial.deserialize(stream));
    REQUIRE(oldMaterial.getMetallicFactor() == 0.75f);
    REQUIRE(oldMaterial.getAlphaMode() == MaterialAlphaMode::Opaque);
    REQUIRE(oldMaterial.getAlphaCutoff() == 0.5f);
    REQUIRE_FALSE(oldMaterial.isTwoSided());
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
  const SPtr<Mesh>& mesh = model->getRootNodes()[0]->getMeshes()[0];
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
  model->createNode("Sword", model->getRootNodes()[0])->addMesh(secondMesh);

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

TEST_CASE("chCore - Model expand")
{
  // Root (x + 10, one mesh) > Child (y + 5, two meshes) and Root > Empty > Leaf (z + 3).
  // Every mesh is the test triangle, centered at (0, 1, 0) in its node.
  const auto createMesh = [](uint32 slot) {
    SPtr<Mesh> mesh = createTriangleModel()->getRootNodes()[0]->getMeshes()[0];
    mesh->setMaterialSlot(slot);
    return mesh;
  };
  const SPtr<Model> model = chMakeShared<Model>();
  ModelNode* rootNode =
      model->createNode("Root", TranslationMatrix(Vector3(10.0f, 0.0f, 0.0f)));
  rootNode->addMesh(createMesh(0));
  ModelNode* childNode =
      model->createNode("Child", TranslationMatrix(Vector3(0.0f, 5.0f, 0.0f)), rootNode);
  childNode->addMesh(createMesh(0));
  childNode->addMesh(createMesh(1));
  ModelNode* emptyNode = model->createNode("Empty", rootNode);
  model->createNode("Leaf", TranslationMatrix(Vector3(0.0f, 0.0f, 3.0f)), emptyNode)
      ->addMesh(createMesh(1));

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
  REQUIRE(ModelComponent::getHierarchyWorldBounds(*gameObject, wholeBounds));

  const auto findChild = [](const GameObject& parent, const String& name) -> GameObject* {
    for (const SPtr<GameObject>& child : parent.getChildren()) {
      if (child->getName() == name) {
        return child.get();
      }
    }
    return nullptr;
  };
  const auto partCenter = [](const GameObject& part) {
    return part.getComponent<ModelComponent>()->getWorldBounds().getCenter();
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
    REQUIRE(ModelComponent::getHierarchyWorldBounds(*gameObject, expandedBounds));
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
