use super::App2048;
use nalgebra::Vector2 as Vec2;

impl App2048 {
    pub(crate) fn set_move_target_right(&mut self) {
        let board = self.reconstruct_board();

        // Loop over board rows
        for row in board {
            let num_pieces = row.iter().filter_map(|&x| x).count();

            match num_pieces {
                0 => continue,

                1 => {
                    let idx_0 = row.iter().filter_map(|&x| x).nth(0).unwrap();

                    let t_slot_0 = Vec2::new(3, self.get_live_pieces()[idx_0].get_c_slot()[1]);

                    self.set_piece_move_target(idx_0, t_slot_0);
                }

                2 => {
                    let idx_0 = row.iter().filter_map(|&x| x).nth(0).unwrap();
                    let idx_1 = row.iter().filter_map(|&x| x).nth(1).unwrap();

                    let t_slot_0 = Vec2::new(2, self.get_live_pieces()[idx_0].get_c_slot()[1]);
                    let t_slot_1 = Vec2::new(3, self.get_live_pieces()[idx_1].get_c_slot()[1]);

                    self.set_piece_move_target(idx_0, t_slot_0);
                    self.set_piece_move_target(idx_1, t_slot_1);
                }

                3 => {
                    let idx_0 = row.iter().filter_map(|&x| x).nth(0).unwrap();
                    let idx_1 = row.iter().filter_map(|&x| x).nth(1).unwrap();
                    let idx_2 = row.iter().filter_map(|&x| x).nth(2).unwrap();

                    let t_slot_0 = Vec2::new(1, self.get_live_pieces()[idx_0].get_c_slot()[1]);
                    let t_slot_1 = Vec2::new(2, self.get_live_pieces()[idx_1].get_c_slot()[1]);
                    let t_slot_2 = Vec2::new(3, self.get_live_pieces()[idx_2].get_c_slot()[1]);

                    self.set_piece_move_target(idx_0, t_slot_0);
                    self.set_piece_move_target(idx_1, t_slot_1);
                    self.set_piece_move_target(idx_2, t_slot_2);
                }

                4 => continue,

                other => {
                    log::warn!(
                        "Unrecognized number of pieces in board row: {}. This should never have happened, and yet, it did. We can continue on, but things may become irrecoverable down the line.",
                        other
                    );
                }
            }
        }
    }
}
