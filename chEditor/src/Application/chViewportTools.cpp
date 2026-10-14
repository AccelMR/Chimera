/************************************************************************/
/**
 * @file chViewportTools.cpp
 * @author AccelMR
 * @date 2026/10/13
 * @brief Selection, transform gizmos, light gizmos and the view cube of the viewport.
 */
/************************************************************************/

#include "chViewportTools.h"

#include "chBox.h"
#include "chCamera.h"
#include "chEditorCamera.h"
#include "chEditorScene.h"
#include "chEditorSelection.h"
#include "chGameObject.h"
#include "chLightItem.h"
#include "chMath.h"
#include "chMatrix4.h"
#include "chModelComponent.h"
#include "chQuaternion.h"
#include "chRay.h"
#include "chScene.h"
#include "chTransform.h"
#include "chVector2.h"

#include "imgui.h"
#include "ImGuizmo.h"

namespace chEngineSDK {

namespace {
// Below this many pixels of movement a press and release is a click, not a drag.
constexpr float kClickMaxDragPixels = 4.0f;
// In font heights, so the icons follow the editor font size.
constexpr float kLightIconRadius = 0.45f;
// A click this close to a light icon selects the light.
constexpr float kLightIconPickRadius = 0.7f;
constexpr uint32 kCircleSegments = 48;
// Fraction of the camera distance the directional light arrow is long, so it keeps its
// size on screen.
constexpr float kArrowScreenScale = 0.15f;
constexpr float kViewCubeSize = 128.0f;
constexpr float kViewportMargin = 8.0f;
// The camera never looks straight up or down (it would lose its yaw); the view cube's top
// and bottom views stop at this pitch.
constexpr float kMaxViewCubePitchDegrees = 89.0f;
// Lights have no bounds; F frames this much around them.
constexpr float kLightFocusExtent = 1.0f;

constexpr ImU32 kShapeColor = IM_COL32(255, 230, 120, 255);
constexpr ImU32 kInnerConeColor = IM_COL32(255, 230, 120, 110);
constexpr ImU32 kIconOutlineColor = IM_COL32(20, 20, 20, 255);
// Light enough to show on a dark background.
constexpr ImU32 kIconRayColor = IM_COL32(230, 230, 230, 255);
constexpr ImU32 kSelectedIconOutlineColor = IM_COL32(255, 200, 40, 255);

// The view cube thinks Y is up, the engine Z. Its world maps to the engine's with this
// rotation (its X, Y, Z are the engine's Y, Z, X), so it turns the camera around Z.
const Matrix4 kViewCubeToEngine(0.0f, 1.0f, 0.0f, 0.0f,
                                0.0f, 0.0f, 1.0f, 0.0f,
                                1.0f, 0.0f, 0.0f, 0.0f,
                                0.0f, 0.0f, 0.0f, 1.0f);

NODISCARD bool
toScreen(const Camera& camera, const ImGuiViewport& viewport, const Vector3& world,
         ImVec2& outPoint)
{
  Vector2 screen;
  if (!camera.worldToScreenPoint(world, screen)) {
    return false;
  }
  outPoint = ImVec2(viewport.Pos.x + screen.x * viewport.Size.x,
                    viewport.Pos.y + screen.y * viewport.Size.y);
  return true;
}

void
drawWorldLine(ImDrawList& drawList, const Camera& camera, const ImGuiViewport& viewport,
              const Vector3& from, const Vector3& to, ImU32 color)
{
  ImVec2 screenFrom;
  ImVec2 screenTo;
  if (toScreen(camera, viewport, from, screenFrom) &&
      toScreen(camera, viewport, to, screenTo)) {
    drawList.AddLine(screenFrom, screenTo, color, 1.5f);
  }
}

// axisU and axisV are unit length and perpendicular; they span the circle's plane.
void
drawWorldCircle(ImDrawList& drawList, const Camera& camera, const ImGuiViewport& viewport,
                const Vector3& center, const Vector3& axisU, const Vector3& axisV,
                float radius, ImU32 color)
{
  Vector3 previous = center + axisU * radius;
  for (uint32 i = 1; i <= kCircleSegments; ++i) {
    const float angle = Math::TWO_PI * static_cast<float>(i) / kCircleSegments;
    float sine = 0.0f;
    float cosine = 0.0f;
    Math::sinCos(angle, sine, cosine);
    const Vector3 point = center + (axisU * cosine + axisV * sine) * radius;
    drawWorldLine(drawList, camera, viewport, previous, point, color);
    previous = point;
  }
}

// Two unit axes perpendicular to direction and to each other.
void
makePerpendicularAxes(const Vector3& direction, Vector3& outU, Vector3& outV)
{
  const Vector3& helper =
      Math::abs(direction.dot(Vector3::UP)) < 0.99f ? Vector3::UP : Vector3::FORWARD;
  outU = direction.cross(helper).getNormalized();
  outV = direction.cross(outU);
}

NODISCARD ImU32
toIconColor(const LinearColor& color)
{
  // The light color includes the intensity; the icon shows only its hue.
  const float brightest = Math::max(color.r, Math::max(color.g, color.b));
  if (brightest <= Math::SMALL_NUMBER) {
    return IM_COL32(0, 0, 0, 255);
  }
  return ImGui::ColorConvertFloat4ToU32(
      ImVec4(color.r / brightest, color.g / brightest, color.b / brightest, 1.0f));
}

// The selection holds an SPtr, so a raw pointer is turned back into the one its parent or
// the scene keeps.
NODISCARD SPtr<GameObject>
findShared(const Scene& scene, const GameObject& gameObject)
{
  const GameObject* parent = gameObject.getParent();
  const Vector<SPtr<GameObject>>& siblings =
      parent ? parent->getChildren() : scene.getRootGameObjects();
  for (const SPtr<GameObject>& sibling : siblings) {
    if (sibling.get() == &gameObject) {
      return sibling;
    }
  }
  return nullptr;
}

} // namespace

/*
 */
ViewportTools::ViewportTools(EditorCamera& editorCamera)
  : m_editorCamera(editorCamera)
{}

/*
 */
void
ViewportTools::beginFrame()
{
  ImGuizmo::BeginFrame();
}

/*
 */
void
ViewportTools::render(Scene& scene)
{
  const Camera& camera = m_editorCamera.getCamera();

  handleShortcuts();
  renderToolbar();
  renderLights(camera);
  manipulateSelection(scene, camera);
  renderViewCube();
  handleClickSelection(scene, camera);
}

/*
 */
bool
ViewportTools::isUsingMouse() const
{
  return ImGuizmo::IsUsing() || ImGuizmo::IsUsingViewManipulate();
}

/*
 */
void
ViewportTools::handleShortcuts()
{
  // A panel keeps the keyboard focus after a click on it, so the keys also work while the
  // mouse is over the scene, unless text is being typed. With the right button held the
  // same keys fly the camera.
  const ImGuiIO& io = ImGui::GetIO();
  const bool bOverScene = !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
  if (io.WantTextInput || (io.WantCaptureKeyboard && !bOverScene) ||
      ImGui::IsMouseDown(ImGuiMouseButton_Right) || isUsingMouse()) {
    return;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_W, false)) {
    m_operation = Operation::Translate;
  }
  if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
    m_operation = Operation::Rotate;
  }
  if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
    m_operation = Operation::Scale;
  }
  if (ImGui::IsKeyPressed(ImGuiKey_F, false)) {
    focusSelection();
  }
  if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
    EditorSelection::setSelectedGameObject(nullptr);
  }
}

