local game_data = require("2048/game_data")
local piece = require("2048/piece")
local board = require("2048/board")

function surge.pre_loop()
  game_data.piece_list = surge.util.prealloc_table(16)
  game_data.piece_list_buffer = surge.util.prealloc_table(16)
  game_data.element_buffer = surge.util.prealloc_table(4)

  local board_texture_path = game_data.debug and
                                 "2048/resources/board_debug.png" or
                                 "2048/resources/board_normal.png"

  game_data.board_image_object = surge.image.new(board_texture_path, 0.0, 0.0,
                                                 0.0, surge.config.window_width,
                                                 surge.config.window_width, 1.0)

  game_data.piece_image_object = surge.image.new("2048/resources/pieces.png",
                                                 game_data.slots_x_pos[1],
                                                 game_data.slots_y_pos[1], 0.1,
                                                 game_data.slot_size,
                                                 game_data.slot_size, 1.0)

  board.add_random_piece(piece, game_data)
  board.add_random_piece(piece, game_data)
end

function surge.key_event(key, action, mods)
  local press_R = key == surge.input.keyboard.key.R and action ==
                      surge.input.action.PRESS

  local press_UP = key == surge.input.keyboard.key.UP and action ==
                       surge.input.action.PRESS

  local press_DOWN = key == surge.input.keyboard.key.DOWN and action ==
                         surge.input.action.PRESS

  local press_LEFT = key == surge.input.keyboard.key.LEFT and action ==
                         surge.input.action.PRESS

  local press_RIGHT = key == surge.input.keyboard.key.RIGHT and action ==
                          surge.input.action.PRESS

  local is_idle = board.is_idle(game_data) and board.pieces_idle(game_data)

  if press_UP and is_idle then
    game_data.game_state_queue:push_back(game_data.game_states.compress_up)
    game_data.game_state_queue:push_back(game_data.game_states.merge_up)
    game_data.game_state_queue:push_back(game_data.game_states.piece_removal)
    game_data.game_state_queue:push_back(game_data.game_states.compress_up)

  elseif press_DOWN and is_idle then
    game_data.game_state_queue:push_back(game_data.game_states.compress_down)
    game_data.game_state_queue:push_back(game_data.game_states.merge_down)
    game_data.game_state_queue:push_back(game_data.game_states.piece_removal)
    game_data.game_state_queue:push_back(game_data.game_states.compress_down)

  elseif press_LEFT and is_idle then
    game_data.game_state_queue:push_back(game_data.game_states.compress_left)
    game_data.game_state_queue:push_back(game_data.game_states.merge_left)
    game_data.game_state_queue:push_back(game_data.game_states.piece_removal)
    game_data.game_state_queue:push_back(game_data.game_states.compress_left)

  elseif press_RIGHT and is_idle then
    game_data.game_state_queue:push_back(game_data.game_states.compress_right)
    game_data.game_state_queue:push_back(game_data.game_states.merge_right)
    game_data.game_state_queue:push_back(game_data.game_states.piece_removal)
    game_data.game_state_queue:push_back(game_data.game_states.compress_right)
  end
end

function surge.mouse_button_event(button, action, mods)
  -- do nothing
end

function surge.mouse_scroll_event(xoffset, yoffset)
  -- do nothing
end

function surge.draw()
  surge.image.draw(game_data.board_image_object)
  board.draw_pieces(game_data)
end

