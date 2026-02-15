use nalgebra::Vector2 as Vec2;

pub(crate) struct BoardGeometry {
    piece_z: f32,
    board_z: f32,
    piece_dims: f32,
    x_slot_to_pos: [f32; 4],
    y_slot_to_pos: [f32; 4],
}

impl BoardGeometry {
    pub(crate) fn new() -> Self {
        let slot_dims = 121.0;
        let x_slot_base = 15.0;
        let y_slot_base = 315.0;

        BoardGeometry {
            piece_z: 0.0,
            board_z: 0.5,
            piece_dims: 105.0,
            x_slot_to_pos: [
                x_slot_base + 0.0 * slot_dims,
                x_slot_base + 1.0 * slot_dims,
                x_slot_base + 2.0 * slot_dims,
                x_slot_base + 3.0 * slot_dims,
            ],
            y_slot_to_pos: [
                y_slot_base + 0.0 * slot_dims,
                y_slot_base + 1.0 * slot_dims,
                y_slot_base + 2.0 * slot_dims,
                y_slot_base + 3.0 * slot_dims,
            ],
        }
    }

    pub(crate) fn slot_to_pos(&self, slot: Vec2<usize>) -> Vec2<f32> {
        Vec2::new(self.x_slot_to_pos[slot[0]], self.y_slot_to_pos[slot[1]])
    }

    pub(crate) fn get_piece_dims(&self) -> f32 {
        self.piece_dims
    }

    pub(crate) fn get_board_z(&self) -> f32 {
        self.board_z
    }

    pub(crate) fn get_piece_z(&self) -> f32 {
        self.piece_z
    }
}
