-- chimera-uuid: cbe5cef4-9dfc-4c9a-ac8b-ce61b8913ef2
-- Lives in the Arena map: counts the visit in GameState (a Lua global, so it is still there
-- after Scene.openMap) and opens the way back with M.
---@class ArenaExit : Script
---@field back string?
local ArenaExit = {}

ArenaExit.properties = {
  { "back", type = "Scene", tooltip = "The map M goes back to" },
}

GameState = GameState or { visits = 0 }

function ArenaExit:onStart()
  GameState.visits = GameState.visits + 1
  print("Welcome to " .. Scene.getMapName() .. " (visit " .. GameState.visits .. "). M goes back.")
end

function ArenaExit:update(dt)
  if Input.wasKeyPressed(Key.M) and self.back then
    Scene.openMap(self.back)
  end
end

return ArenaExit