/*
 */
void
ViewportTools::renderToolbar()
{
  constexpr ImGuiWindowFlags kFlags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
      ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove;

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
                                 viewport->WorkPos.y + kViewportMargin),
                          ImGuiCond_Always, ImVec2(0.5f, 0.0f));
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::SetNextWindowBgAlpha(0.75f);
  if (!ImGui::Begin("##ViewportToolbar", nullptr, kFlags)) {
    ImGui::End();
    return;
  }

  const auto operationButton = [this](const ANSICHAR* label, Operation operation) {
    const bool bActive = m_operation == operation;
    if (bActive) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }
    if (ImGui::Button(label)) {
      m_operation = operation;
    }
    if (bActive) {
      ImGui::PopStyleColor();
    }
  };
  operationButton("Move (W)", Operation::Translate);
  ImGui::SameLine();
  operationButton("Rotate (E)", Operation::Rotate);
  ImGui::SameLine();
  operationButton("Scale (R)", Operation::Scale);

  ImGui::SameLine();
  ImGui::TextDisabled("|");
  ImGui::SameLine();
  // Scale always works along the object's own axes.
  ImGui::BeginDisabled(m_operation == Operation::Scale);
  if (ImGui::Button(m_bWorldSpace && m_operation != Operation::Scale ? "World###Space"
                                                                     : "Local###Space")) {
    m_bWorldSpace = !m_bWorldSpace;
  }
  ImGui::EndDisabled();

  ImGui::SameLine();
  ImGui::Checkbox("Snap", &m_bSnap);
  if (m_bSnap) {
    ImGui::SameLine();
    ImGui::SetNextItemWidth(70.0f);
    switch (m_operation) {
      case Operation::Translate:
        ImGui::DragFloat("##SnapValue", &m_translationSnap, 0.05f, 0.01f, 1000.0f, "%.2f");
        break;
      case Operation::Rotate:
        ImGui::DragFloat("##SnapValue", &m_rotationSnapDegrees, 0.5f, 1.0f, 180.0f,
                         "%.0f deg");
        break;
      case Operation::Scale:
        ImGui::DragFloat("##SnapValue", &m_scaleSnap, 0.01f, 0.01f, 10.0f, "%.2f");
        break;
    }
  }

  ImGui::SameLine();
  ImGui::TextDisabled("|");
  ImGui::SameLine();
  renderCameraSettings();
  ImGui::End();
}

