local board = {}

function board.print_piece_list(game_data)
  if game_data.debug then
    for _, p in pairs(game_data.piece_list) do
      game_data.debug_file:write(tostring(p))
      game_data.debug_file:write("\n")
    end
  end
end

function board.draw_pieces(game_data)
  for _, p in pairs(game_data.piece_list) do p:draw(game_data) end
end

function board.update_pieces(dt, game_data)
  for _, p in pairs(game_data.piece_list) do p:update(dt, game_data) end
end

function board.is_idle(game_data)
  return game_data.game_state_queue:size() == 0
end

function board.pieces_idle(game_data)
  local pieces_idle = true

  for _, p in pairs(game_data.piece_list) do
    if p.command_queue:front() ~= nil then pieces_idle = false end
  end

  return pieces_idle
end

function board.element_buffer_size(game_data)
  local count = 0

  for i = 1, 4, 1 do
    if game_data.element_buffer[i] ~= nil then count = count + 1 end
  end

  return count
end

function board.sync_piece_list(game_data)
  for i = 1, 16, 1 do game_data.piece_list_buffer[i] = nil end

  for _, p in pairs(game_data.piece_list) do
    game_data.piece_list_buffer[p.I] = p
  end

  for i = 1, 16, 1 do game_data.piece_list[i] = game_data.piece_list_buffer[i] end

end

function board.add_random_piece(piece, game_data)
  math.randomseed(os.time())

  local I = math.random(1, 16)

  while game_data.piece_list[I] ~= nil do
    if game_data.debug then
      game_data.debug_file:write("Hit I = ")
      game_data.debug_file:write(tostring(I))
      game_data.debug_file:write("\n")
    end

    I = math.random(1, 16)
  end

  local exponent = math.random(1, 2)

  local i = math.floor((I - 1) / 4) + 1
  local j = (I - 1) % 4 + 1

  local p = piece:new(i, j, exponent, game_data)

  if game_data.debug then
    game_data.debug_file:write("Added ")
    game_data.debug_file:write(tostring(p))
    game_data.debug_file:write("\n")
  end
end

function board.compress_up(game_data)
  local board_changed = false

  for j = 1, 4, 1 do
    -- Fill the element buffer with the pieces in the column
    game_data.element_buffer[1] = game_data.piece_list[j]
    game_data.element_buffer[2] = game_data.piece_list[j + 4]
    game_data.element_buffer[3] = game_data.piece_list[j + 8]
    game_data.element_buffer[4] = game_data.piece_list[j + 12]

    local element_count = board.element_buffer_size(game_data)

    if element_count == 0 or element_count == 4 then
      goto continue
    else
      local element = 1
      for _, p in pairs(game_data.element_buffer) do
        local delta_slots = p.i - element
        board_changed = delta_slots ~= 0
        p:move_up(game_data, delta_slots)
        element = element + 1
      end
    end

    ::continue::
  end

  return board_changed
end

function board.compress_down(game_data)
  local board_changed = false

  for j = 1, 4, 1 do
    -- Fill the element buffer with the pieces in the column
    game_data.element_buffer[1] = game_data.piece_list[j + 12]
    game_data.element_buffer[2] = game_data.piece_list[j + 8]
    game_data.element_buffer[3] = game_data.piece_list[j + 4]
    game_data.element_buffer[4] = game_data.piece_list[j]

    local element_count = board.element_buffer_size(game_data)

    if element_count == 0 or element_count == 4 then
      goto continue
    else
      local element = 4
      for _, p in pairs(game_data.element_buffer) do
        local delta_slots = element - p.i
        board_changed = delta_slots ~= 0
        p:move_down(game_data, delta_slots)
        element = element - 1
      end
    end

    ::continue::
  end

  return board_changed
end

function board.compress_left(game_data)
  local board_changed = false

  for i = 1, 4, 1 do
    -- Fill the element buffer with the pieces in the column
    game_data.element_buffer[1] = game_data.piece_list[1 + (i - 1) * 4]
    game_data.element_buffer[2] = game_data.piece_list[2 + (i - 1) * 4]
    game_data.element_buffer[3] = game_data.piece_list[3 + (i - 1) * 4]
    game_data.element_buffer[4] = game_data.piece_list[4 + (i - 1) * 4]

    local element_count = board.element_buffer_size(game_data)

    if element_count == 0 or element_count == 4 then
      goto continue
    else
      local element = 1
      for _, p in pairs(game_data.element_buffer) do
        local delta_slots = p.j - element
        board_changed = delta_slots ~= 0
        p:move_left(game_data, delta_slots)
        element = element + 1
      end
    end

    ::continue::
  end

  return board_changed
