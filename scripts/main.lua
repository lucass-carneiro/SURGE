-- SURGE entry point script

function surge.pre_loop()
    surge.log_message("surge.pre_loop()")

    local origin = {0.0, 0.0}
    local size = {surge.window_width, surge.window_height}
    
    sprite = surge.load_sprite("/home/lucas/SURGE/resources/images/spritesheet.png", ".png", 6, 6)
    surge.move_sprite(sprite, origin[1], origin[2], 0.0)
    surge.scale_sprite(sprite, size[1], size[2], 0.0)
    surge.sheet_set(sprite, 0, 0)
end

function surge.draw()
    surge.draw_sprite(sprite)
end

timer = 0.0

function surge.update(dt)
    timer = timer + dt
    if(timer > 0.15) then
        timer = 0.0
        surge.sheet_next(sprite)
    end
end