-- chimera-uuid: 0bcd1907-8c4c-484d-82d5-c0091e99f43d
-- Streams a sublevel: loads it when the target gets close to this object and unloads it
-- when the target walks away (a little farther, so standing on the edge does not load and
-- unload it every frame). L loads or unloads it by hand, M opens another map.
-- GameState is a Lua global, so it survives Scene.openMap: the arena counts the visits.
---@class LevelStreamer : Script
---@field level string?
---@field arena string?
---@field target GameObject?
---@field radius number
---@field showZone boolean
local LevelStreamer = {}

LevelStreamer.properties = {
  { "level", type = "Scene", tooltip = "The sublevel this streams (listed by the map)" },
  { "arena", type = "Scene", tooltip = "The map M opens" },
  { "target", type = "GameObject", tooltip = "What walking close loads the level" },
  { "radius", 12, min = 1, max = 50, tooltip = "Distance that loads the level" },
  { "showZone", true, tooltip = "Draws the zone on the ground" },
}

-- Unloads only past radius times this.
local kUnloadMargin = 1.25

GameState = GameState or { visits = 0 }

function LevelStreamer:onStart()
  self.manual = false
  print("Map " .. Scene.getMapName() .. ", arena visits: " .. GameState.visits)
end

function LevelStreamer:update(dt)
  if not self.level then
    return
  end

  if Input.wasKeyPressed(Key.L) then
    -- From now on only L decides.
    self.manual = true
    if Scene.isLevelLoaded(self.level) then
      Scene.unloadLevel(self.level)
    else
      Scene.loadLevel(self.level)
    end
  end
  if Input.wasKeyPressed(Key.M) and self.arena then
    Scene.openMap(self.arena)
    return
  end

  local center = self.transform:getWorldPosition()
  if self.showZone then
    local loaded = Scene.isLevelLoaded(self.level)
    local color = loaded and Color(0.2, 1, 0.3, 1) or Color(1, 0.8, 0.2, 1)
    DebugDraw.circle(center, Vector3.UP, self.radius, color)
  end
  if self.manual or not self.target or Scene.isLevelLoading(self.level) then
    return
  end

  local distance = (self.target:getTransform():getWorldPosition() - center):length()
  local loaded = Scene.isLevelLoaded(self.level)
  if not loaded and distance < self.radius then
    Scene.loadLevel(self.level)
  elseif loaded and distance > self.radius * kUnloadMargin then
    Scene.unloadLevel(self.level)
  end
end

return LevelStreamer
