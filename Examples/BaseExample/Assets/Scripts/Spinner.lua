-- chimera-uuid: 0eda3483-e362-4a89-ad86-f2afb2337093
-- Turns its object around the up axis. Each object can set its own speed in the Inspector.
---@class Spinner : Script
---@field speed number
local Spinner = {}

Spinner.properties = {
  { "speed", 90, tooltip = "Degrees per second" },
}

function Spinner:update(dt)
  self.transform:rotateLocal(0, self.speed * dt, 0)
end

return Spinner
