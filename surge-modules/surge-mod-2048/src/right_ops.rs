use super::App2048;
use surge_sdk::nalgebra::Vector2 as Vec2;

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

#[cfg(test)]
mod tests {
    use crate::{app_2048::App2048, board_geometry::BoardGeometry, piece::Piece};
    use surge_sdk::nalgebra::Vector2 as Vec2;

    /// Builds an app with one piece per `(x, y)` slot, in order.
    fn app_with(slots: &[(usize, usize)]) -> App2048 {
        let bg = BoardGeometry::new();
        App2048::with_pieces(
            slots
                .iter()
                .map(|&(x, y)| Piece::new(&bg, 1, Vec2::new(x, y)))
                .collect(),
        )
    }

    /// Runs the move to completion and returns each piece's final slot.
    fn settle(app: &mut App2048) -> Vec<(usize, usize)> {
        for _ in 0..100 {
            if app.update_live_pieces_pos(1.0) {
                return app
                    .get_live_pieces()
                    .iter()
                    .map(|p| (p.get_c_slot()[0], p.get_c_slot()[1]))
                    .collect();
            }
        }
        panic!("pieces never settled");
    }

    #[test]
    fn single_piece_slides_to_the_right_edge() {
        let mut app = app_with(&[(0, 2)]);
        app.set_move_target_right();
        assert_eq!(settle(&mut app), vec![(3, 2)]);
    }

    #[test]
    fn two_pieces_pack_against_the_right_edge_in_order() {
        let mut app = app_with(&[(0, 1), (2, 1)]);
        app.set_move_target_right();
        assert_eq!(settle(&mut app), vec![(2, 1), (3, 1)]);
    }

    #[test]
    fn three_pieces_pack_against_the_right_edge_in_order() {
        // Listed out of x order to prove the order comes from the board, not the piece list.
        let mut app = app_with(&[(3, 0), (0, 0), (1, 0)]);
        app.set_move_target_right();
        assert_eq!(settle(&mut app), vec![(3, 0), (1, 0), (2, 0)]);
    }

    #[test]
    fn rows_are_independent_and_full_rows_do_not_move() {
        let mut app = app_with(&[(0, 0), (0, 1), (1, 1), (2, 1), (3, 1)]);
        app.set_move_target_right();
        assert_eq!(
            settle(&mut app),
            vec![(3, 0), (0, 1), (1, 1), (2, 1), (3, 1)]
        );
    }

    #[test]
    fn already_packed_pieces_stay_put() {
        let mut app = app_with(&[(3, 3), (2, 3)]);
        app.set_move_target_right();
        assert_eq!(settle(&mut app), vec![(3, 3), (2, 3)]);
    }
}
