local queue = require("2048/queue")
local game_data = {}

-- Slot positions
game_data.slots_x_pos = {
  9.0, 120.0, 231.0, 342.0, 9.0, 120.0, 231.0, 342.0, 9.0, 120.0, 231.0, 342.0,
  9.0, 120.0, 231.0, 342.0,
}
game_data.slots_y_pos = {
  9.0, 9.0, 9.0, 9.0, 120.0, 120.0, 120.0, 120.0, 231.0, 231.0, 231.0, 231.0,
  342.0, 342.0, 342.0, 342.0,
}

-- Size of a slot in the board
game_data.slot_size = 102.0

-- Distance between two slots
game_data.slot_delta = game_data.slots_x_pos[2] - game_data.slots_x_pos[1]

-- Origins for the textures of each number within the piece texture image
game_data.texture_origin_x = {
  1.0, 104.0, 207.0, 310.0, 1.0, 104.0, 207.0, 310.0, 1.0, 104.0, 207.0,
}
game_data.texture_origin_y = {
  1.0, 1.0, 1.0, 1.0, 104.0, 104.0, 104.0, 104.0, 207.0, 207.0, 207.0,
}

-- Threshold for using when determining if a piece reached a target
game_data.motion_threshold = 1.5

-- The speed at which a piece moves through the board
game_data.shift_speed = 5.0 * game_data.slot_delta

-- The piece image object, shared for all pieces
game_data.board_image_object = nil

-- The piece image object, shared for all pieces
game_data.piece_image_object = nil

-- The commands a piece can execute. 1-11 are Reserved for exponents
game_data.commands = {move_v = 12, move_h = 13}

-- Represents the game board and the pieces in it
game_data.piece_list = nil
game_data.piece_list_buffer = nil

-- A buffer for storing lines or rows for compression/merging processing
game_data.element_buffer = nil

-- Possible game states
game_data.game_states = {
  compress_up = 1,
  compress_down = 2,
  compress_left = 3,
  compress_right = 4,
  merge_up = 5,
  merge_down = 6,
  merge_left = 7,
  merge_right = 8,
  piece_removal = 9,
  add_piece = 10,
  print_piece_list = 11,
}

-- Game states queue
game_data.game_state_queue = queue:new()

-- Pieces to be removed from the piece list
game_data.piece_removal_queue = queue:new()

-- Dump piece movement data and use debug board
game_data.debug = true
game_data.debug_file = game_data.debug and io.open("2048_debug_log.txt", "w") or
                           nil

return game_data
