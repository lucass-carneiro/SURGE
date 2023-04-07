-- Demonstrates character animations

local keyboard_motion_actor = require("character_animations/keyboard_motion_actor")

function surge.pre_loop()
    -- Load actor
    sophia = keyboard_motion_actor:new(
        "character_animations/resources/spritesheet.png",                        -- Sprite sheet image file
        "character_animations/resources/spritesheet.sad",                        -- Animations file
        5,                                                                       -- First animation index
        16.0, 48.0, 0.0,                                                         -- Anchor
        surge.config.window_width / 2.0,  surge.config.window_height / 2.0, 0.0, -- Position
        5.0, 5.0, 1.0                                                            -- Scale
    )
end

function surge.key_event(key, action, mods)
    if key == surge.input.keyboard.key.PAGE_UP and action == surge.input.action.PRESS then
        sophia:scale(1.5, 1.5, 0.0)
    elseif key == surge.input.keyboard.key.PAGE_DOWN and action == surge.input.action.PRESS then
        sophia:scale(1.0/1.5, 1.0/1.5, 0.0)
    end
end

function surge.mouse_button_event(button, action, mods)
    if button == surge.input.mouse.button.RIGHT and action == surge.input.action.PRESS then
        click_x, click_y, click_z = surge.input.mouse.get_cursor_position()
    end
end

function surge.mouse_scroll_event(xoffset, yoffset)
    -- do nothing
end

function surge.draw()
    sophia:draw()
end

function surge.update(dt)
    sophia:update(dt, 0.08)
end