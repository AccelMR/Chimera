/************************************************************************/
/**
 * @file chCoreTestMain.cpp
 * @author AccelMR
 * @date 2026/10/03
 * @brief Unit tests for chCore code that runs without a window or a GPU.
 */
/************************************************************************/
#include <cstddef>

#include "chBox.h"
#include "chCamera.h"
#include "chAngle.h"
#include "chGraphicsTypes.h"
#include "chIShader.h"
#include "chITexture.h"
#include "chITextureView.h"
#include "chMath.h"
#include "chMesh.h"
#include "chModel.h"
#include "chModelComponent.h"
#include "chQuaternion.h"
#include "chRay.h"
#include "chRenderGraph.h"
#include "chRotator.h"
#include "chScene.h"
#include "chShapeOverlap.h"
#include "chSphereBoxBounds.h"
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
  REQUIRE(VertexNormalTexCoord::getLayout().getVertexSize() == sizeof(VertexNormalTexCoord));
  REQUIRE(VertexGBuffer::getLayout().getVertexSize() == sizeof(VertexGBuffer));

  const VertexLayout layout = VertexNormalTexCoord::getLayout();
  const Vector<VertexAttributeDesc>& attributes = layout.getAttributes();
  REQUIRE(attributes.size() == 3);
  REQUIRE(attributes[0].offset == offsetof(VertexNormalTexCoord, position));
  REQUIRE(attributes[1].offset == offsetof(VertexNormalTexCoord, normal));
  REQUIRE(attributes[2].offset == offsetof(VertexNormalTexCoord, texCoord));

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
                            .vertexLayout = VertexLayout::createPositionNormalTexCoordLayout(),
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
  mesh->setVertexData(Vector<VertexNormalTexCoord>{
      {Vector3(1.0f, 0.0f, 0.0f), Vector3::UP, Vector2(0.0f, 0.0f)},
      {Vector3(-1.0f, 0.0f, 0.0f), Vector3::UP, Vector2(1.0f, 0.0f)},
      {Vector3(0.0f, 2.0f, 0.0f), Vector3::UP, Vector2(0.0f, 1.0f)}});
  mesh->setIndexData(Vector<uint16>{0, 1, 2});

  SPtr<Model> model = chMakeShared<Model>();
  model->createNode("Root")->addMesh(mesh);
  return model;
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
  ModelComponent& modelComponent = gameObject->addComponent<ModelComponent>(model);
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
    other->addComponent<ModelComponent>(createTriangleModel());
    REQUIRE(scene.getRenderItemCount() == 1);
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
