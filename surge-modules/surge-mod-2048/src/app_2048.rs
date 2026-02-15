use crate::{board_geometry::BoardGeometry, piece::Piece};
use nalgebra::Vector2 as Vec2;
use std::collections::VecDeque;

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

        while piece_a == piece_b {
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

    pub(crate) fn board_idle(&self) -> bool {
        match self.board_states.front() {
            Some(s) => *s == BoardState::Idle,
            None => true,
        }
    }

    pub(crate) fn reconstruct_board_row_major(&self) -> [[Option<usize>; 4]; 4] {
        let mut board: [[Option<usize>; 4]; 4] = [
            [None, None, None, None],
            [None, None, None, None],
            [None, None, None, None],
            [None, None, None, None],
        ];

        for (idx, piece) in self.live_pieces.iter().enumerate() {
            let (i, j) = (piece.get_c_slot()[0], piece.get_c_slot()[1]);
            board[j][i] = Some(idx);
        }

        board
    }

    pub(crate) fn update_live_pieces_pos(&mut self) -> bool {
        let mut update_done = true;
        for piece in &mut self.live_pieces {
            update_done &= piece.update_pos();
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
