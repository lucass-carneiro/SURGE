 -- Demonstrates character animations

 local animation_index = 0

-- Show next animation
 function next_anim()
    if animation_index < 6 then
        animation_index = animation_index + 1
    else
        animation_index = 0
    end
end

-- Show previous
function prev_anim()
    if animation_index > 0 then
        animation_index = animation_index - 1
    else
        animation_index = 6
    end
end

function surge.key_event(key, action, mods)
    if key == surge.keyboard_key.RIGHT and action == surge.input_action.PRESS then
        next_anim()
        surge.set_actor_animation(sophia_actor, animation_index)       
    elseif key == surge.keyboard_key.LEFT and action == surge.input_action.PRESS then
        prev_anim()
        surge.set_actor_animation(sophia_actor, animation_index)
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
    surge.set_actor_animation(sophia_actor, animation_index)
    
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

function surge.update(dt)
    surge.play_actor_animation(sophia_actor, 0.1)
end