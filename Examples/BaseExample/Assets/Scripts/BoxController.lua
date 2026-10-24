-- chimera-uuid: 78dc343e-3c8d-4908-ab5b-8240f7daa46e
-- Drives the AG_Box animation graph from the keyboard: N steps the Speed parameter
-- (0, 1, ... maxSpeed, then 0 again) and B fires the Burst trigger.
---@class BoxController : Script
---@field maxSpeed number
local BoxController = {}

BoxController.properties = {
  { "maxSpeed", 2, tooltip = "Highest value N gives the Speed parameter" },
}

function BoxController:onStart()
  self.animator = self.gameObject:getAnimator()
  self.speed = 0
  self.warned = false
end

-- The graph is built in the editor and only runs once it is valid, so until then the
-- calls fail; the first failure is logged and the script keeps running.
function BoxController:tryGraph(call)
  local ok, err = pcall(call)
  if not ok and not self.warned then
    Log.warning("BoxController: " .. tostring(err))
    self.warned = true
  end
end

function BoxController:update(dt)
  if not self.animator then
    return
  end
  if Input.wasKeyPressed(Key.N) then
    self.speed = self.speed + 1
    if self.speed > self.maxSpeed then
      self.speed = 0
    end
    print("Speed = " .. self.speed)
    self:tryGraph(function() self.animator:setFloat("Speed", self.speed) end)
  end
  if Input.wasKeyPressed(Key.B) then
    print("Burst")
    self:tryGraph(function() self.animator:setTrigger("Burst") end)
  end
end

return BoxController