end

function board.compress_right(game_data)
  local board_changed = false

  for i = 1, 4, 1 do
    -- Fill the element buffer with the pieces in the column
    game_data.element_buffer[1] = game_data.piece_list[4 + (i - 1) * 4]
    game_data.element_buffer[2] = game_data.piece_list[3 + (i - 1) * 4]
    game_data.element_buffer[3] = game_data.piece_list[2 + (i - 1) * 4]
    game_data.element_buffer[4] = game_data.piece_list[1 + (i - 1) * 4]

    local element_count = board.element_buffer_size(game_data)

    if element_count == 0 or element_count == 4 then
      goto continue
    else
      local element = 4
      for _, p in pairs(game_data.element_buffer) do
        local delta_slots = element - p.j
        board_changed = delta_slots ~= 0
        p:move_right(game_data, delta_slots)
        element = element - 1
      end
    end

    ::continue::
  end

  return board_changed
end

function board.merge_up(game_data)
  local board_changed = false

  for j = 1, 4, 1 do
    -- Fill the element buffer with the pieces in the column
    game_data.element_buffer[1] = game_data.piece_list[j]
    game_data.element_buffer[2] = game_data.piece_list[j + 4]
    game_data.element_buffer[3] = game_data.piece_list[j + 8]
    game_data.element_buffer[4] = game_data.piece_list[j + 12]

    local num_elements = board.element_buffer_size(game_data)

    if num_elements == 0 or num_elements == 1 then
      goto continue

    elseif num_elements == 2 then
      local exp1 = game_data.element_buffer[1].exponent
      local exp2 = game_data.element_buffer[2].exponent

      if exp1 == exp2 then
        game_data.element_buffer[2]:move_up(game_data)
        game_data.element_buffer[2].command_queue:push_back(exp2 + 1)
        game_data.piece_removal_queue:push_back(j)
        board_changed = true
      end

    elseif num_elements == 3 then
      local exp1 = game_data.element_buffer[1].exponent
      local exp2 = game_data.element_buffer[2].exponent
      local exp3 = game_data.element_buffer[3].exponent

      if exp1 == exp2 then
        game_data.element_buffer[2]:move_up(game_data)
        game_data.element_buffer[2].command_queue:push_back(exp2 + 1)
        game_data.piece_removal_queue:push_back(j)
        board_changed = true

      elseif exp2 == exp3 then
        game_data.element_buffer[3]:move_up(game_data)
        game_data.element_buffer[3].command_queue:push_back(exp3 + 1)
        game_data.piece_removal_queue:push_back(j + 4)
        board_changed = true
      end

    elseif num_elements == 4 then
      local exp1 = game_data.element_buffer[1].exponent
      local exp2 = game_data.element_buffer[2].exponent
      local exp3 = game_data.element_buffer[3].exponent
      local exp4 = game_data.element_buffer[4].exponent

      if exp1 == exp2 then
        game_data.element_buffer[2]:move_up(game_data)
        game_data.element_buffer[2].command_queue:push_back(exp2 + 1)
        game_data.piece_removal_queue:push_back(j)
        board_changed = true

        if exp3 == exp4 then
          game_data.element_buffer[4]:move_up(game_data)
          game_data.element_buffer[4].command_queue:push_back(exp4 + 1)
          game_data.piece_removal_queue:push_back(j + 8)
          board_changed = true
        end

      elseif exp2 == exp3 then
        game_data.element_buffer[3]:move_up(game_data)
        game_data.element_buffer[3].command_queue:push_back(exp3 + 1)
        game_data.piece_removal_queue:push_back(j + 4)
        board_changed = true

      elseif exp3 == exp4 then
        game_data.element_buffer[4]:move_up(game_data)
        game_data.element_buffer[4].command_queue:push_back(exp4 + 1)
        game_data.piece_removal_queue:push_back(j + 8)
        board_changed = true
      end
    end

    ::continue::
  end

  return board_changed
end

