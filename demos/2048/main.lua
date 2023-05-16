board = require("2048/board")

function surge.pre_loop()
  -- Loads texture file used for all pieces
  piece:load_texture()

  -- Create board object and matrix
  b = board:new()
  b:new_piece()
  b:new_piece()
end

function surge.key_event(key, action, mods)
  if key == surge.input.keyboard.key.C and action == surge.input.action.PRESS and b:is_idle() then
    -- TODO:
  elseif key == surge.input.keyboard.key.UP and action == surge.input.action.PRESS and b:is_idle() then
    b:game_move_up()
  elseif key == surge.input.keyboard.key.DOWN and action == surge.input.action.PRESS and b:is_idle() then
    b:game_move_down()
  elseif key == surge.input.keyboard.key.LEFT and action == surge.input.action.PRESS and b:is_idle() then
    b:game_move_left()
  elseif key == surge.input.keyboard.key.RIGHT and action == surge.input.action.PRESS and b:is_idle() then
    b:game_move_right()
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