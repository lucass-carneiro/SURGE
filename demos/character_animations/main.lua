-- Demonstrates character animations

local vec3 = require("character_animations/vec3")

local displacement_threshold = 2.0
local speed = 3.8

local current_anim_idx = 5
local left_click_coords = vec3.new(0, 0, 0)
local anchor_coords = vec3.new(0, 0, 0)
local required_displacement = vec3.new(0, 0, 0)
local reuired_velocity = vec3.new(0, 0, 0)
local heading_direction = 0 -- north = 0 south = 1 east = 2 west = 3

function should_move()
    anchor_coords = vec3.new(surge.get_actor_anchor_coords(sophia_actor))
    required_displacement = left_click_coords - anchor_coords
    
    if required_displacement:norm() > displacement_threshold then
        required_displacement:normalize()
        reuired_velocity = vec3.scale(required_displacement, speed)
        return true
    else
        return false
    end
end

function find_heading()
    if math.abs(required_displacement.x) > math.abs(required_displacement.y) then
        if required_displacement.x > 0 then
            -- east
            heading_direction = 2
        else
            -- west
            heading_direction = 3
        end
    else
        if required_displacement.y > 0 then
            -- north
            heading_direction = 0
        else
            -- south
            heading_direction = 1
        end
    end
end

function set_anim_on_heading()
    if heading_direction == 0 and current_anim_idx  ~= 1 then
        surge.set_actor_animation(sophia_actor, 1)
        current_anim_idx = 1
    elseif heading_direction == 1 and current_anim_idx  ~= 0 then
        surge.set_actor_animation(sophia_actor, 0)
        current_anim_idx = 0
    elseif heading_direction == 2 and current_anim_idx  ~= 6 then
        surge.set_actor_animation(sophia_actor, 6)
        current_anim_idx = 6
    end
end

function surge.pre_loop()
    -- Load actor
    sophia_actor = surge.new_actor(
        "character_animations/resources/spritesheet.png",
        "character_animations/resources/spritesheet.sad"
    )
    
    -- Select first animation
    surge.set_actor_animation(sophia_actor, current_anim_idx)
    
    -- Place the actor on the scene
    surge.set_actor_geometry(
        sophia_actor, 
        16.0, 48.0, 0.0,                                           -- Anchor
        surge.window_width / 2.0,  surge.window_height / 2.0, 0.0, -- Position
        5.0, 5.0, 1.0                                              -- Scale
    )

    -- Stops initial movement
    left_click_coords = vec3.new(surge.get_actor_anchor_coords(sophia_actor))
end

function surge.key_event(key, action, mods)
    if key == surge.keyboard_key.PAGE_UP and action == surge.input_action.PRESS then
        surge.scale_actor(sophia_actor, 1.5, 1.5, 0.0)
    elseif key == surge.keyboard_key.PAGE_DOWN and action == surge.input_action.PRESS then
        surge.scale_actor(sophia_actor, 1.0/1.5, 1.0/1.5, 0.0)
    end
end

function surge.mouse_button_event(button, action, mods)
    if button == surge.mouse_button.RIGHT and action == surge.input_action.PRESS then
        left_click_coords = vec3.new(surge.get_cursor_pos())
    end
end

function surge.mouse_scroll_event(xoffset, yoffset)
    -- do nothing
end

function surge.draw()
    surge.draw_actor(sophia_actor)
end

function surge.update(dt)
    surge.play_actor_animation(sophia_actor, 0.1)

    if should_move() then
        surge.move_actor(sophia_actor, reuired_velocity.x, reuired_velocity.y, reuired_velocity.z)
        find_heading()
        set_anim_on_heading()
    end
end