function board.merge_down(game_data)
  local board_changed = false

  for j = 1, 4, 1 do
    -- Fill the element buffer with the pieces in the column
    game_data.element_buffer[1] = game_data.piece_list[j + 12]
    game_data.element_buffer[2] = game_data.piece_list[j + 8]
    game_data.element_buffer[3] = game_data.piece_list[j + 4]
    game_data.element_buffer[4] = game_data.piece_list[j]

    local num_elements = board.element_buffer_size(game_data)

    if num_elements == 0 or num_elements == 1 then
      goto continue

    elseif num_elements == 2 then
      local exp1 = game_data.element_buffer[1].exponent
      local exp2 = game_data.element_buffer[2].exponent

      if exp1 == exp2 then
        game_data.element_buffer[2]:move_down(game_data)
        game_data.element_buffer[2].command_queue:push_back(exp2 + 1)
        game_data.piece_removal_queue:push_back(j + 12)
        board_changed = true
      end

    elseif num_elements == 3 then
      local exp1 = game_data.element_buffer[1].exponent
      local exp2 = game_data.element_buffer[2].exponent
      local exp3 = game_data.element_buffer[3].exponent

      if exp1 == exp2 then
        game_data.element_buffer[2]:move_down(game_data)
        game_data.element_buffer[2].command_queue:push_back(exp2 + 1)
        game_data.piece_removal_queue:push_back(j + 12)
        board_changed = true

      elseif exp2 == exp3 then
        game_data.element_buffer[3]:move_down(game_data)
        game_data.element_buffer[3].command_queue:push_back(exp3 + 1)
        game_data.piece_removal_queue:push_back(j + 8)
        board_changed = true
      end

    elseif num_elements == 4 then
      local exp1 = game_data.element_buffer[1].exponent
      local exp2 = game_data.element_buffer[2].exponent
      local exp3 = game_data.element_buffer[3].exponent
      local exp4 = game_data.element_buffer[4].exponent

      if exp1 == exp2 then
        game_data.element_buffer[2]:move_down(game_data)
        game_data.element_buffer[2].command_queue:push_back(exp2 + 1)
        game_data.piece_removal_queue:push_back(j + 12)
        board_changed = true

        if exp3 == exp4 then
          game_data.element_buffer[4]:move_down(game_data)
          game_data.element_buffer[4].command_queue:push_back(exp4 + 1)
          game_data.piece_removal_queue:push_back(j + 4)
          board_changed = true
        end

      elseif exp2 == exp3 then
        game_data.element_buffer[3]:move_down(game_data)
        game_data.element_buffer[3].command_queue:push_back(exp3 + 1)
        game_data.piece_removal_queue:push_back(j + 8)
        board_changed = true

      elseif exp3 == exp4 then
        game_data.element_buffer[4]:move_down(game_data)
        game_data.element_buffer[4].command_queue:push_back(exp4 + 1)
        game_data.piece_removal_queue:push_back(j + 4)
        board_changed = true
      end
    end

    ::continue::
  end

  return board_changed
end

function board.merge_left(game_data)
  local board_changed = false

  for i = 1, 4, 1 do
    -- Fill the element buffer with the pieces in the column
    game_data.element_buffer[1] = game_data.piece_list[4 * i - 3]
    game_data.element_buffer[2] = game_data.piece_list[4 * i - 2]
    game_data.element_buffer[3] = game_data.piece_list[4 * i - 1]
    game_data.element_buffer[4] = game_data.piece_list[4 * i]

    local num_elements = board.element_buffer_size(game_data)

    if num_elements == 0 or num_elements == 1 then
      goto continue

    elseif num_elements == 2 then
      local exp1 = game_data.element_buffer[1].exponent
      local exp2 = game_data.element_buffer[2].exponent

      if exp1 == exp2 then
        game_data.element_buffer[2]:move_left(game_data)
        game_data.element_buffer[2].command_queue:push_back(exp2 + 1)
        game_data.piece_removal_queue:push_back(4 * i - 3)
        board_changed = true
      end

    elseif num_elements == 3 then
      local exp1 = game_data.element_buffer[1].exponent
      local exp2 = game_data.element_buffer[2].exponent
      local exp3 = game_data.element_buffer[3].exponent

      if exp1 == exp2 then
        game_data.element_buffer[2]:move_left(game_data)
        game_data.element_buffer[2].command_queue:push_back(exp2 + 1)
        game_data.piece_removal_queue:push_back(4 * i - 3)
        board_changed = true

      elseif exp2 == exp3 then
        game_data.element_buffer[3]:move_left(game_data)
        game_data.element_buffer[3].command_queue:push_back(exp3 + 1)
        game_data.piece_removal_queue:push_back(4 * i - 2)
        board_changed = true
      end

    elseif num_elements == 4 then
      local exp1 = game_data.element_buffer[1].exponent
      local exp2 = game_data.element_buffer[2].exponent
      local exp3 = game_data.element_buffer[3].exponent
      local exp4 = game_data.element_buffer[4].exponent

      if exp1 == exp2 then
        game_data.element_buffer[2]:move_left(game_data)
        game_data.element_buffer[2].command_queue:push_back(exp2 + 1)
        game_data.piece_removal_queue:push_back(4 * i - 3)
        board_changed = true

        if exp3 == exp4 then
          game_data.element_buffer[4]:move_left(game_data)
          game_data.element_buffer[4].command_queue:push_back(exp4 + 1)
          game_data.piece_removal_queue:push_back(4 * i - 1)
          board_changed = true
        end

      elseif exp2 == exp3 then
        game_data.element_buffer[3]:move_left(game_data)
        game_data.element_buffer[3].command_queue:push_back(exp3 + 1)
        game_data.piece_removal_queue:push_back(4 * i - 2)
        board_changed = true

      elseif exp3 == exp4 then
        game_data.element_buffer[4]:move_left(game_data)
        game_data.element_buffer[4].command_queue:push_back(exp4 + 1)
        game_data.piece_removal_queue:push_back(4 * i - 1)
        board_changed = true
      end
    end

    ::continue::
  end

  return board_changed