/*
 */
void
ViewportTools::renderCameraSettings()
{
  // Saved once an edit ends instead of on every frame of a drag.
  bool bSave = false;
  const auto speedField = [this, &bSave](const ANSICHAR* label) {
    float speed = m_editorCamera.getMoveSpeed();
    // Logarithmic, because useful speeds go from a few centimeters to hundreds of meters.
    if (ImGui::DragFloat(label, &speed, 0.01f, EditorCamera::MIN_MOVE_SPEED,
                         EditorCamera::MAX_MOVE_SPEED, "Speed %.2f",
                         ImGuiSliderFlags_Logarithmic)) {
      m_editorCamera.setMoveSpeed(speed);
    }
    bSave |= ImGui::IsItemDeactivatedAfterEdit();
  };

  ImGui::SetNextItemWidth(130.0f);
  speedField("##CameraSpeed");
  ImGui::SetItemTooltip("Camera speed; the mouse wheel changes it while flying");
  ImGui::SameLine();
  if (ImGui::Button("Camera")) {
    ImGui::OpenPopup("##CameraSettings");
  }

  if (ImGui::BeginPopup("##CameraSettings")) {
    ImGui::SetNextItemWidth(160.0f);
    speedField("##PopupCameraSpeed");

    float sensitivity = m_editorCamera.getLookSensitivity();
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::SliderFloat("Look Sensitivity", &sensitivity, 0.01f, 1.0f, "%.2f deg/px")) {
      m_editorCamera.setLookSensitivity(sensitivity);
    }
    bSave |= ImGui::IsItemDeactivatedAfterEdit();

    float sprint = m_editorCamera.getSprintMultiplier();
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::SliderFloat("Sprint (Shift)", &sprint, 1.0f, 10.0f, "x%.1f")) {
      m_editorCamera.setSprintMultiplier(sprint);
    }
    bSave |= ImGui::IsItemDeactivatedAfterEdit();

    ImGui::Separator();
    float fieldOfView = m_editorCamera.getFieldOfView();
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::SliderFloat("Field of View", &fieldOfView, EditorCamera::MIN_FIELD_OF_VIEW,
                           EditorCamera::MAX_FIELD_OF_VIEW, "%.0f deg")) {
      m_editorCamera.setFieldOfView(fieldOfView);
    }
    ImGui::SetItemTooltip("Horizontal field of view");
    bSave |= ImGui::IsItemDeactivatedAfterEdit();

    // Logarithmic, because the planes go from millimeters to kilometers. Each range stops
    // at the other plane, so the far plane always stays beyond the near one.
    float nearPlane = m_editorCamera.getNearPlane();
    float farPlane = m_editorCamera.getFarPlane();
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::DragFloat("Near Plane", &nearPlane, 0.001f, EditorCamera::MIN_NEAR_PLANE,
                         farPlane / EditorCamera::MIN_FAR_TO_NEAR_RATIO, "%.3f",
                         ImGuiSliderFlags_Logarithmic)) {
      m_editorCamera.setClipPlanes(nearPlane, farPlane);
    }
    bSave |= ImGui::IsItemDeactivatedAfterEdit();

    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::DragFloat("Far Plane", &farPlane, 1.0f,
                         nearPlane * EditorCamera::MIN_FAR_TO_NEAR_RATIO,
                         EditorCamera::MAX_FAR_PLANE, "%.1f", ImGuiSliderFlags_Logarithmic)) {
      m_editorCamera.setClipPlanes(nearPlane, farPlane);
    }
    ImGui::SetItemTooltip("Far planes much farther than the near one lose depth precision");
    bSave |= ImGui::IsItemDeactivatedAfterEdit();

    if (ImGui::Button("Reset to Defaults")) {
      m_editorCamera.resetSettings();
    }

    ImGui::Separator();
    ImGui::TextDisabled("Hold the right mouse button to fly:");
    ImGui::TextDisabled("WASD move, E up, Q down, Shift faster,");
    ImGui::TextDisabled("mouse wheel changes the speed.");
    ImGui::EndPopup();
  }

  if (bSave) {
    m_editorCamera.saveSettings();
  }
}

