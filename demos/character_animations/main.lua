 -- Demonstrates character animations

 local move_speed = 10.0

function surge.key_event(key, action, mods)
    if key == surge.keyboard_key.RIGHT and action == surge.input_action.PRESS then
        surge.move_actor(sophia_actor, move_speed, 0.0, 0.0)
    elseif key == surge.keyboard_key.LEFT and action == surge.input_action.PRESS then
        surge.move_actor(sophia_actor, -move_speed, 0.0, 0.0)
    elseif key == surge.keyboard_key.PAGE_UP and action == surge.input_action.PRESS then
        surge.scale_actor(sophia_actor, 1.5, 1.5, 0.0)
    elseif key == surge.keyboard_key.PAGE_DOWN and action == surge.input_action.PRESS then
        surge.scale_actor(sophia_actor, 1.0/1.5, 1.0/1.5, 0.0)
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
    
    -- Select first animation
    surge.set_actor_animation(sophia_actor, 6)
    
    -- Place the actor on the scene
    surge.set_actor_geometry(
        sophia_actor, 
        16.0, 48.0, 0.0,                                           -- Anchor
        surge.window_width / 2.0,  surge.window_height / 2.0, 0.0, -- Position
        5.0, 5.0, 1.0                                              -- Scale
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