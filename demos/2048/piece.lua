board_data = require("2048/board_data")
queue = require("2048/queue")

local piece = {}
piece.__index = piece
piece.__name = "2048_piece"

-- Origins for the textures of each number within the piece texture image
piece.texture_origin_x = {1.0, 104.0, 207.0, 310.0, 1.0, 104.0, 207.0, 310.0, 1.0, 104.0, 207.0}
piece.texture_origin_y = {1.0, 1.0, 1.0, 1.0, 104.0, 104.0, 104.0, 104.0, 207.0, 207.0, 207.0}

-- Threshold for using when determining if a piece reached a target
piece.motion_threshold = 1.5

-- The speed at which a piece moves through the board
piece.shift_speed = 5.0 * board_data.slot_delta

-- The piece image object, shared for all pieces
piece.image_object = nil

-- The commands a piece can execute. 1-11 are Reserved for exponents
piece.commands = {
  move_u = 12,
  move_d = 13,
  move_l = 14,
  move_r = 15,
  sync_down = 16
}

function piece:load_texture()
  piece.image_object = surge.image.new(
    "2048/resources/pieces.png",
    board_data.slots_x_pos[1], board_data.slots_y_pos[1], 0.1,
    board_data.slot_size     , board_data.slot_size     , 1.0
  )
end

function piece:new(i, j, exponent)
  local p = {}
  setmetatable(p, piece)

  p.i = i
  p.j = j
  p.I = ((p.i - 1) * 4 + (p.j - 1)) + 1
  
  p.exponent = exponent

  p.curent_slot_x = board_data.slots_x_pos[p.I]
  p.curent_slot_y = board_data.slots_y_pos[p.I]
  
  p.command_queue = queue:new()
 
  return p
end

function piece:move_up()
  self.command_queue:push_back(piece.commands.move_u)
end

function piece:move_down()
  self.command_queue:push_back(piece.commands.move_d)
end

function piece:move_left()
  self.command_queue:push_back(piece.commands.move_l)
end

function piece:move_right()
  self.command_queue:push_back(piece.commands.move_r)
end

function piece:double_exponent()
  self.command_queue:push_back(self.exponent + 1)
end

function piece:sync_down()
  self.command_queue:push_back(piece.commands.sync_down)
end

function piece:draw()
  surge.image.reset_position(piece.image_object, self.curent_slot_x, self.curent_slot_y, 0.1)
  surge.image.draw_region(
    piece.image_object,
       
    piece.texture_origin_x[self.exponent],
    piece.texture_origin_y[self.exponent],
        
    board_data.slot_size,
    board_data.slot_size
  )
end

function piece:update(dt, occupation_matrix)
  local command = self.command_queue:front()

  -- Sync ccupation matrix
  if command == piece.commands.sync_down then
    self.command_queue:pop_front()
    occupation_matrix[self.i - 1][self.j] = nil
    occupation_matrix[self.i][self.j] = self
    return
  end

  -- Piece exponent changes
  if command ~= nil and 1 <= command and command <= 11 then
    self.exponent = command
    self.command_queue:pop_front()
    return
  end

  -- Piece motions
  if command == piece.commands.move_u then
    local target_i = self.i - 1
    local target_I = ((target_i - 1) * 4 + (self.j - 1)) + 1
    
    local target_y = board_data.slots_y_pos[target_I]

    local dy = target_y - self.curent_slot_y
    local abs_dy = math.abs(dy)
    local dy_norm = dy / abs_dy

    if abs_dy > self.motion_threshold then
      surge.image.move(piece.image_object, 0.0, dy_norm * self.shift_speed * dt, 0.0)
      self.curent_slot_y = self.curent_slot_y + dy_norm * self.shift_speed * dt
    else
      self.i = target_i
      self.I = target_I

      self.curent_slot_y = target_y

      self.command_queue:pop_front()
    end

  elseif command == piece.commands.move_d then
    local target_i = self.i + 1
    local target_I = ((target_i - 1) * 4 + (self.j - 1)) + 1
    
    local target_y = board_data.slots_y_pos[target_I]

    local dy = target_y - self.curent_slot_y
    local abs_dy = math.abs(dy)
    local dy_norm = dy / abs_dy

    if abs_dy > self.motion_threshold then
      surge.image.move(piece.image_object, 0.0, dy_norm * self.shift_speed * dt, 0.0)
      self.curent_slot_y = self.curent_slot_y + dy_norm * self.shift_speed * dt
    else
      self.i = target_i
      self.I = target_I

      self.curent_slot_y = target_y

      self.command_queue:pop_front()
    end

  elseif command == piece.commands.move_l then
    local target_j = self.j - 1
    local target_I = ((self.i - 1) * 4 + (target_j - 1)) + 1
    
    local target_x = board_data.slots_x_pos[target_I]

    local dx = target_x - self.curent_slot_x
    local abs_dx = math.abs(dx)
    local dx_norm = dx / abs_dx

    if abs_dx > self.motion_threshold then
      surge.image.move(piece.image_object, dx_norm * self.shift_speed * dt, 0.0, 0.0)
      self.curent_slot_x = self.curent_slot_x + dx_norm * self.shift_speed * dt
    else
      self.j = target_j
      self.I = target_I

      self.curent_slot_x = target_x

      self.command_queue:pop_front()
    end

  elseif command == piece.commands.move_r then
    local target_j = self.j + 1
    local target_I = ((self.i - 1) * 4 + (target_j - 1)) + 1
    
    local target_x = board_data.slots_x_pos[target_I]

    local dx = target_x - self.curent_slot_x
    local abs_dx = math.abs(dx)
    local dx_norm = dx / abs_dx

    if abs_dx > self.motion_threshold then
      surge.image.move(piece.image_object, dx_norm * self.shift_speed * dt, 0.0, 0.0)
      self.curent_slot_x = self.curent_slot_x + dx_norm * self.shift_speed * dt
    else
      self.j = target_j
      self.I = target_I

      self.curent_slot_x = target_x

      self.command_queue:pop_front()
    end
  end
end

return piece