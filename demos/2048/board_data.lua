local board_data = {}

-- Slot positions
board_data.slots_x_pos = {9.0, 120.0, 231.0, 342.0, 9.0, 120.0, 231.0, 342.0, 9.0 , 120.0, 231.0, 342.0, 9.0, 120.0, 231.0, 342.0}
board_data.slots_y_pos = {9.0, 9.0, 9.0, 9.0, 120.0, 120.0, 120.0, 120.0, 231.0, 231.0, 231.0, 231.0, 342.0, 342.0, 342.0, 342.0}

-- Size of a slot in the board
board_data.slot_size = 102.0

-- Distance between two slots
board_data.slot_delta = board_data.slots_x_pos[2] - board_data.slots_x_pos[1]

return board_data