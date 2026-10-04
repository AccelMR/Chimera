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
#include "chDegree.h"
#include "chRadian.h"
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
  camera.updateMatrices();
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
  const Vector2 center = camera.worldToScreenPoint(Vector3::ZERO);
  REQUIRE(center.x == Approx(0.5f).margin(kTolerance));
  REQUIRE(center.y == Approx(0.5f).margin(kTolerance));

  const Vector2 right = camera.worldToScreenPoint(Vector3(0.0f, 1.0f, 0.0f));
  REQUIRE(right.x > 0.5f);

  const Vector2 up = camera.worldToScreenPoint(Vector3(0.0f, 0.0f, 1.0f));
  REQUIRE(up.y < 0.5f);
}

TEST_CASE("chCore - Camera frustum")
{
  const Camera camera = createTestCamera();

  REQUIRE(camera.isPointInFrustum(Vector3::ZERO));
  REQUIRE(camera.isPointInFrustum(Vector3(90.0f, 0.0f, 0.0f)));

  // Behind the camera, closer than the near plane and past the far plane.
  REQUIRE_FALSE(camera.isPointInFrustum(Vector3(-10.0f, 0.0f, 0.0f)));
  REQUIRE_FALSE(camera.isPointInFrustum(Vector3(-4.95f, 0.0f, 0.0f)));
  REQUIRE_FALSE(camera.isPointInFrustum(Vector3(100.0f, 0.0f, 0.0f)));

  // Far to a side, outside the 90 degree field of view.
  REQUIRE_FALSE(camera.isPointInFrustum(Vector3(0.0f, 50.0f, 0.0f)));
  REQUIRE_FALSE(camera.isPointInFrustum(Vector3(0.0f, 0.0f, 50.0f)));

  // A sphere outside the side plane but close enough to touch it is still visible.
  REQUIRE(camera.isSphereInFrustum(Vector3(0.0f, 12.0f, 0.0f), 5.0f));
  REQUIRE_FALSE(camera.isSphereInFrustum(Vector3(0.0f, 50.0f, 0.0f), 1.0f));

  const AABox visibleBox(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));
  REQUIRE(camera.isBoxInFrustum(visibleBox));

  const AABox boxBehind(Vector3(-20.0f, -1.0f, -1.0f), Vector3(-15.0f, 1.0f, 1.0f));
  REQUIRE_FALSE(camera.isBoxInFrustum(boxBehind));
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
}
