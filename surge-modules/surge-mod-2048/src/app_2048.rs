use crate::{
    board_geometry::BoardGeometry,
    new_game_button::NewGameButton,
    piece::{MergeRole, Piece},
    score_display::ScoreDisplay,
};
use rand::seq::IndexedRandom;
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
    Spawn,
    GameOver,
}

pub(crate) struct App2048 {
    // Board geometric data
    board_geometry: BoardGeometry,

    // Sequence of states to execute
    board_states: VecDeque<BoardState>,

    // Live pieces on the board
    live_pieces: Vec<Piece>,

    // The "New Game" button, which calls `restart`
    new_game_button: NewGameButton,

    // Points scored this game: each merge adds the merged tile's number.
    score: u32,

    // Highest score reached by an earlier game this session. `get_best` folds the current game's
    // score in, so the displayed best is live and never needs touching on a merge.
    best: u32,

    // Draws `score` and `best` into the two boxes at the top of the board.
    score_display: ScoreDisplay,
}

impl App2048 {
    pub(crate) fn new() -> Self {
        let board_geometry = BoardGeometry::new();
        let live_pieces = App2048::start_pieces(&board_geometry);

        App2048 {
            board_geometry,
            board_states: VecDeque::from([BoardState::Idle]),
            live_pieces,
            new_game_button: NewGameButton::default(),
            score: 0,
            best: 0,
            score_display: ScoreDisplay::default(),
        }
    }

    fn start_pieces(bg: &BoardGeometry) -> Vec<Piece> {
        let piece_a = Piece::new_random(bg);
        let mut piece_b = Piece::new_random(bg);

        while piece_a.get_c_slot() == piece_b.get_c_slot() {
            piece_b = Piece::new_random(bg);
        }

        vec![piece_a, piece_b]
    }

    pub(crate) fn restart(&mut self) {
        self.live_pieces = App2048::start_pieces(&self.board_geometry);
        self.board_states = VecDeque::from([BoardState::Idle]);
        self.best = self.get_best();
        self.score = 0;
    }

    pub(crate) fn get_score(&self) -> u32 {
        self.score
    }

    pub(crate) fn get_best(&self) -> u32 {
        self.best.max(self.score)
    }

    pub(crate) fn get_score_display(&self) -> &ScoreDisplay {
        &self.score_display
    }

    pub(crate) fn get_score_display_mut(&mut self) -> &mut ScoreDisplay {
        &mut self.score_display
    }

    pub(crate) fn get_new_game_button(&self) -> &NewGameButton {
        &self.new_game_button
    }

    pub(crate) fn get_new_game_button_mut(&mut self) -> &mut NewGameButton {
        &mut self.new_game_button
    }

    #[cfg(test)]
    pub(crate) fn with_pieces(live_pieces: Vec<Piece>) -> Self {
        let mut app = App2048::new();
        app.live_pieces = live_pieces;
        app
    }

    #[cfg(test)]
    pub(crate) fn take_last_piece(&mut self) -> Option<Piece> {
        self.live_pieces.pop()
    }

    pub(crate) fn is_game_over(&self) -> bool {
        self.board_states.front() == Some(&BoardState::GameOver)
    }

