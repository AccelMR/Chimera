-- chimera-uuid: 5b40870d-ffad-4477-a1a1-af906c32bf5b
-- Shows the rest of the Input API in the log:
--   1: the next key pressed becomes the first key of Jump (captureNextKey).
--   2: Jump goes back to its default keys.
--   3: saves the changed keys to Saved/Config/Input.ini.
--   4: the cursor switches between normal and locked (Esc stops Play in the editor).
local Input = Input

local InputDemo = {}

local function describe(action)
  return Input.getActionName(action) .. " = " .. table.concat(Input.getBindings(action), ", ")
end

function InputDemo:onStart()
  self.jump = Input.findAction("Jump")
  print("InputDemo: 1 rebinds Jump, 2 resets it, 3 saves, 4 locks the cursor.")
  for id = 0, Input.getActionCount() - 1 do
    print("  " .. describe(id))
  end
end

function InputDemo:update(dt)
  if Input.wasActionPressed(self.jump) then
    print("Jump pressed (" .. describe(self.jump) .. ")")
  end

  if Input.wasKeyPressed(Key.Num1) and not Input.isCapturingKey() then
    print("Press the new key for Jump...")
    local jump = self.jump
    Input.captureNextKey(function(key)
      local name = Input.getKeyName(key)
      local users = Input.findActionsUsingKey(key)
      if #users > 0 then
        print("Note:", name, "is also used by", Input.getActionName(users[1]))
      end
      if Input.setBinding(jump, 1, name) then
        print("Jump now:", describe(jump))
      else
        print(name, "cannot be a key of Jump")
      end
    end)
  elseif Input.wasKeyPressed(Key.Num2) then
    Input.resetBindings(self.jump)
    print("Jump reset:", describe(self.jump))
  elseif Input.wasKeyPressed(Key.Num3) then
    print("Saved keys:", Input.saveUserBindings())
  elseif Input.wasKeyPressed(Key.Num4) then
    local mode = Input.getCursorMode() == "Locked" and "Normal" or "Locked"
    Input.setCursorMode(mode)
    print("Cursor:", mode)
  end
end

return InputDemo
