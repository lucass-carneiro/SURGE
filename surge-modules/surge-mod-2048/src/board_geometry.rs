use surge_sdk::nalgebra::Vector2 as Vec2;

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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn slot_to_pos_maps_corner_slots() {
        let bg = BoardGeometry::new();
        assert_eq!(bg.slot_to_pos(Vec2::new(0, 0)), Vec2::new(15.0, 315.0));
        assert_eq!(bg.slot_to_pos(Vec2::new(3, 3)), Vec2::new(378.0, 678.0));
    }

    #[test]
    fn x_and_y_are_not_swapped() {
        let bg = BoardGeometry::new();
        let pos = bg.slot_to_pos(Vec2::new(1, 0));
        assert!(pos[0] > bg.slot_to_pos(Vec2::new(0, 0))[0]);
        assert_eq!(pos[1], bg.slot_to_pos(Vec2::new(0, 0))[1]);
    }

    #[test]
    fn every_slot_fits_inside_the_500x800_window() {
        let bg = BoardGeometry::new();
        for x in 0..4 {
            for y in 0..4 {
                let pos = bg.slot_to_pos(Vec2::new(x, y));
                assert!(pos[0] >= 0.0 && pos[0] + bg.get_piece_dims() <= 500.0);
                assert!(pos[1] >= 0.0 && pos[1] + bg.get_piece_dims() <= 800.0);
            }
        }
    }

    #[test]
    fn pieces_draw_in_front_of_the_board() {
        // Larger z draws behind (depth test is GREATER_OR_EQUAL).
        let bg = BoardGeometry::new();
        assert!(bg.get_piece_z() < bg.get_board_z());
    }
}
