-- 2048 Main file

local board = require("2048/board")

function surge.pre_loop()
    -- Load board
    b = board:new()
end

function surge.key_event(key, action, mods)
    -- Reset game
    if key == surge.input.keyboard.key.R and action == surge.input.action.PRESS then
        b:reset()

    elseif key == surge.input.keyboard.key.DOWN and action == surge.input.action.PRESS then
        b:compress_down()
        -- b:merge_down()
        --b:compress_down()
        --b:new_piece()
    elseif key == surge.input.keyboard.key.UP and action == surge.input.action.PRESS then
        b:compress_up()
    elseif key == surge.input.keyboard.key.RIGHT and action == surge.input.action.PRESS then
        b:compress_right()
    elseif key == surge.input.keyboard.key.LEFT and action == surge.input.action.PRESS then
        b:compress_left()
    end
end

function surge.mouse_button_event(button, action, mods)
    -- do nothing
end

function surge.mouse_scroll_event(xoffset, yoffset)
    -- do nothing
end

function surge.draw()
    b:draw()
end

function surge.update(dt)
    b:update(dt)
end