/*
 */
void
ViewportTools::renderLights(const Camera& camera)
{
  // The scene is under every window, so the lights draw behind them too.
  const ImGuiViewport& viewport = *ImGui::GetMainViewport();
  ImDrawList& drawList = *ImGui::GetBackgroundDrawList(ImGui::GetMainViewport());
  const GameObject* selected = EditorSelection::getSelectedGameObject().get();

  for (const LightItem* light : m_visibleLights) {
    const bool bSelected = light->owner && light->owner == selected;
    if (bSelected) {
      const Vector3& position = light->position;
      const Vector3& direction = light->direction;
      Vector3 axisU;
      Vector3 axisV;
      makePerpendicularAxes(direction, axisU, axisV);

      switch (light->type) {
        case LightType::Directional: {
          const float length =
              camera.getPosition().distance(position) * kArrowScreenScale;
          const Vector3 tip = position + direction * length;
          const Vector3 headBase = tip - direction * (length * 0.25f);
          drawWorldLine(drawList, camera, viewport, position, tip, kShapeColor);
          for (const Vector3& side : {axisU, -axisU, axisV, -axisV}) {
            drawWorldLine(drawList, camera, viewport, tip,
                          headBase + side * (length * 0.1f), kShapeColor);
          }
          break;
        }
        case LightType::Point:
          drawWorldCircle(drawList, camera, viewport, position, Vector3::FORWARD,
                          Vector3::RIGHT, light->range, kShapeColor);
          drawWorldCircle(drawList, camera, viewport, position, Vector3::FORWARD,
                          Vector3::UP, light->range, kShapeColor);
          drawWorldCircle(drawList, camera, viewport, position, Vector3::RIGHT,
                          Vector3::UP, light->range, kShapeColor);
          break;
        case LightType::Spot: {
          // Each cone ends on the sphere of the range, as far as the light reaches.
          const auto drawCone = [&](float coneCos, ImU32 color, bool bWithEdges) {
            const float coneSin = Math::sqrt(Math::max(1.0f - coneCos * coneCos, 0.0f));
            const Vector3 center = position + direction * (light->range * coneCos);
            const float radius = light->range * coneSin;
            drawWorldCircle(drawList, camera, viewport, center, axisU, axisV, radius, color);
            if (bWithEdges) {
              for (const Vector3& side : {axisU, -axisU, axisV, -axisV}) {
                drawWorldLine(drawList, camera, viewport, position, center + side * radius,
                              color);
              }
            }
          };
          drawCone(light->outerConeCos, kShapeColor, true);
          drawCone(light->innerConeCos, kInnerConeColor, false);
          break;
        }
      }
    }

    ImVec2 icon;
    if (!toScreen(camera, viewport, light->position, icon)) {
      continue;
    }
    const ImU32 fill = toIconColor(light->color);
    const ImU32 outline = bSelected ? kSelectedIconOutlineColor : kIconOutlineColor;
    const float radius = ImGui::GetFontSize() * kLightIconRadius;
    const ImU32 rayColor = bSelected ? kSelectedIconOutlineColor : kIconRayColor;
    if (light->type == LightType::Directional) {
      // Rays around the disc, like a sun.
      for (uint32 i = 0; i < 8; ++i) {
        float sine = 0.0f;
        float cosine = 0.0f;
        Math::sinCos(Math::TWO_PI * static_cast<float>(i) / 8.0f, sine, cosine);
        drawList.AddLine(ImVec2(icon.x + cosine * radius * 1.3f,
                                icon.y + sine * radius * 1.3f),
                         ImVec2(icon.x + cosine * radius * 1.8f,
                                icon.y + sine * radius * 1.8f),
                         rayColor, 2.0f);
      }
    }
    else if (light->type == LightType::Spot) {
      ImVec2 aim;
      if (toScreen(camera, viewport,
                   light->position + light->direction *
                                         (camera.getPosition().distance(light->position) *
                                          kArrowScreenScale * 0.3f),
                   aim)) {
        drawList.AddLine(icon, aim, rayColor, 2.0f);
      }
    }
    drawList.AddCircleFilled(icon, radius, fill);
    drawList.AddCircle(icon, radius, outline, 0, bSelected ? 2.5f : 1.5f);
  }
}

