-- chimera-uuid: a1490e4f-44e8-4b1f-a570-9d6b572b8825
-- Floats its object up and down around where it started, using Time.time.
---@class Bobber : Script
---@field height number
---@field speed number
local Bobber = {}

Bobber.properties = {
  { "height", 0.75, min = 0, max = 5, tooltip = "How far up it goes" },
  { "speed", 2, tooltip = "Radians of the wave per second" },
}

function Bobber:onStart()
  self.start = self.transform:getLocalPosition()
  -- Each object starts at another point of the wave.
  self.phase = math.random() * math.pi * 2
end

function Bobber:update(dt)
  local s = self.start
  local z = s.z + self.height * (0.5 + 0.5 * math.sin(Time.time * self.speed + self.phase))
  self.transform:setLocalPosition(s.x, s.y, z)
end

return Bobber
