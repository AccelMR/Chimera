-- chimera-uuid: 710a2454-6aca-4b0e-9de9-b9c8e573fe0f
-- Moves its object with the Move action (WASD, arrows, left stick) along the ground,
-- relative to where the camera looks. Sprint runs faster and Jump hops with gravity.
local Input = Input

local PlayerController = {}

local WALK_SPEED = 4
local SPRINT_SPEED = 9
local JUMP_SPEED = 7
local GRAVITY = -20
local CAMERA_NAME = "Camera"

function PlayerController:onStart()
  self.move = Input.findAction("Move")
  self.jump = Input.findAction("Jump")
  self.sprint = Input.findAction("Sprint")
  self.camera = Scene.find(CAMERA_NAME)

  -- Values kept between frames, so update makes no new vectors.
  self.position = self.transform:getWorldPosition()
  self.groundZ = self.position.z
  self.verticalSpeed = 0
  self.input = Vector2()
  self.forward = Vector3()
end

function PlayerController:update(dt)
  local input = Input.getActionValue2D(self.move, self.input)
  local speed = Input.isActionDown(self.sprint) and SPRINT_SPEED or WALK_SPEED

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
    self.verticalSpeed = JUMP_SPEED
  end
  self.verticalSpeed = self.verticalSpeed + GRAVITY * dt
  p.z = p.z + self.verticalSpeed * dt
  if p.z < self.groundZ then
    p.z = self.groundZ
    self.verticalSpeed = 0
  end

  self.transform:setWorldPosition(p)
end

return PlayerController