/*
 */
void
ViewportTools::manipulateSelection(Scene& scene, const Camera& camera)
{
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGuizmo::SetRect(viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y);
  ImGuizmo::SetOrthographic(camera.getProjectionType() == CameraProjectionType::Orthographic);

  // Gathered here, while the frustum is current, for the light icons and picking.
  scene.gatherLights(camera.getFrustum(), m_visibleLights);

  GameObject* selected = EditorSelection::getSelectedGameObject().get();
  if (!selected || selected->getScene() != &scene) {
    return;
  }

  Transform& transform = selected->getTransform();
  Matrix4 world = transform.getWorldMatrix();

  ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
  float snap[3] = {0.0f, 0.0f, 0.0f};
  switch (m_operation) {
    case Operation::Translate:
      operation = ImGuizmo::TRANSLATE;
      snap[0] = snap[1] = snap[2] = m_translationSnap;
      break;
    case Operation::Rotate:
      operation = ImGuizmo::ROTATE;
      snap[0] = m_rotationSnapDegrees;
      break;
    case Operation::Scale:
      operation = ImGuizmo::SCALE;
      snap[0] = snap[1] = snap[2] = m_scaleSnap;
      break;
  }

  // Row vectors stored row by row, with the translation in elements 12 to 14, are the
  // layout ImGuizmo reads.
  const bool bChanged = ImGuizmo::Manipulate(
      camera.getViewMatrix().data(), camera.getProjectionMatrix().data(),
      operation,
      m_bWorldSpace ? ImGuizmo::WORLD : ImGuizmo::LOCAL, world[0], nullptr,
      m_bSnap ? snap : nullptr);
  if (!bChanged) {
    return;
  }

  // world = local * parentWorld, so the local matrix takes the inverse of the parent's.
  const GameObject* parent = selected->getParent();
  const Matrix4 local =
      parent ? world * parent->getTransform().getWorldMatrix().getInverseAffine() : world;
  Vector3 scale;
  Quaternion rotation;
  Vector3 position;
  if (local.decompose(scale, rotation, position)) {
    transform.setLocalPosition(position);
    transform.setLocalRotation(rotation);
    transform.setLocalScale(scale);
    EditorScene::markDirty();
  }
}

/*
 */
