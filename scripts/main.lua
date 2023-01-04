-- SURGE entry point script

local timer = 0.0
local threshold = 0.1

local cursor_x = 0.0
local cursor_y = 0.0

function surge.key_event(key, action, mods)
    if key == surge.keyboard_key.RIGHT and action == surge.input_action.PRESS then
        threshold = threshold + 0.01
        surge.log_message("Animation frame rate: ", 1.0/threshold, " FPS")

    elseif key == surge.keyboard_key.LEFT and action == surge.input_action.PRESS then
        threshold = threshold - 0.01
        surge.log_message("Animation frame rate: ", 1.0/threshold, " FPS")
    end
end

function surge.mouse_button_event(button, action, mods)
    if button == surge.mouse_button.LEFT and action == surge.input_action.PRESS then
        surge.log_message("Left click at pos (", cursor_x, ",", cursor_y, ")")

    elseif button == surge.mouse_button.MIDDLE and action == surge.input_action.PRESS then
        surge.log_message("Middle click at pos (", cursor_x, ",", cursor_y, ")")

    elseif button == surge.mouse_button.RIGHT and action == surge.input_action.PRESS then
        surge.log_message("Right click at pos (", cursor_x, ",", cursor_y, ")")
    end
end

function surge.mouse_scroll_event(xoffset, yoffset)
    surge.log_message("Mouse offset (", xoffset, ",", yoffset, ")")
end

function surge.pre_loop()
    surge.log_message("surge.pre_loop()")

    local origin = {0.0, 0.0}
    local size = {surge.window_width, surge.window_height}
    
    sprite = surge.load_sprite("/home/lucas/SURGE/resources/images/spritesheet_rain.png", ".png", 5, 4)
    surge.move_sprite(sprite, origin[1], origin[2], 0.0)
    surge.scale_sprite(sprite, size[1], size[2], 0.0)
    surge.sheet_set(sprite, 0, 0)
end

function surge.draw()
    surge.draw_sprite(sprite)
end

function surge.update(dt)
    timer = timer + dt
    if(timer > threshold) then
        timer = 0.0
        surge.sheet_next(sprite)
    end

    cursor_x, cursor_y = surge.get_cursor_pos()
end