end

function board.merge_right(game_data)
  local board_changed = false

  for i = 1, 4, 1 do
    -- Fill the element buffer with the pieces in the column
    game_data.element_buffer[1] = game_data.piece_list[4 * i]
    game_data.element_buffer[2] = game_data.piece_list[4 * i - 1]
    game_data.element_buffer[3] = game_data.piece_list[4 * i - 2]
    game_data.element_buffer[4] = game_data.piece_list[4 * i - 3]

    local num_elements = board.element_buffer_size(game_data)

    if num_elements == 0 or num_elements == 1 then
      goto continue

    elseif num_elements == 2 then
      local exp1 = game_data.element_buffer[1].exponent
      local exp2 = game_data.element_buffer[2].exponent

      if exp1 == exp2 then
        game_data.element_buffer[2]:move_right(game_data)
        game_data.element_buffer[2].command_queue:push_back(exp2 + 1)
        game_data.piece_removal_queue:push_back(4 * i)
        board_changed = true
      end

    elseif num_elements == 3 then
      local exp1 = game_data.element_buffer[1].exponent
      local exp2 = game_data.element_buffer[2].exponent
      local exp3 = game_data.element_buffer[3].exponent

      if exp1 == exp2 then
        game_data.element_buffer[2]:move_right(game_data)
        game_data.element_buffer[2].command_queue:push_back(exp2 + 1)
        game_data.piece_removal_queue:push_back(4 * i)
        board_changed = true

      elseif exp2 == exp3 then
        game_data.element_buffer[3]:move_right(game_data)
        game_data.element_buffer[3].command_queue:push_back(exp3 + 1)
        game_data.piece_removal_queue:push_back(4 * i - 1)
        board_changed = true
      end

    elseif num_elements == 4 then
      local exp1 = game_data.element_buffer[1].exponent
      local exp2 = game_data.element_buffer[2].exponent
      local exp3 = game_data.element_buffer[3].exponent
      local exp4 = game_data.element_buffer[4].exponent

      if exp1 == exp2 then
        game_data.element_buffer[2]:move_right(game_data)
        game_data.element_buffer[2].command_queue:push_back(exp2 + 1)
        game_data.piece_removal_queue:push_back(4 * i)
        board_changed = true

        if exp3 == exp4 then
          game_data.element_buffer[4]:move_right(game_data)
          game_data.element_buffer[4].command_queue:push_back(exp4 + 1)
          game_data.piece_removal_queue:push_back(4 * i - 2)
          board_changed = true
        end

      elseif exp2 == exp3 then
        game_data.element_buffer[3]:move_right(game_data)
        game_data.element_buffer[3].command_queue:push_back(exp3 + 1)
        game_data.piece_removal_queue:push_back(4 * i - 1)
        board_changed = true

      elseif exp3 == exp4 then
        game_data.element_buffer[4]:move_right(game_data)
        game_data.element_buffer[4].command_queue:push_back(exp4 + 1)
        game_data.piece_removal_queue:push_back(4 * i - 2)
        board_changed = true
      end
    end

    ::continue::
  end

  return board_changed
end

return board