void
ViewportTools::renderViewCube()
{
  const Camera& camera = m_editorCamera.getCamera();
  const float distance =
      Math::max(camera.getPosition().distance(camera.getLookAt()), Math::KINDA_SMALL_NUMBER);

  // view takes engine positions, so the cube's positions go through kViewCubeToEngine
  // first.
  Matrix4 cubeView = kViewCubeToEngine * camera.getViewMatrix();
  const Matrix4 startView = cubeView;
  Matrix4 unusedModel = Matrix4::IDENTITY;

  // The cube colors its faces by its own axes; these keep X red, Y green and Z blue in
  // engine terms.
  ImGuizmo::Style& style = ImGuizmo::GetStyle();
  const ImVec4 colorX = style.Colors[ImGuizmo::DIRECTION_X];
  const ImVec4 colorY = style.Colors[ImGuizmo::DIRECTION_Y];
  const ImVec4 colorZ = style.Colors[ImGuizmo::DIRECTION_Z];
  style.Colors[ImGuizmo::DIRECTION_X] = colorY;
  style.Colors[ImGuizmo::DIRECTION_Y] = colorZ;
  style.Colors[ImGuizmo::DIRECTION_Z] = colorX;

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  const ImVec2 position(viewport->WorkPos.x + viewport->WorkSize.x - kViewCubeSize -
                            kViewportMargin,
                        viewport->WorkPos.y + kViewportMargin);
  ImGuizmo::ViewManipulate(cubeView[0], camera.getProjectionMatrix().data(),
                           ImGuizmo::TRANSLATE, ImGuizmo::WORLD, unusedModel[0], distance,
                           position, ImVec2(kViewCubeSize, kViewCubeSize), 0x10101010);

  style.Colors[ImGuizmo::DIRECTION_X] = colorX;
  style.Colors[ImGuizmo::DIRECTION_Y] = colorY;
  style.Colors[ImGuizmo::DIRECTION_Z] = colorZ;

  if (cubeView.nearEqual(startView, Math::SMALL_NUMBER)) {
    return;
  }

  // The inverse of a rotation is its transpose.
  Matrix4 engineToCube = kViewCubeToEngine;
  engineToCube.transpose();
  const Matrix4 cameraWorld = (engineToCube * cubeView).getInverseAffine();
  Vector3 forward(cameraWorld[2][0], cameraWorld[2][1], cameraWorld[2][2]);
  if (!forward.normalize()) {
    return;
  }

  // Straight down or up the camera would have no yaw left; the cube's up vector says which
  // way the screen top points, so the horizontal part comes from it.
  const float maxVertical = Math::sin(Radian(Degree(kMaxViewCubePitchDegrees)));
  if (Math::abs(forward.z) > maxVertical) {
    Vector3 horizontal(cameraWorld[1][0], cameraWorld[1][1], 0.0f);
    if (forward.z > 0.0f) {
      horizontal = -horizontal;
    }
    if (!horizontal.normalize()) {
      horizontal = Vector3::FORWARD;
    }
    const float horizontalLength = Math::sqrt(1.0f - maxVertical * maxVertical);
    forward = horizontal * horizontalLength +
              Vector3::UP * (forward.z > 0.0f ? maxVertical : -maxVertical);
  }

  // The cube orbits around the look at point, which stays where it is.
  const Vector3 lookAt = camera.getLookAt();
  m_editorCamera.setView(lookAt - forward * distance, lookAt);
}

/*
 */
void
ViewportTools::handleClickSelection(Scene& scene, const Camera& camera)
{
  const ImGuiIO& io = ImGui::GetIO();
  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    m_bClickStartedInScene = !io.WantCaptureMouse && !ImGuizmo::IsOver() &&
                             !ImGuizmo::IsViewManipulateHovered() && !isUsingMouse();
  }
  if (!ImGui::IsMouseReleased(ImGuiMouseButton_Left) || !m_bClickStartedInScene) {
    return;
  }
  m_bClickStartedInScene = false;
  if (io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] >
      kClickMaxDragPixels * kClickMaxDragPixels) {
    return;
  }

  const ImGuiViewport& viewport = *ImGui::GetMainViewport();
  const ImVec2 mouse = ImGui::GetMousePos();

  // Light icons first: they sit in front of what they light.
  GameObject* picked = nullptr;
  const float pickRadius = ImGui::GetFontSize() * kLightIconPickRadius;
  float nearestIcon = pickRadius * pickRadius;
  for (const LightItem* light : m_visibleLights) {
    ImVec2 icon;
    if (!light->owner || !toScreen(camera, viewport, light->position, icon)) {
      continue;
    }
    const float dx = icon.x - mouse.x;
    const float dy = icon.y - mouse.y;
    const float distanceSquared = dx * dx + dy * dy;
    if (distanceSquared <= nearestIcon) {
      nearestIcon = distanceSquared;
      picked = light->owner;
    }
  }

  if (!picked) {
    const Vector2 screenPoint((mouse.x - viewport.Pos.x) / viewport.Size.x,
                              (mouse.y - viewport.Pos.y) / viewport.Size.y);
    SceneRayHit hit;
    if (scene.raycast(camera.screenToWorldRay(screenPoint), hit)) {
      picked = hit.gameObject;
    }
  }

  EditorSelection::setSelectedGameObject(picked ? findShared(scene, *picked) : nullptr);
}

/*
 */
void
ViewportTools::focusSelection()
{
  const SPtr<GameObject>& selected = EditorSelection::getSelectedGameObject();
  if (!selected) {
    return;
  }

  // A model split into parts has its meshes on the children.
  AABox bounds(Vector3::ZERO, Vector3::ZERO);
  if (ModelComponent::getHierarchyWorldBounds(*selected, bounds)) {
    m_editorCamera.focus(bounds);
    return;
  }
  const Vector3 position = selected->getTransform().getWorldPosition();
  const Vector3 extent(kLightFocusExtent, kLightFocusExtent, kLightFocusExtent);
  m_editorCamera.focus(AABox(position - extent, position + extent));
}

} // namespace chEngineSDK
