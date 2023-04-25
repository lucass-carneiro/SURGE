-- 2048 Main file

local ww = surge.config.window_width
local wh = surge.config.window_height

function surge.pre_loop()
    -- Load board
    board = surge.image.new(
        "2048/resources/board.png",
        0.0, 0.0, 0.0,
        ww , wh , 1.0
    )

    -- Load pieces
    piece = surge.image.new(
       "2048/resources/pieces.png",
       18.0,  18.0, 0.1,
       214, 214, 1.0
   )
end

function surge.key_event(key, action, mods)
    -- do nothing
end

function surge.mouse_button_event(button, action, mods)
    -- do nothing
end

function surge.mouse_scroll_event(xoffset, yoffset)
    -- do nothing
end

function surge.draw()
    surge.image.draw(board)
    --surge.image.draw(piece)
    surge.image.draw_region(piece, 214.0, 214.0, 214.0, 214.0)
end

function surge.update(dt)
    -- do nothing
end