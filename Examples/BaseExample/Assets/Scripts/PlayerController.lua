-- chimera-uuid: 710a2454-6aca-4b0e-9de9-b9c8e573fe0f
-- Moves its object with the Move action (WASD, arrows, left stick) along the ground,
-- relative to where the camera looks. Sprint runs faster and Jump hops with gravity.
local Input = Input

---@class PlayerController : Script
---@field walkSpeed number
---@field sprintSpeed number
---@field jumpSpeed number
---@field gravity number
---@field camera GameObject?
---@field showGroundRay boolean
local PlayerController = {}

PlayerController.properties = {
  { "walkSpeed", 4 },
  { "sprintSpeed", 9 },
  { "jumpSpeed", 7 },
  { "gravity", -20 },
  { "camera", type = "GameObject",
    tooltip = "Moves relative to it; the object named Camera when empty" },
  { "showGroundRay", true,
    tooltip = "Draws a ray down to the ground and the hit normal (debug lines only)" },
}

-- Made once and shared by every instance, so drawing makes no new values.
local kRayColor = Color(1, 1, 0, 1)
local kHitColor = Color(0, 1, 0, 1)
local kRayHeight = 3
local kRayLength = 20

function PlayerController:onStart()
  self.move = Input.findAction("Move")
  self.jump = Input.findAction("Jump")
  self.sprint = Input.findAction("Sprint")
  self.camera = self.camera or Scene.find("Camera")

  -- Values kept between frames, so update makes no new vectors.
  self.position = self.transform:getWorldPosition()
  self.groundZ = self.position.z
  self.verticalSpeed = 0
  self.input = Vector2()
  self.forward = Vector3()
  self.rayStart = Vector3()
  -- Scene.raycast fills this table again each frame instead of making a new one.
  self.hit = {}
end

-- Casts a ray down from above the player, skipping the player itself, and shows where it
-- lands. Packaged games draw nothing, but the raycast still runs, so it is behind the flag.
function PlayerController:drawGroundRay()
  local start = self.rayStart:copyFrom(self.position)
  start.z = start.z + kRayHeight
  local hit = Scene.raycast(start, Vector3.DOWN, kRayLength, self.gameObject, self.hit)
  if hit then
    DebugDraw.line(start, hit.point, kRayColor, true)
    DebugDraw.circle(hit.point, hit.normal, 0.8, kHitColor, true)
  else
    DebugDraw.ray(start, Vector3.DOWN, kRayLength, kRayColor, true)
  end
end

function PlayerController:update(dt)
  local input = Input.getActionValue2D(self.move, self.input)
  local speed = Input.isActionDown(self.sprint) and self.sprintSpeed or self.walkSpeed

  -- Forward and right of the camera, flat on the ground (X forward, Y right, Z up).
  local fx, fy = 1, 0
  if self.camera and self.camera:isValid() then
    local forward = self.camera:getTransform():getForward(self.forward)
    local length = math.sqrt(forward.x * forward.x + forward.y * forward.y)
    if length > 0.001 then
      fx, fy = forward.x / length, forward.y / length
    end
  end
  local rx, ry = -fy, fx

  local p = self.position
  p.x = p.x + (fx * input.y + rx * input.x) * speed * dt
  p.y = p.y + (fy * input.y + ry * input.x) * speed * dt

  local grounded = p.z <= self.groundZ
  if grounded and Input.wasActionPressed(self.jump) then
    self.verticalSpeed = self.jumpSpeed
  end
  self.verticalSpeed = self.verticalSpeed + self.gravity * dt
  p.z = p.z + self.verticalSpeed * dt
  if p.z < self.groundZ then
    p.z = self.groundZ
    self.verticalSpeed = 0
  end

  self.transform:setWorldPosition(p)

  if self.showGroundRay then
    self:drawGroundRay()
  end
end

return PlayerController
