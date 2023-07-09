local queue = require("2048/queue")

local piece = {}
piece.__index = piece
piece.__name = "2048_piece"

function piece:__tostring()
  return "| exponent = " .. tostring(self.exponent) .. " | value = " ..
             tostring(2 ^ self.exponent) .. " | pos = (" .. tostring(self.i) ..
             "," .. tostring(self.j) .. "," .. tostring(self.I) .. ") |"
end

function piece:new(i, j, exponent, game_data)
  local p = {}
  setmetatable(p, piece)

  p.i = i
  p.j = j
  p.I = ((p.i - 1) * 4 + (p.j - 1)) + 1

  p.target_i = p.i
  p.target_j = p.j
  p.target_I = p.I

  p.exponent = exponent

  p.curent_slot_x = game_data.slots_x_pos[p.I]
  p.curent_slot_y = game_data.slots_y_pos[p.I]

  p.command_queue = queue:new()

  game_data.piece_list[p.I] = p

  return p
end

function piece:draw(game_data)
  surge.image.reset_position(game_data.piece_image_object, self.curent_slot_x,
                             self.curent_slot_y, 0.1)
  surge.image.draw_region(game_data.piece_image_object,

                          game_data.texture_origin_x[self.exponent],
                          game_data.texture_origin_y[self.exponent],

                          game_data.slot_size, game_data.slot_size)
end

function piece:move_up(game_data, slots)
  slots = slots or 1

  if slots == 0 then return end

  self.target_i = self.i - slots
  self.target_I = ((self.target_i - 1) * 4 + (self.j - 1)) + 1

  self.command_queue:push_back(game_data.commands.move_v)
end

function piece:move_down(game_data, slots)
  slots = slots or 1

  if slots == 0 then return end

  self.target_i = self.i + slots
  self.target_I = ((self.target_i - 1) * 4 + (self.j - 1)) + 1

  self.command_queue:push_back(game_data.commands.move_v)
end

function piece:move_left(game_data, slots)
  slots = slots or 1

  if slots == 0 then return end

  self.target_j = self.j - slots
  self.target_I = ((self.i - 1) * 4 + (self.target_j - 1)) + 1

  self.command_queue:push_back(game_data.commands.move_h)
end

function piece:move_right(game_data, slots)
  slots = slots or 1

  if slots == 0 then return end

  self.target_j = self.j + slots
  self.target_I = ((self.i - 1) * 4 + (self.target_j - 1)) + 1

  self.command_queue:push_back(game_data.commands.move_h)
end

function piece:update(dt, game_data)
  local command = self.command_queue:front()

  -- Piece exponent changes
  if command ~= nil and 1 <= command and command <= 11 then
    self.exponent = command
    self.command_queue:pop_front()
    return
  end

  -- Piece motions
  if command == game_data.commands.move_v then
    local target_y = game_data.slots_y_pos[self.target_I]

    local dy = target_y - self.curent_slot_y
    local abs_dy = math.abs(dy)
    local dy_norm = dy / abs_dy

    if math.floor(abs_dy) > game_data.motion_threshold then
      surge.image.move(game_data.piece_image_object, 0.0,
                       dy_norm * game_data.shift_speed * dt, 0.0)
      self.curent_slot_y =
          self.curent_slot_y + dy_norm * game_data.shift_speed * dt
    else
      self.i = self.target_i
      self.I = self.target_I

      self.curent_slot_y = target_y

      self.command_queue:pop_front()
    end
  elseif command == game_data.commands.move_h then
    local target_x = game_data.slots_x_pos[self.target_I]

    local dx = target_x - self.curent_slot_x
    local abs_dx = math.abs(dx)
    local dx_norm = dx / abs_dx

    if math.floor(abs_dx) > game_data.motion_threshold then
      surge.image.move(game_data.piece_image_object,
                       dx_norm * game_data.shift_speed * dt, 0.0, 0.0)
      self.curent_slot_x =
          self.curent_slot_x + dx_norm * game_data.shift_speed * dt
    else
      self.j = self.target_j
      self.I = self.target_I

      self.curent_slot_x = target_x

      self.command_queue:pop_front()
    end
  end
end

return piece
