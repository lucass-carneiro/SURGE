local piece = require("2048/piece")
--local inspect = require("2048/inspect")

local board = {}
board.__index = board
board.__name = "2048_board"

-- Size of a slot in the board
board.slot_size = 102.0

-- Slot positions
board.slots_x_pos = {9.0, 120.0, 231.0, 342.0, 9.0, 120.0, 231.0, 342.0, 9.0 , 120.0, 231.0, 342.0, 9.0, 120.0, 231.0, 342.0}
board.slots_y_pos = {9.0, 9.0, 9.0, 9.0, 120.0, 120.0, 120.0, 120.0, 231.0, 231.0, 231.0, 231.0, 342.0, 342.0, 342.0, 342.0}
board.slot_delta = (120.0 - 9.0)

-- Create board obj
function board:new()
  local b = {}
  setmetatable(b, board)

  b.surge_board_img = surge.image.new(
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

  -- Place initial pieces on the board
  board.new_piece(b)
  board.new_piece(b)

  return b
end

function board:reset()
  self.occupation_matrix = {
    {nil, nil, nil, nil},
    {nil, nil, nil, nil},
    {nil, nil, nil, nil},
    {nil, nil, nil, nil}
  }

  self:new_piece()
  self:new_piece()
end

function board:create_random_pos()
  math.randomseed(os.time())
  
  local exponent = math.random(1, 2)
  
  local i = math.random(1, 4)
  local j = math.random(1, 4)

  while self.occupation_matrix[i][j] ~= nil do
    i = math.random(1, 4)
    j = math.random(1, 4)
  end

  local I = ( (i - 1) * 4 + (j - 1) ) + 1, exponent

  return i, j, I, exponent
end

function board:new_piece()
  local i, j, I, exponent = self:create_random_pos()
  self.occupation_matrix[i][j] = piece:new(board.slots_x_pos[I], board.slots_y_pos[I], board.slot_size, board.slot_delta, exponent)
end

function board:draw()
  -- Draw board
  surge.image.draw(self.surge_board_img)

  -- Draw pieces
  for _,rows in pairs(self.occupation_matrix) do
    for _,p in pairs(rows) do
      p:draw()
    end
  end

end

function board:update(dt)
  -- Update board pieces
  for _,rows in pairs(self.occupation_matrix) do
    for _,p in pairs(rows) do
      p:update(dt)
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
          this_piece:shift_down()
        end
      end
    end
  end
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
          this_piece:shift_up()
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
          this_piece:shift_right()
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
          this_piece:shift_left()
        end
      end
    end
  end
end

-- Merges each column down, in pairs
function board:merge_down()
  for j=1,4,1 do
    for i=4,2,-1 do
      local this_piece = self.occupation_matrix[i][j]
      local next_piece = self.occupation_matrix[i - 1][j]
      
      if next_piece ~= nil and this_piece.exponent == next_piece.exponent then
        self.occupation_matrix[i][j].exponent = 2 * this_piece.exponent
        self.occupation_matrix[i - 1][j] = nil
      end
    end
  end
end

return board
