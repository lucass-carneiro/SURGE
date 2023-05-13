local states = require("2048/piece_states")

local piece = {}
piece.__index = piece
piece.__name = "2048_piece"

piece.texture_origin_x = {1.0, 104.0, 207.0, 310.0, 1.0, 104.0, 207.0, 310.0, 1.0, 104.0, 207.0,}
piece.texture_origin_y = {1.0, 1.0, 1.0, 1.0, 104.0, 104.0, 104.0, 104.0, 207.0, 207.0, 207.0}

function piece:new(slot_x, slot_y, slot_size, slot_delta, exponent)
  local p = {}
  setmetatable(p, piece)    

  p.surge_image_object = surge.image.new(
    "2048/resources/pieces.png",
    slot_x  , slot_y   , 0.1,
    slot_size, slot_size, 1.0
  )

  p.curent_slot_x = slot_x
  p.curent_slot_y = slot_y
  
  p.target_slot_x = p.curent_slot_x
  p.target_slot_y = p.curent_slot_y
  

  p.exponent = exponent
  p.slot_size = slot_size

  p.slot_delta = slot_delta
  p.shift_speed = 5.0 * slot_delta

  p.motion_threshold = 1.5

  p.state = nil
 
  return p
end

function piece:draw()
  surge.image.draw_region(
    self.surge_image_object,
       
    piece.texture_origin_x[self.exponent],
    piece.texture_origin_y[self.exponent],
        
    self.slot_size,
    self.slot_size
  )
end

function piece:shift_up()
  self.target_slot_y = self.target_slot_y - self.slot_delta  
end

function piece:shift_down()
  self.target_slot_y = self.target_slot_y + self.slot_delta
end

function piece:shift_right()
  self.target_slot_x = self.target_slot_x + self.slot_delta  
end

function piece:shift_left()
  self.target_slot_x = self.target_slot_x - self.slot_delta  
end

function piece:moving()
  local abs_dx = math.abs(self.target_slot_y - self.curent_slot_y)
  local abs_dy = math.abs(self.target_slot_y - self.curent_slot_y)

  if abs_dx > self.motion_threshold or abs_dy > motion_threshold then
    return true
  else
    return false
  end
end

function piece:update(dt)
  -- Vertical motion
  if self.state == states.compress_up or self.state == states.compress_down then
    local dy = self.target_slot_y - self.curent_slot_y
    local abs_dy = math.abs(dy)
    local dy_norm = dy / abs_dy  
    
    if abs_dy > self.motion_threshold then
      surge.image.move(self.surge_image_object, 0.0, dy_norm * self.shift_speed * dt, 0.0)
      self.curent_slot_y = self.curent_slot_y + dy_norm * self.shift_speed * dt
    else
      self.curent_slot_y = self.target_slot_y
      self.state = nil
    end
  end

  -- Horizontal motion
  if self.state == states.compress_left or self.state == states.compress_right then
    local dx = self.target_slot_x - self.curent_slot_x
    local abs_dx = math.abs(dx)
    local dx_norm = dx / abs_dx  
    
    if abs_dx > self.motion_threshold then
      surge.image.move(self.surge_image_object, dx_norm * self.shift_speed * dt, 0.0, 0.0)
      self.curent_slot_x = self.curent_slot_x + dx_norm * self.shift_speed * dt
    else
      self.curent_slot_x = self.target_slot_x
      self.state = nil
    end
  end
end

return piece, states