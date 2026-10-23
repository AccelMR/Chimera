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
local PlayerController = {}

PlayerController.properties = {
  { "walkSpeed", 4 },
  { "sprintSpeed", 9 },
  { "jumpSpeed", 7 },
  { "gravity", -20 },
  { "camera", type = "GameObject",
    tooltip = "Moves relative to it; the object named Camera when empty" },
}

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
end

return PlayerController
