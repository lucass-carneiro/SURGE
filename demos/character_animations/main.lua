-- Demonstrates character animations

local vec3 = require("character_animations/vec3")

function surge.pre_loop()
    -- Load actor
    sophia_actor = surge.actor.new(
        "character_animations/resources/spritesheet.png",          -- Sprite sheet image file
        "character_animations/resources/spritesheet.sad",          -- Animations file
        5,                                                         -- First animation index
        16.0, 48.0, 0.0,                                           -- Anchor
        surge.config.window_width / 2.0,  surge.config.window_height / 2.0, 0.0, -- Position
        5.0, 5.0, 1.0                                              -- Scale
    )
    
    click_x, click_y, click_z = surge.actor.get_anchor_pos(sophia_actor)
end

function surge.key_event(key, action, mods)
    if key == surge.input.keyboard.key.PAGE_UP and action == surge.input_action.PRESS then
        surge.actor.scale(sophia_actor, 1.5, 1.5, 0.0)
    elseif key == surge.input.keyboard.key.PAGE_DOWN and action == surge.input_action.PRESS then
        surge.actor.scale(sophia_actor, 1.0/1.5, 1.0/1.5, 0.0)
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
    surge.actor.draw(sophia_actor)
end

function surge.update(dt)
    surge.actor.walk_to(sophia_actor, click_x, click_y, click_z, 3.0, 2.0)
    surge.actor.update_animation(sophia_actor, 0.1)
end