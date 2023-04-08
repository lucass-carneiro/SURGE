local keyboard_motion_actor = {}
keyboard_motion_actor.__index = keyboard_motion_actor

function keyboard_motion_actor:new(sprite_image_file, animations_file, first_anim, anchor_x,anchor_y,anchor_z, position_x,position_y,position_z, scale_x, scale_y, scale_z)
   local actr = {}
   setmetatable(actr, keyboard_motion_actor)
    
   actr.surge_actor_object = surge.actor.new(
       sprite_image_file,
       animations_file,
       first_anim,
       anchor_x,  anchor_y, anchor_z,
       position_x, position_y, position_z,
       scale_x, scale_y, scale_z
   )

   actr.moving = false
   actr.heading = surge.geometry.heading.E

   return actr
end

function keyboard_motion_actor:draw()
    surge.actor.draw(self.surge_actor_object)
end

function keyboard_motion_actor:update(dt, fps)
    local is_pressed_right = surge.input.keyboard.get_key_state(surge.input.keyboard.key.RIGHT) == surge.input.action.PRESS
    local is_pressed_left = surge.input.keyboard.get_key_state(surge.input.keyboard.key.LEFT) == surge.input.action.PRESS
    local is_pressed_up = surge.input.keyboard.get_key_state(surge.input.keyboard.key.UP) == surge.input.action.PRESS
    local is_pressed_down = surge.input.keyboard.get_key_state(surge.input.keyboard.key.DOWN) == surge.input.action.PRESS

    local is_released_right = surge.input.keyboard.get_key_state(surge.input.keyboard.key.RIGHT) == surge.input.action.RELEASE
    local is_released_left = surge.input.keyboard.get_key_state(surge.input.keyboard.key.LEFT) == surge.input.action.RELEASE
    local is_released_up = surge.input.keyboard.get_key_state(surge.input.keyboard.key.UP) == surge.input.action.RELEASE
    local is_released_down = surge.input.keyboard.get_key_state(surge.input.keyboard.key.DOWN) == surge.input.action.RELEASE

    local right_exclusive_press = is_pressed_right and is_released_left and is_released_up and is_released_down
    local left_exclusive_press  = is_pressed_left and is_released_right and is_released_up and is_released_down
    local up_exclusive_press    = is_pressed_up and is_released_right and is_released_left and is_released_down
    local down_exclusive_press  = is_pressed_down and is_released_right and is_released_left and is_released_up

    if right_exclusive_press then

        if self.heading ~= surge.geometry.heading.E then
            surge.actor.toggle_h_flip(self.surge_actor_object)
            self.heading = surge.geometry.heading.E
        end

        if self.moving == false then
            surge.actor.change_anim(self.surge_actor_object, 6, true)
            self.moving = true
        end

        surge.actor.move(self.surge_actor_object, 1.3 * dt, 0.0, 0.0)
        
    elseif left_exclusive_press then

        if self.heading ~= surge.geometry.heading.W then
            surge.actor.toggle_h_flip(self.surge_actor_object)
            self.heading = surge.geometry.heading.W
        end

        if self.moving == false then
            surge.actor.change_anim(self.surge_actor_object, 6, true)
            self.moving = true
        end

        surge.actor.move(self.surge_actor_object, -1.3 * dt, 0.0, 0.0)

    elseif up_exclusive_press then

        if self.heading ~= surge.geometry.heading.N then
            self.heading = surge.geometry.heading.N
        end

        if self.moving == false then
            surge.actor.change_anim(self.surge_actor_object, 0, true)
            self.moving = true
        end

        surge.actor.move(self.surge_actor_object, 0.0, -1.0 * dt, 0.0)

    elseif down_exclusive_press then

        if self.heading ~= surge.geometry.heading.S then
            self.heading = surge.geometry.heading.S
        end

        if self.moving == false then
            surge.actor.change_anim(self.surge_actor_object, 1, true)
            self.moving = true
        end

        surge.actor.move(self.surge_actor_object, 0.0, 1.0 * dt, 0.0)
    end

    surge.actor.update(self.surge_actor_object, fps)
end

function keyboard_motion_actor:scale(x, y, z)
    surge.actor.scale(self.surge_actor_object, x, y, z)
end

function keyboard_motion_actor:move(x, y, z)
    surge.actor.move(self.surge_actor_object, x, y, z)
end

function keyboard_motion_actor:detect_arrows_released(key, action, mods)
    if key == surge.input.keyboard.key.RIGHT and action == surge.input.action.RELEASE then
        self.moving = false
        surge.actor.change_anim(self.surge_actor_object, 5, false)

    elseif key == surge.input.keyboard.key.LEFT and action == surge.input.action.RELEASE then
        self.moving = false
        surge.actor.change_anim(self.surge_actor_object, 5, false)

    elseif key == surge.input.keyboard.key.UP and action == surge.input.action.RELEASE then
        self.moving = false
        surge.actor.change_anim(self.surge_actor_object, 3, false)

    elseif key == surge.input.keyboard.key.DOWN and action == surge.input.action.RELEASE then
        self.moving = false
        surge.actor.change_anim(self.surge_actor_object, 4, false)
    end
end

return keyboard_motion_actor