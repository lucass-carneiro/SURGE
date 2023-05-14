board_data = require("2048/board_data")

local piece = {}
piece.__index = piece
piece.__name = "2048_piece"

-- Origins for the textures of each number within the piece texture image
piece.texture_origin_x = {1.0, 104.0, 207.0, 310.0, 1.0, 104.0, 207.0, 310.0, 1.0, 104.0, 207.0,}
piece.texture_origin_y = {1.0, 1.0, 1.0, 1.0, 104.0, 104.0, 104.0, 104.0, 207.0, 207.0, 207.0}

-- Threshold for using when determining if a piece reached a target
piece.motion_threshold = 1.5

-- The speed at which a piece moves through the board
piece.shift_speed = 5.0 * board_data.slot_delta

-- The states of a piece
piece.states = {}
piece.states.idle = 0
piece.states.moving = 1

piece.image_object = nil

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
  
  p.target_slot_x = p.curent_slot_x
  p.target_slot_y = p.curent_slot_y
  

  p.state = piece.states.idle
 
  return p
end

function piece:move_up()
  self.target_slot_y = self.target_slot_y - board_data.slot_delta  
  self.i = self.i - 1
  self.I = ((self.i - 1) * 4 + (self.j - 1)) + 1
end

function piece:move_down()
  self.target_slot_y = self.target_slot_y + board_data.slot_delta
  self.i = self.i + 1
  self.I = ((self.i - 1) * 4 + (self.j - 1)) + 1
end

function piece:move_left()
  self.target_slot_x = self.target_slot_x - board_data.slot_delta  
  self.j = self.j - 1
  self.I = ((self.i - 1) * 4 + (self.j - 1)) + 1
end

function piece:move_right()
  self.target_slot_x = self.target_slot_x + board_data.slot_delta  
  self.j = self.j + 1
  self.I = ((self.i - 1) * 4 + (self.j - 1)) + 1
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

function piece:update(dt)
  local dx = self.target_slot_x - self.curent_slot_x
  local abs_dx = math.abs(dx)
  local dx_norm = dx / abs_dx  

  local dy = self.target_slot_y - self.curent_slot_y
  local abs_dy = math.abs(dy)
  local dy_norm = dy / abs_dy
    
  if abs_dy > self.motion_threshold then
    surge.image.move(piece.image_object, 0.0, dy_norm * self.shift_speed * dt, 0.0)
    self.curent_slot_y = self.curent_slot_y + dy_norm * self.shift_speed * dt
    self.state = piece.states.moving
  elseif abs_dx > self.motion_threshold then
    surge.image.move(self.image_object, dx_norm * self.shift_speed * dt, 0.0, 0.0)
    self.curent_slot_x = self.curent_slot_x + dx_norm * self.shift_speed * dt
    self.state = piece.states.moving
  else
    self.curent_slot_x = self.target_slot_x
    self.curent_slot_y = self.target_slot_y
    self.state = piece.states.idle
  end
end

return piece