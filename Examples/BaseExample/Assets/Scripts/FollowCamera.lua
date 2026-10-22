-- Keeps its object (the camera) orbiting a target. Hold the right mouse button and move the
-- mouse, or use the right stick, to turn around it (Look action); the wheel or the triggers
-- zoom (Zoom action).
local Input = Input

local FollowCamera = {}

local TARGET_NAME = "Sphere"
local MIN_PITCH, MAX_PITCH = -10, 80
local MIN_DISTANCE, MAX_DISTANCE = 2, 40
local ZOOM_STEP = 1.5
-- Where the camera starts: above the target, so the other shapes do not hide it.
local START_PITCH = 35
local START_DISTANCE = 8

function FollowCamera:onStart()
  self.look = Input.findAction("Look")
  self.zoom = Input.findAction("Zoom")
  self.target = Scene.find(TARGET_NAME)
  if not self.target then
    Log.warning("FollowCamera: no object named", TARGET_NAME)
    return
  end

  -- Keeps the direction the camera already looks from, at the start height and distance.
  self.targetPosition = self.target:getTransform():getWorldPosition()
  local from = self.transform:getWorldPosition()
  local dx = self.targetPosition.x - from.x
  local dy = self.targetPosition.y - from.y
  self.yaw = math.deg(math.atan(dy, dx))
  self.pitch = START_PITCH
  self.distance = START_DISTANCE
  self.lookValue = Vector2()
  self.position = Vector3()
end

function FollowCamera:update(dt)
  if not self.target or not self.target:isValid() then
    return
  end

  -- Mouse look only while the right button is held, so the mouse stays free otherwise.
  if Input.isKeyDown(Key.MouseRight) or Input.getLastGamepad() then
    local look = Input.getActionValue2D(self.look, self.lookValue)
    self.yaw = self.yaw + look.x
    self.pitch = math.max(MIN_PITCH, math.min(MAX_PITCH, self.pitch - look.y))
  end
  local zoom = Input.getActionValue(self.zoom)
  self.distance = math.max(MIN_DISTANCE, math.min(MAX_DISTANCE,
                                                  self.distance - zoom * ZOOM_STEP))

  local target = self.target:getTransform():getWorldPosition(self.targetPosition)
  local yaw, pitch = math.rad(self.yaw), math.rad(self.pitch)
  local p = self.position
  p.x = target.x - math.cos(pitch) * math.cos(yaw) * self.distance
  p.y = target.y - math.cos(pitch) * math.sin(yaw) * self.distance
  p.z = target.z + math.sin(pitch) * self.distance
  self.transform:setWorldPosition(p)
  self.transform:lookAt(target)
end

return FollowCamera