function surge.update(dt)
  local game_state = game_data.game_state_queue:front()
  local pieces_idle = board.pieces_idle(game_data)

  if game_state == game_data.game_states.compress_up and pieces_idle then
    if game_data.debug then
      game_data.debug_file:write("Compress up\n")
      surge.log.log_info("Compress up")
    end

    board.sync_piece_list(game_data)

    if board.compress_up(game_data) then
      game_data.game_state_queue:push_back(game_data.game_states.add_piece)
    end

    game_data.game_state_queue:pop_front()

  elseif game_state == game_data.game_states.compress_down and pieces_idle then
    if game_data.debug then
      game_data.debug_file:write("Compress down\n")
      surge.log.log_info("Compress down")
    end

    board.sync_piece_list(game_data)

    if board.compress_down(game_data) then
      game_data.game_state_queue:push_back(game_data.game_states.add_piece)
    end

    game_data.game_state_queue:pop_front()

  elseif game_state == game_data.game_states.compress_left and pieces_idle then
    if game_data.debug then
      game_data.debug_file:write("Compress left\n")
      surge.log.log_info("Compress left")
    end

    board.sync_piece_list(game_data)

    if board.compress_left(game_data) then
      game_data.game_state_queue:push_back(game_data.game_states.add_piece)
    end

    game_data.game_state_queue:pop_front()

  elseif game_state == game_data.game_states.compress_right and pieces_idle then
    if game_data.debug then
      game_data.debug_file:write("Compress right\n")
      surge.log.log_info("Compress right")
    end

    board.sync_piece_list(game_data)

    if board.compress_right(game_data) then
      game_data.game_state_queue:push_back(game_data.game_states.add_piece)
    end

    game_data.game_state_queue:pop_front()

  elseif game_state == game_data.game_states.merge_up and pieces_idle then
    if game_data.debug then
      game_data.debug_file:write("Merge up\n")
      surge.log.log_info("Merge up")
    end

    board.sync_piece_list(game_data)

    if board.merge_up(game_data) then
      game_data.game_state_queue:push_back(game_data.game_states.add_piece)
    end

    game_data.game_state_queue:pop_front()

  elseif game_state == game_data.game_states.merge_down and pieces_idle then
    if game_data.debug then
      game_data.debug_file:write("Merge down\n")
      surge.log.log_info("Merge down")
    end

    board.sync_piece_list(game_data)

    if board.merge_down(game_data) then
      game_data.game_state_queue:push_back(game_data.game_states.add_piece)
    end

    game_data.game_state_queue:pop_front()

  elseif game_state == game_data.game_states.merge_left and pieces_idle then
    if game_data.debug then
      game_data.debug_file:write("Merge left\n")
      surge.log.log_info("Merge left")
    end

    board.sync_piece_list(game_data)

    if board.merge_left(game_data) then
      game_data.game_state_queue:push_back(game_data.game_states.add_piece)
    end

    game_data.game_state_queue:pop_front()

  elseif game_state == game_data.game_states.merge_right and pieces_idle then
    if game_data.debug then
      game_data.debug_file:write("Merge right\n")
      surge.log.log_info("Merge right")
    end

    board.sync_piece_list(game_data)

    if board.merge_right(game_data) then
      game_data.game_state_queue:push_back(game_data.game_states.add_piece)
    end

    game_data.game_state_queue:pop_front()

  elseif game_state == game_data.game_states.piece_removal and pieces_idle then
    if game_data.debug then
      game_data.debug_file:write("Remove piece\n")
      surge.log.log_info("Remove piece")
    end

    while game_data.piece_removal_queue:size() ~= 0 do
      local I = game_data.piece_removal_queue:pop_front()
      game_data.piece_list[I] = nil
    end

    game_data.game_state_queue:pop_front()

  elseif game_state == game_data.game_states.add_piece and pieces_idle then
    if game_data.debug then
      game_data.debug_file:write("Add piece\n")
      surge.log.log_info("Add piece")
    end

    board.sync_piece_list(game_data)
    board.add_random_piece(piece, game_data)

    while game_data.game_state_queue:front() == game_data.game_states.add_piece do
      game_data.game_state_queue:pop_front()
    end

    game_data.game_state_queue:push_back(game_data.game_states.print_piece_list)

  elseif game_state == game_data.game_states.print_piece_list and pieces_idle then
    board.sync_piece_list(game_data)
    board.print_piece_list(game_data)
    game_data.game_state_queue:pop_front()
  end

  board.update_pieces(dt, game_data)
end
