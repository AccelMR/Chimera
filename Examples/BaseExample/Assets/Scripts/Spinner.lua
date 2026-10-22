-- chimera-uuid: 0eda3483-e362-4a89-ad86-f2afb2337093
-- Turns its object around the up axis. Change SPEED (degrees per second) to try a hot
-- reload: the editor reads scripts again every time Play starts.
local Spinner = {}

local SPEED = 90

function Spinner:update(dt)
  self.transform:rotateLocal(0, SPEED * dt, 0)
end

return Spinner
