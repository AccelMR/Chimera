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
#include "chRay.h"
#include "chRotator.h"
#include "chShapeOverlap.h"
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