    pub(crate) fn can_move(&self) -> bool {
        let board = self.reconstruct_board();
        let value = |x: usize, y: usize| board[y][x].map(|i| self.live_pieces[i].get_value());

        (0..4).any(|y| {
            (0..4).any(|x| {
                let v = value(x, y);
                v.is_none() || (x < 3 && value(x + 1, y) == v) || (y < 3 && value(x, y + 1) == v)
            })
        })
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

    pub(crate) fn start_move(&mut self, dir: MoveDirection) {
        if !self.board_idle() {
            return;
        }

        self.set_move_targets(&dir);

        if !self.live_pieces.iter().any(Piece::is_moving) {
            return;
        }

        self.push_state(BoardState::Move(dir));
        self.push_state(BoardState::Resolve);
        self.push_state(BoardState::Spawn);
        self.push_state(BoardState::Idle);
        self.pop_state();
    }

    /// Advances the board state machine by one frame.
    pub(crate) fn step(&mut self, dt: f32) {
        match self.board_states.front() {
            None => self.push_state(BoardState::Idle),
            Some(BoardState::Idle) => {}
            Some(BoardState::Move(_)) => {
                // Pop the state once every piece has arrived.
                if self.update_live_pieces_pos(dt) {
                    self.pop_state();
                }
            }
            Some(BoardState::Resolve) => {
                self.resolve_merges();
                self.pop_state();
            }
            Some(BoardState::Spawn) => {
                self.spawn_piece();
                self.pop_state();
                if !self.can_move() {
                    log::info!("2048: game over, no moves left");
                    self.board_states.clear();
                    self.push_state(BoardState::GameOver);
                }
            }
            Some(BoardState::GameOver) => {}
        }
    }

    pub(crate) fn spawn_piece(&mut self) -> Option<Vec2<usize>> {
        let board = self.reconstruct_board();
        let empty: Vec<Vec2<usize>> = (0..4)
            .flat_map(|y| (0..4).map(move |x| Vec2::new(x, y)))
            .filter(|s| board[s[1]][s[0]].is_none())
            .collect();

        let slot = *empty.choose(&mut rand::rng())?;
        self.live_pieces.push(Piece::new(
            &self.board_geometry,
            Piece::random_value(),
            slot,
        ));
        Some(slot)
    }

    pub(crate) fn update_live_pieces_pos(&mut self, dt: f32) -> bool {
        let mut update_done = true;
        for piece in &mut self.live_pieces {
            update_done &= piece.update_pos(dt);
        }
        update_done
    }

    pub(crate) fn resolve_merges(&mut self) {
        self.live_pieces
            .retain(|piece| piece.get_merge() != MergeRole::Consumed);
        for piece in &mut self.live_pieces {
            self.score += piece.resolve_merge();
        }
    }

    pub(crate) fn set_piece_merge(&mut self, idx: usize, merge: MergeRole) {
        self.live_pieces[idx].set_merge(merge);
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
    fn spawning_fills_the_only_empty_slot_and_then_stops() {
        let bg = BoardGeometry::new();
        // Every slot but (1, 2), in a checkerboard of 2s and 4s so nothing could merge.
        let pieces = (0..4)
            .flat_map(|y| (0..4).map(move |x| (x, y)))
            .filter(|&(x, y)| (x, y) != (1, 2))
            .map(|(x, y)| Piece::new(&bg, 1 + (x + y) % 2, Vec2::new(x, y)))
            .collect();
        let mut app = App2048::with_pieces(pieces);

        assert_eq!(app.spawn_piece(), Some(Vec2::new(1, 2)));
        let spawned = app.get_live_pieces().last().unwrap();
        assert_eq!(spawned.get_c_slot(), Vec2::new(1, 2));
        assert!((1..=2).contains(&spawned.get_value()));

        assert_eq!(app.spawn_piece(), None);
        assert_eq!(app.get_live_pieces().len(), 16);
    }

    /// A full board in a checkerboard of 2s and 4s: no slot is empty and nothing can merge.
    fn stuck_pieces(bg: &BoardGeometry) -> Vec<Piece> {
        (0..4)
            .flat_map(|y| (0..4).map(move |x| (x, y)))
            .map(|(x, y)| Piece::new(bg, 1 + (x + y) % 2, Vec2::new(x, y)))
            .collect()
    }

    #[test]
    fn a_full_board_without_equal_neighbours_cannot_move() {
        let bg = BoardGeometry::new();
        assert!(!App2048::with_pieces(stuck_pieces(&bg)).can_move());
    }

    #[test]
    fn an_empty_slot_or_equal_neighbours_still_allow_a_move() {
        let bg = BoardGeometry::new();

        let mut with_gap = stuck_pieces(&bg);
        with_gap.remove(5);
        assert!(App2048::with_pieces(with_gap).can_move());

        // Equal neighbours in a row (bottom-right corner matches its left neighbour) ...
        let mut row_pair = stuck_pieces(&bg);
        row_pair[15] = Piece::new(&bg, row_pair[14].get_value(), Vec2::new(3, 3));
        assert!(App2048::with_pieces(row_pair).can_move());

        // ... and in a column (bottom-right corner matches the piece above it).
        let mut col_pair = stuck_pieces(&bg);
        col_pair[15] = Piece::new(&bg, col_pair[11].get_value(), Vec2::new(3, 3));
        assert!(App2048::with_pieces(col_pair).can_move());
    }

    #[test]
    fn a_spawn_that_leaves_no_move_ends_the_game_and_freezes_input() {
        let bg = BoardGeometry::new();
        // A stuck board minus (0, 0); the spawn must land there. Make both possible spawn values
        // dead ends: (1, 0) and (0, 1) are 8s, which no 2 or 4 can merge with.
        let mut pieces = stuck_pieces(&bg);
        pieces.remove(0);
        for p in pieces.iter_mut() {
            if p.get_c_slot() == Vec2::new(1, 0) || p.get_c_slot() == Vec2::new(0, 1) {
                *p = Piece::new(&bg, 3, p.get_c_slot());
            }
        }
        let mut app = App2048::with_pieces(pieces);
        app.pop_state();
        app.push_state(BoardState::Spawn);
        app.push_state(BoardState::Idle);

        app.step(1.0 / 60.0);
        assert!(app.is_game_over());
        assert!(!app.board_idle());

        let before: Vec<_> = app.get_live_pieces().clone();
        app.start_move(MoveDirection::Left);
        for _ in 0..100 {
            app.step(1.0 / 60.0);
        }
        assert!(app.is_game_over());
        assert_eq!(app.get_live_pieces(), &before);
    }

    #[test]
    fn restart_after_a_game_over_gives_a_fresh_playable_board() {
        let bg = BoardGeometry::new();
        let mut app = App2048::with_pieces(stuck_pieces(&bg));
        app.board_states = VecDeque::from([BoardState::GameOver]);
        app.score = 1234;
        assert!(app.is_game_over());

        app.restart();
        assert!(!app.is_game_over());
        assert_eq!(app.get_score(), 0);
        assert!(app.board_idle());
        assert_eq!(app.get_live_pieces().len(), 2);
        assert_ne!(
            app.get_live_pieces()[0].get_c_slot(),
            app.get_live_pieces()[1].get_c_slot()
        );

        // And it plays: some direction moves the two pieces.
        for dir in [
            MoveDirection::Up,
            MoveDirection::Down,
            MoveDirection::Left,
            MoveDirection::Right,
        ] {
            app.start_move(dir);
            if !app.board_idle() {
                return;
            }
        }
        panic!("no direction moved the fresh board");
    }

    #[test]
    fn merging_scores_the_merged_tile_and_the_best_follows_the_score() {
        let bg = BoardGeometry::new();
        // Two 2s on one row: a right press merges them into a 4, scoring 4.
        let mut app = App2048::with_pieces(vec![
            Piece::new(&bg, 1, Vec2::new(0, 0)),
            Piece::new(&bg, 1, Vec2::new(1, 0)),
        ]);
        assert_eq!(app.get_best(), 0);

        app.start_move(MoveDirection::Right);
        for _ in 0..100 {
            app.step(1.0 / 60.0);
        }

        assert_eq!(app.get_score(), 4);
        assert_eq!(app.get_best(), 4);
    }

    #[test]
    fn the_best_survives_a_restart_and_a_worse_game_never_lowers_it() {
        let mut app = App2048::new();
        app.score = 5000;
        assert_eq!(app.get_best(), 5000);

        app.restart();
        assert_eq!(app.get_score(), 0);
        assert_eq!(app.get_best(), 5000);

        app.score = 8;
        assert_eq!(app.get_score(), 8);
        assert_eq!(app.get_best(), 5000);

        app.restart();
        assert_eq!(app.get_best(), 5000);
    }

    #[test]
    fn restart_mid_move_drops_the_move() {
        let bg = BoardGeometry::new();
        let mut app = App2048::with_pieces(vec![Piece::new(&bg, 1, Vec2::new(0, 0))]);
        app.start_move(MoveDirection::Right);
        app.step(1.0 / 60.0);
        assert!(!app.board_idle());

        app.restart();
        assert!(app.board_idle());
        assert_eq!(app.get_live_pieces().len(), 2);
        for _ in 0..100 {
            app.step(1.0 / 60.0);
        }
        assert_eq!(app.get_live_pieces().len(), 2); // Nothing resolved or spawned.
    }

    #[test]
    fn a_spawn_that_leaves_a_move_does_not_end_the_game() {
        let mut app = App2048::new();
        app.pop_state();
        app.push_state(BoardState::Spawn);
        app.push_state(BoardState::Idle);
        app.step(1.0 / 60.0);
        assert!(!app.is_game_over());
        assert!(app.board_idle());
    }

    #[test]
    fn spawned_values_are_mostly_twos() {
        let twos = (0..1000).filter(|_| Piece::random_value() == 1).count();
        assert!((800..=980).contains(&twos), "{twos} of 1000 were 2s");
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
