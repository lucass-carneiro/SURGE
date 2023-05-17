piece = require("2048/piece")
local inspect = require("2048/inspect")

local board = {}
board.__index = board
board.__name = "2048_board"

function board:new()
  local b = {}
  setmetatable(b, board)

  b.image_object = surge.image.new(
    "2048/resources/board.png",
    0.0, 0.0, 0.0,
    surge.config.window_width, surge.config.window_width, 1.0
  )

  b.occupation_matrix = {
    {nil, nil, nil, nil},
    {nil, nil, nil, nil},
    {nil, nil, nil, nil},
    {nil, nil, nil, nil}
  }

  b.add_new_piece = false

  return b
end

function board:reset()
  self.occupation_matrix = {
    {nil, nil, nil, nil},
    {nil, nil, nil, nil},
    {nil, nil, nil, nil},
    {nil, nil, nil, nil}
  }

  self.add_new_piece = false
end

function board:draw()
  surge.image.draw(self.image_object)

  for _,rows in pairs(self.occupation_matrix) do
    for _,p in pairs(rows) do
      p:draw()
    end
  end

end

function board:update(dt)
  for _,rows in pairs(self.occupation_matrix) do
    for _,p in pairs(rows, self.occupation_matrix) do
        p:update(dt, self.occupation_matrix)
    end
  end

  if self.add_new_piece and self:is_idle() then
    self:new_piece()
    self.add_new_piece = false
  end
end

function board:new_piece()
  math.randomseed(os.time())
  
  local exponent = math.random(1, 2)
  
  local i = math.random(1, 4)
  local j = math.random(1, 4)

  while self.occupation_matrix[i][j] ~= nil do
    i = math.random(1, 4)
    j = math.random(1, 4)
  end

  self.occupation_matrix[i][j] = piece:new(i, j, exponent)
end

function board:compress_up()
  for j=1,4,1 do
    for i=1,4,1 do
      for k=4,2,-1 do
        local this_piece = self.occupation_matrix[k][j]
        local next_piece = self.occupation_matrix[k - 1][j]
    
        if this_piece ~= nil and next_piece == nil then
          self.occupation_matrix[k][j] = nil
          self.occupation_matrix[k - 1][j] = this_piece
          this_piece:move_up()
        end
      end
    end
  end
end

function board:compress_down()
  for j=1,4,1 do
    for i=1,4,1 do
      for k=1,3,1 do
        local this_piece = self.occupation_matrix[k][j]
        local next_piece = self.occupation_matrix[k + 1][j]
    
        if this_piece ~= nil and next_piece == nil then
          self.occupation_matrix[k][j] = nil
          self.occupation_matrix[k + 1][j] = this_piece
          this_piece:move_down()
        end
      end
    end
  end
end

function board:compress_left()
  for i=1,4,1 do
    for j=1,4,1 do
      for k=4,2,-1 do
        local this_piece = self.occupation_matrix[i][k]
        local next_piece = self.occupation_matrix[i][k - 1]
    
        if this_piece ~= nil and next_piece == nil then
          self.occupation_matrix[i][k] = nil
          self.occupation_matrix[i][k - 1] = this_piece
          this_piece:move_left()
        end
      end
    end
  end
end

function board:compress_right()
  for i=1,4,1 do
    for j=1,4,1 do
      for k=1,3,1 do
        local this_piece = self.occupation_matrix[i][k]
        local next_piece = self.occupation_matrix[i][k + 1]
    
        if this_piece ~= nil and next_piece == nil then
          self.occupation_matrix[i][k] = nil
          self.occupation_matrix[i][k + 1] = this_piece
          this_piece:move_right()
        end
      end
    end
  end
end

function board:merge_up()
  for j=1,4,1 do
    for i=1,3,1 do
      local this_piece = self.occupation_matrix[i][j]
      local next_piece = self.occupation_matrix[i + 1][j]
      
      if this_piece ~= nil and next_piece ~= nil and this_piece.exponent == next_piece.exponent then
        next_piece:move_up()
        next_piece:double_exponent()
        next_piece:sync_up()
      end
    end
  end
end

function board:merge_down()
  for j=1,4,1 do
    for i=4,2,-1 do
      local this_piece = self.occupation_matrix[i][j]
      local next_piece = self.occupation_matrix[i - 1][j]
      
      if this_piece ~= nil and next_piece ~= nil and this_piece.exponent == next_piece.exponent then
        next_piece:move_down()
        next_piece:double_exponent()
        next_piece:sync_down()
      end
    end
  end
end

function board:merge_left()
  for i=1,4,1 do
    for j=1,3,1 do
      local this_piece = self.occupation_matrix[i][j]
      local next_piece = self.occupation_matrix[i][j + 1]
      
      if this_piece ~= nil and next_piece ~= nil and this_piece.exponent == next_piece.exponent then
        next_piece:move_left()
        next_piece:double_exponent()
        next_piece:sync_left()
      end
    end
  end
end

function board:merge_right()
  for i=1,4,1 do
    for j=4,2,-1 do
      local this_piece = self.occupation_matrix[i][j]
      local next_piece = self.occupation_matrix[i][j - 1]
      
      if this_piece ~= nil and next_piece ~= nil and this_piece.exponent == next_piece.exponent then
        next_piece:move_right()
        next_piece:double_exponent()
        next_piece:sync_right()
      end
    end
  end
end

function board:is_idle()
  local idle = true

  for _,rows in pairs(self.occupation_matrix) do
    for _,p in pairs(rows) do
      idle = idle and p.command_queue:front() == nil
    end
  end

  return idle
end

function board:game_move_up()
  self:compress_up()
  self:merge_up()
  self:compress_up()
  self.add_new_piece = true
end

function board:game_move_down()
  self:compress_down()
  self:merge_down()
  self:compress_down()
  self.add_new_piece = true
end

function board:game_move_left()
  self:compress_left()
  self:merge_left()
  self:compress_left()
  self.add_new_piece = true
end

function board:game_move_right()
  self:compress_right()
  self:merge_right()
  self:compress_right()
  self.add_new_piece = true
end

return board
