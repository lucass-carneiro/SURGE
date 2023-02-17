 -- Demonstrates character animations

local sophia_scale_factor = 5.0
local move_speed = 0.1

function surge.key_event(key, action, mods)
    if key == surge.keyboard_key.RIGHT and action == surge.input_action.PRESS then
        surge.move_actor(sophia_actor, move_speed, 0.0, 0.0)
    elseif key == surge.keyboard_key.LEFT and action == surge.input_action.PRESS then
        surge.move_actor(sophia_actor, -move_speed, 0.0, 0.0)
    end
end

function surge.mouse_button_event(button, action, mods)
    -- do nothing
end

function surge.mouse_scroll_event(xoffset, yoffset)
    -- do nothing
end

function surge.pre_loop()
    -- Load actor
    sophia_actor = surge.new_actor(
        "character_animations/resources/spritesheet.png",
        "character_animations/resources/spritesheet.sad"
    )

    -- Set actor animation index. Do this before calling set_actor_position
    -- because it uses the current animation sheet for scaling the sprite
    surge.set_actor_animation(sophia_actor, 6)

    -- Set actor anchor point (the place in the sprite where the position parameter refers to)
    surge.set_actor_anchor_point(sophia_actor, 16.0 * sophia_scale_factor, 48.0 * sophia_scale_factor, 0.0)
    
    -- Set actor position and scale
    surge.set_actor_position(
        sophia_actor,
        
        -- Pos
        surge.window_width / 2.0, 
        surge.window_height / 2.0,
        0.0,
        
        -- XY Scale factor
        sophia_scale_factor
    )
end

function surge.draw()
    surge.draw_actor(sophia_actor)
end

local animation_frame_time = 0.0

function surge.update(dt)
    animation_frame_time = animation_frame_time + dt
    if animation_frame_time > 0.1 then
        animation_frame_time = 0.0
        surge.advance_actor_frame(sophia_actor)
    end
end