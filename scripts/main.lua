-- SURGE entry point script

local timer = 0.0
local threshold = 0.1

surge.log_message("Animation frame rate: ", 1.0/threshold, " FPS")

function surge.key_event(key, action, mods)
    if key == surge.keys.RIGHT then
        threshold = threshold + 0.01
        surge.log_message("Animation frame rate: ", 1.0/threshold, " FPS")
    elseif key == surge.keys.LEFT then
        threshold = threshold - 0.01
        surge.log_message("Animation frame rate: ", 1.0/threshold, " FPS")
    end
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
end