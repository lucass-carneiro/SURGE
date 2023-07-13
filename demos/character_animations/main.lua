-- Demonstrates character animations
local keyboard_motion_actor = require(
                                  "character_animations/keyboard_motion_actor")

function surge.pre_loop()
  sophia = keyboard_motion_actor:new(
               "character_animations/resources/sophia/sophia.png", -- Sprite sheet image file
               "character_animations/resources/sophia/sophia.sad", -- Animations file
               4, -- First animation index
      16.0, 48.0, 0.0, -- Anchor
      400.0, 300.0, 0.0, -- Position
      5.0, 5.0, 1.0 -- Scale
  )
end

function surge.key_event(key, action, mods)
  sophia:detect_arrows_released(key, action, scale)
end

function surge.mouse_button_event(button, action, mods)
  if button == surge.input.mouse.button.RIGHT and action ==
      surge.input.action.PRESS then
    local click_x, click_y, click_z = surge.input.mouse.get_cursor_position()
    surge.log.message("(", click_x, ",", click_y, ",", click_z, ")")
  end
end

function surge.mouse_scroll_event(xoffset, yoffset)
  -- do nothing
end

function surge.draw()
  sophia:draw()
end

function surge.update(dt)
  -- sophia:update(dt, 0.08)
end
