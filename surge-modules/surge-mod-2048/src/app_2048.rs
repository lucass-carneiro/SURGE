use crate::{board_geometry::BoardGeometry, piece::Piece};
use std::collections::VecDeque;
use surge_sdk::nalgebra::Vector2 as Vec2;

#[derive(Debug, PartialEq)]
pub(crate) enum MoveDirection {
    Up,
    Down,
    Left,
    Right,
}

#[derive(Debug, PartialEq)]
pub(crate) enum BoardState {
    Idle,
    Move(MoveDirection),
    Resolve,
}

pub(crate) struct App2048 {
    // Board geometric data
    board_geometry: BoardGeometry,

    // Sequence of states to execute
    board_states: VecDeque<BoardState>,

    // Live pieces on the board
    live_pieces: Vec<Piece>,
}

impl App2048 {
    pub(crate) fn new() -> Self {
        let board_geometry = BoardGeometry::new();

        let mut board_states = VecDeque::new();
        board_states.push_back(BoardState::Idle);

        let piece_a = Piece::new_random(&board_geometry);
        let mut piece_b = Piece::new_random(&board_geometry);

        while piece_a.get_c_slot() == piece_b.get_c_slot() {
            piece_b = Piece::new_random(&board_geometry);
        }

        let mut live_pieces = Vec::new();
        live_pieces.push(piece_a);
        live_pieces.push(piece_b);

        App2048 {
            board_geometry,
            board_states,
            live_pieces,
        }
    }

    #[cfg(test)]
    pub(crate) fn with_pieces(live_pieces: Vec<Piece>) -> Self {
        let mut app = App2048::new();
        app.live_pieces = live_pieces;
        app
    }

    pub(crate) fn board_idle(&self) -> bool {
        match self.board_states.front() {
            Some(s) => *s == BoardState::Idle,
            None => true,
        }
    }

    pub(crate) fn reconstruct_board(&self) -> [[Option<usize>; 4]; 4] {
        let mut board: [[Option<usize>; 4]; 4] = [
            [None, None, None, None],
            [None, None, None, None],
            [None, None, None, None],
            [None, None, None, None],
        ];

        for (idx, piece) in self.live_pieces.iter().enumerate() {
            let c_slot = piece.get_c_slot();
            board[c_slot[1]][c_slot[0]] = Some(idx);
        }

        board
    }

    pub(crate) fn update_live_pieces_pos(&mut self, dt: f32) -> bool {
        let mut update_done = true;
        for piece in &mut self.live_pieces {
            update_done &= piece.update_pos(dt);
        }
        update_done
    }

    pub(crate) fn set_piece_move_target(&mut self, idx: usize, slot: Vec2<usize>) {
        self.live_pieces[idx].set_move_target(&self.board_geometry, slot);
    }

    pub(crate) fn get_board_geometry(&self) -> &BoardGeometry {
        &self.board_geometry
    }

    pub(crate) fn get_live_pieces(&self) -> &Vec<Piece> {
        &self.live_pieces
    }

    pub(crate) fn get_board_states(&self) -> &VecDeque<BoardState> {
        &self.board_states
    }

    pub(crate) fn push_state(&mut self, state: BoardState) {
        self.board_states.push_back(state);
    }

    pub(crate) fn pop_state(&mut self) {
        self.board_states.pop_front();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn reconstruct_board_indexes_by_row_then_column() {
        let bg = BoardGeometry::new();
        let app = App2048::with_pieces(vec![
            Piece::new(&bg, 1, Vec2::new(2, 1)), // x = 2, y = 1
            Piece::new(&bg, 2, Vec2::new(0, 3)),
        ]);

        let board = app.reconstruct_board();

        assert_eq!(board[1][2], Some(0));
        assert_eq!(board[3][0], Some(1));
        assert_eq!(board.iter().flatten().filter(|s| s.is_some()).count(), 2);
    }

    #[test]
    fn new_app_starts_idle_with_two_distinct_pieces() {
        for _ in 0..50 {
            let app = App2048::new();
            assert!(app.board_idle());
            assert_eq!(app.get_live_pieces().len(), 2);
            assert_ne!(
                app.get_live_pieces()[0].get_c_slot(),
                app.get_live_pieces()[1].get_c_slot()
            );
        }
    }

    #[test]
    fn an_empty_queue_counts_as_idle() {
        let mut app = App2048::new();
        app.pop_state();
        assert!(app.board_idle());
    }

    #[test]
    fn input_is_refused_while_a_move_is_in_flight() {
        let mut app = App2048::new();
        app.push_state(BoardState::Move(MoveDirection::Right));
        app.pop_state(); // Drop the initial Idle so Move is at the front.
        assert!(!app.board_idle());
    }
}
