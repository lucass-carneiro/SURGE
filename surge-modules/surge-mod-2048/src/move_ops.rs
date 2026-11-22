use super::App2048;
use crate::{app_2048::MoveDirection, piece::MergeRole};
use surge_sdk::nalgebra::Vector2 as Vec2;

/// The slot at position `k` of lane `lane` for a move in `dir`, counted from the edge the pieces
/// move towards (`k = 0`) inwards. A lane is a row for Left/Right and a column for Up/Down.
/// Row 0 is the top of the board (+y points down), so Up moves towards `y = 0`.
fn lane_slot(dir: &MoveDirection, lane: usize, k: usize) -> Vec2<usize> {
    match dir {
        MoveDirection::Right => Vec2::new(3 - k, lane),
        MoveDirection::Left => Vec2::new(k, lane),
        MoveDirection::Down => Vec2::new(lane, 3 - k),
        MoveDirection::Up => Vec2::new(lane, k),
    }
}

impl App2048 {
    /// Sets every piece's target slot (and merge role) for a move in `dir`.
    ///
    /// Each lane (a row for Left/Right, a column for Up/Down) is scanned from the edge the pieces
    /// move towards inwards, packing pieces against that edge. A piece equal in value to the last
    /// one placed slides into it instead: the placed piece `Absorbs` and the incoming one is
    /// `Consumed`. A piece that has already absorbed cannot absorb again in the same move, so
    /// (moving right) `[2, 2, 2, 2]` becomes `[_, _, 4, 4]` and `[4, 2, 2, _]` becomes
    /// `[_, _, 4, 4]`, not `[_, _, _, 8]`. The merges themselves are applied on `Resolve`.
    pub(crate) fn set_move_targets(&mut self, dir: &MoveDirection) {
        let board = self.reconstruct_board();

        for lane in 0..4 {
            // How many slots of this lane are taken, counted from the edge; the last placed piece
            // sits at `placed - 1` and the next free slot is `placed`.
            let mut placed = 0;
            // The last placed piece, while it can still absorb another one.
            let mut absorber: Option<usize> = None;

            for k in 0..4 {
                let slot = lane_slot(dir, lane, k);
                let Some(idx) = board[slot[1]][slot[0]] else {
                    continue;
                };
                let value = self.get_live_pieces()[idx].get_value();

                match absorber {
                    Some(prev) if self.get_live_pieces()[prev].get_value() == value => {
                        // Slide into the absorber's target slot and merge there.
                        self.set_piece_move_target(idx, lane_slot(dir, lane, placed - 1));
                        self.set_piece_merge(prev, MergeRole::Absorbs);
                        self.set_piece_merge(idx, MergeRole::Consumed);
                        absorber = None;
                    }
                    _ => {
                        self.set_piece_move_target(idx, lane_slot(dir, lane, placed));
                        placed += 1;
                        absorber = Some(idx);
                    }
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use crate::{
        app_2048::{App2048, MoveDirection},
        board_geometry::BoardGeometry,
        piece::Piece,
    };
    use surge_sdk::nalgebra::Vector2 as Vec2;

    /// Builds an app with one piece per `(x, y, value)`, in order. `value` is a texture index
    /// (`1` is the 2 tile, `2` the 4 tile, ...).
    fn app_with(pieces: &[(usize, usize, usize)]) -> App2048 {
        let bg = BoardGeometry::new();
        App2048::with_pieces(
            pieces
                .iter()
                .map(|&(x, y, v)| Piece::new(&bg, v, Vec2::new(x, y)))
                .collect(),
        )
    }

    /// Runs the move to completion, resolves its merges and returns each surviving piece's
    /// final `(x, y, value)`.
    fn settle(app: &mut App2048) -> Vec<(usize, usize, usize)> {
        for _ in 0..100 {
            if app.update_live_pieces_pos(1.0) {
                app.resolve_merges();
                return app
                    .get_live_pieces()
                    .iter()
                    .map(|p| (p.get_c_slot()[0], p.get_c_slot()[1], p.get_value()))
                    .collect();
            }
        }
        panic!("pieces never settled");
    }

    fn move_in(dir: MoveDirection, pieces: &[(usize, usize, usize)]) -> Vec<(usize, usize, usize)> {
        let mut app = app_with(pieces);
        app.set_move_targets(&dir);
        settle(&mut app)
    }

    fn move_right(pieces: &[(usize, usize, usize)]) -> Vec<(usize, usize, usize)> {
        move_in(MoveDirection::Right, pieces)
    }

    fn move_left(pieces: &[(usize, usize, usize)]) -> Vec<(usize, usize, usize)> {
        move_in(MoveDirection::Left, pieces)
    }

    fn move_up(pieces: &[(usize, usize, usize)]) -> Vec<(usize, usize, usize)> {
        move_in(MoveDirection::Up, pieces)
    }

    fn move_down(pieces: &[(usize, usize, usize)]) -> Vec<(usize, usize, usize)> {
        move_in(MoveDirection::Down, pieces)
    }

    #[test]
    fn single_piece_slides_to_the_right_edge() {
        assert_eq!(move_right(&[(0, 2, 1)]), vec![(3, 2, 1)]);
    }

    #[test]
    fn two_unequal_pieces_pack_against_the_right_edge_in_order() {
        assert_eq!(
            move_right(&[(0, 1, 1), (2, 1, 2)]),
            vec![(2, 1, 1), (3, 1, 2)]
        );
    }

    #[test]
    fn three_unequal_pieces_pack_against_the_right_edge_in_order() {
        // Listed out of x order to prove the order comes from the board, not the piece list.
        assert_eq!(
            move_right(&[(3, 0, 1), (0, 0, 3), (1, 0, 2)]),
            vec![(3, 0, 1), (1, 0, 3), (2, 0, 2)]
        );
    }

    #[test]
    fn rows_are_independent_and_full_unmergeable_rows_do_not_move() {
        assert_eq!(
            move_right(&[(0, 0, 1), (0, 1, 1), (1, 1, 2), (2, 1, 1), (3, 1, 2)]),
            vec![(3, 0, 1), (0, 1, 1), (1, 1, 2), (2, 1, 1), (3, 1, 2)]
        );
    }

    #[test]
    fn already_packed_pieces_stay_put() {
        assert_eq!(
            move_right(&[(3, 3, 1), (2, 3, 2)]),
            vec![(3, 3, 1), (2, 3, 2)]
        );
    }

    #[test]
    fn two_equal_pieces_merge_at_the_right_edge() {
        assert_eq!(move_right(&[(0, 0, 1), (1, 0, 1)]), vec![(3, 0, 2)]);
    }

    #[test]
    fn equal_pieces_merge_across_a_gap() {
        assert_eq!(move_right(&[(0, 2, 3), (3, 2, 3)]), vec![(3, 2, 4)]);
    }

    #[test]
    fn a_full_row_of_equal_pieces_merges_in_pairs() {
        assert_eq!(
            move_right(&[(0, 0, 1), (1, 0, 1), (2, 0, 1), (3, 0, 1)]),
            vec![(2, 0, 2), (3, 0, 2)]
        );
    }

    #[test]
    fn of_three_equal_pieces_the_rightmost_pair_merges() {
        assert_eq!(
            move_right(&[(0, 1, 1), (1, 1, 1), (2, 1, 1)]),
            vec![(2, 1, 1), (3, 1, 2)]
        );
    }

    #[test]
    fn a_merged_piece_does_not_merge_again_in_the_same_move() {
        // [4, 2, 2, _] -> [_, _, 4, 4], not [_, _, _, 8].
        assert_eq!(
            move_right(&[(0, 0, 2), (1, 0, 1), (2, 0, 1)]),
            vec![(2, 0, 2), (3, 0, 2)]
        );
    }

    #[test]
    fn a_blocked_piece_does_not_merge_past_a_different_one() {
        // [2, 4, 2, _] -> [_, 2, 4, 2]: nothing merges.
        assert_eq!(
            move_right(&[(0, 3, 1), (1, 3, 2), (2, 3, 1)]),
            vec![(1, 3, 1), (2, 3, 2), (3, 3, 1)]
        );
    }

    /// The surviving pieces as `(x, y, value)`, sorted so the order does not matter.
    fn board_of(app: &App2048) -> Vec<(usize, usize, usize)> {
        let mut pieces: Vec<_> = app
            .get_live_pieces()
            .iter()
            .map(|p| (p.get_c_slot()[0], p.get_c_slot()[1], p.get_value()))
            .collect();
        pieces.sort();
        pieces
    }

    // Left, Up and Down: the same packing and merging rules, towards the other edges.

    #[test]
    fn single_piece_slides_to_the_left_edge() {
        assert_eq!(move_left(&[(3, 2, 1)]), vec![(0, 2, 1)]);
    }

    #[test]
    fn single_piece_slides_to_the_top_edge() {
        // Row 0 is the top of the board.
        assert_eq!(move_up(&[(1, 3, 1)]), vec![(1, 0, 1)]);
    }

    #[test]
    fn single_piece_slides_to_the_bottom_edge() {
        assert_eq!(move_down(&[(1, 0, 1)]), vec![(1, 3, 1)]);
    }

    #[test]
    fn unequal_pieces_pack_against_the_left_edge_in_order() {
        // [_, 2, _, 4] -> [2, 4, _, _]
        assert_eq!(
            move_left(&[(1, 1, 1), (3, 1, 2)]),
            vec![(0, 1, 1), (1, 1, 2)]
        );
    }

    #[test]
    fn unequal_pieces_pack_against_the_top_and_bottom_edges_in_order() {
        // Column 2, top to bottom: [2, _, 4, _]
        assert_eq!(move_up(&[(2, 0, 1), (2, 2, 2)]), vec![(2, 0, 1), (2, 1, 2)]);
        assert_eq!(
            move_down(&[(2, 0, 1), (2, 2, 2)]),
            vec![(2, 2, 1), (2, 3, 2)]
        );
    }

    #[test]
    fn full_lanes_of_equal_pieces_merge_in_pairs_towards_the_edge() {
        // [2, 2, 2, 2] in row 0 (left) and in column 0 (up / down).
        let row = [(0, 0, 1), (1, 0, 1), (2, 0, 1), (3, 0, 1)];
        let col = [(0, 0, 1), (0, 1, 1), (0, 2, 1), (0, 3, 1)];
        assert_eq!(move_left(&row), vec![(0, 0, 2), (1, 0, 2)]);
        assert_eq!(move_up(&col), vec![(0, 0, 2), (0, 1, 2)]);
        assert_eq!(move_down(&col), vec![(0, 2, 2), (0, 3, 2)]);
    }

    #[test]
    fn of_three_equal_pieces_the_pair_nearest_the_edge_merges() {
        // [2, 2, 2, _]: left -> [4, 2, _, _]; right -> [_, _, 2, 4] (covered above).
        assert_eq!(
            move_left(&[(0, 1, 1), (1, 1, 1), (2, 1, 1)]),
            vec![(0, 1, 2), (1, 1, 1)]
        );
        // Column 3, top to bottom [_, 2, 2, 2]: up -> [4, 2, _, _]; down -> [_, _, 2, 4].
        assert_eq!(
            move_up(&[(3, 1, 1), (3, 2, 1), (3, 3, 1)]),
            vec![(3, 0, 2), (3, 1, 1)]
        );
        assert_eq!(
            move_down(&[(3, 1, 1), (3, 2, 1), (3, 3, 1)]),
            vec![(3, 2, 1), (3, 3, 2)]
        );
    }

    #[test]
    fn a_merged_piece_does_not_merge_again_moving_left_up_or_down() {
        // [_, 2, 2, 4] -> [4, 4, _, _], not [8, _, _, _].
        assert_eq!(
            move_left(&[(1, 0, 1), (2, 0, 1), (3, 0, 2)]),
            vec![(0, 0, 2), (1, 0, 2)]
        );
        // Column 1, top to bottom [_, 2, 2, 4] -> up [4, 4, _, _].
        assert_eq!(
            move_up(&[(1, 1, 1), (1, 2, 1), (1, 3, 2)]),
            vec![(1, 0, 2), (1, 1, 2)]
        );
        // Column 1, top to bottom [4, 2, 2, _] -> down [_, _, 4, 4].
        assert_eq!(
            move_down(&[(1, 0, 2), (1, 1, 1), (1, 2, 1)]),
            vec![(1, 2, 2), (1, 3, 2)]
        );
    }

    #[test]
    fn columns_are_independent_when_moving_vertically() {
        // Column 0: [2, 2, _, _]; column 1: [4, _, _, _]; column 3: [2, 4, 2, 4] (full, stuck).
        let pieces = [
            (0, 0, 1),
            (0, 1, 1),
            (1, 0, 2),
            (3, 0, 1),
            (3, 1, 2),
            (3, 2, 1),
            (3, 3, 2),
        ];
        assert_eq!(
            move_down(&pieces),
            vec![
                (0, 3, 2),
                (1, 3, 2),
                (3, 0, 1),
                (3, 1, 2),
                (3, 2, 1),
                (3, 3, 2)
            ]
        );
        assert_eq!(
            move_up(&pieces),
            vec![
                (0, 0, 2),
                (1, 0, 2),
                (3, 0, 1),
                (3, 1, 2),
                (3, 2, 1),
                (3, 3, 2)
            ]
        );
    }

    /// Mirrors a board left-right (`x -> 3 - x`).
    fn mirror_x(pieces: &[(usize, usize, usize)]) -> Vec<(usize, usize, usize)> {
        pieces.iter().map(|&(x, y, v)| (3 - x, y, v)).collect()
    }

    /// Transposes a board (`(x, y) -> (y, x)`), turning rows into columns.
    fn transpose(pieces: &[(usize, usize, usize)]) -> Vec<(usize, usize, usize)> {
        pieces.iter().map(|&(x, y, v)| (y, x, v)).collect()
    }

    fn sorted(mut pieces: Vec<(usize, usize, usize)>) -> Vec<(usize, usize, usize)> {
        pieces.sort();
        pieces
    }

    #[test]
    fn every_direction_is_a_reflection_of_right() {
        // Left is Right on the mirrored board; Down is Right on the transposed board; Up is
        // Right on the transposed, then mirrored board.
        let boards: [&[(usize, usize, usize)]; 4] = [
            &[
                (0, 0, 1),
                (2, 0, 1),
                (3, 0, 2),
                (0, 1, 3),
                (1, 1, 3),
                (2, 1, 3),
            ],
            &[
                (0, 3, 1),
                (1, 3, 2),
                (2, 3, 3),
                (3, 3, 4),
                (1, 2, 1),
                (3, 2, 1),
            ],
            &[
                (0, 0, 2),
                (1, 0, 1),
                (2, 0, 1),
                (0, 2, 5),
                (3, 2, 5),
                (2, 1, 1),
            ],
            &[
                (0, 0, 1),
                (1, 0, 1),
                (2, 0, 1),
                (3, 0, 1),
                (0, 1, 1),
                (0, 2, 1),
                (0, 3, 1),
            ],
        ];

        for board in boards {
            assert_eq!(
                sorted(move_left(board)),
                sorted(mirror_x(&move_right(&mirror_x(board))))
            );
            assert_eq!(
                sorted(move_down(board)),
                sorted(transpose(&move_right(&transpose(board))))
            );
            assert_eq!(
                sorted(move_up(board)),
                sorted(transpose(&mirror_x(&move_right(&mirror_x(&transpose(
                    board
                ))))))
            );
        }
    }

    /// Steps the state machine (at 60 fps) until the board is idle again (or the game is over).
    fn run_until_idle(app: &mut App2048) {
        for _ in 0..1000 {
            app.step(1.0 / 60.0);
            if app.board_idle() || app.is_game_over() {
                return;
            }
        }
        panic!("the move never finished");
    }

    /// Checks that the board is `expected` plus a spawned piece if `spawn` is true (none
    /// otherwise), and removes that piece so later presses stay deterministic. The spawn must
    /// sit on a slot `expected` leaves empty and be a 2 or a 4.
    fn take_spawn(app: &mut App2048, expected: &[(usize, usize, usize)], spawn: bool) {
        let mut expected = expected.to_vec();
        expected.sort();
        if spawn {
            let spawned = app.take_last_piece().expect("no piece at all");
            let (x, y) = (spawned.get_c_slot()[0], spawned.get_c_slot()[1]);
            let v = spawned.get_value();
            assert!((1..=2).contains(&v), "spawned value {v}");
            assert!(
                !expected.iter().any(|&(ex, ey, _)| (ex, ey) == (x, y)),
                "spawned on the occupied slot ({x}, {y})"
            );
        }
        assert_eq!(board_of(app), expected);
    }

    /// Presses `dir`, runs the move to completion and checks the result is `expected` plus one
    /// spawned piece (or no spawn if the board did not change), removing the spawned piece.
    fn press(app: &mut App2048, dir: MoveDirection, expected: &[(usize, usize, usize)]) {
        let before = board_of(app);
        app.start_move(dir);
        run_until_idle(app);
        let mut sorted_expected = expected.to_vec();
        sorted_expected.sort();
        take_spawn(app, expected, sorted_expected != before);
    }

    #[test]
    fn presses_in_every_direction_run_through_the_state_machine() {
        // [2, _, _, 2] in row 0 plus a 4 in the bottom-right corner.
        let mut app = app_with(&[(0, 0, 1), (3, 0, 1), (3, 3, 2)]);
        // Left: row 0 merges into a 4 at the left edge, the corner 4 slides left.
        press(&mut app, MoveDirection::Left, &[(0, 0, 2), (0, 3, 2)]);
        // Down: the two 4s in column 0 merge into an 8 at the bottom.
        press(&mut app, MoveDirection::Down, &[(0, 3, 3)]);
        // Up, then Right: the 8 travels to the top, then to the top-right corner.
        press(&mut app, MoveDirection::Up, &[(0, 0, 3)]);
        press(&mut app, MoveDirection::Right, &[(3, 0, 3)]);
    }

    #[test]
    fn a_right_press_runs_move_resolve_and_spawn_through_the_state_machine() {
        let mut app = app_with(&[(0, 0, 1), (1, 0, 1), (2, 0, 1), (3, 0, 1)]);
        press(&mut app, MoveDirection::Right, &[(2, 0, 2), (3, 0, 2)]);
    }

    #[test]
    fn consecutive_right_presses_keep_merging() {
        // [2, 2, 2, 2] -> [_, _, 4, 4] -> [_, _, _, 8] -> unchanged (and nothing spawns).
        let mut app = app_with(&[(0, 0, 1), (1, 0, 1), (2, 0, 1), (3, 0, 1)]);
        assert_eq!(app.get_score(), 0);
        press(&mut app, MoveDirection::Right, &[(2, 0, 2), (3, 0, 2)]);
        assert_eq!(app.get_score(), 8); // Two 4s.
        press(&mut app, MoveDirection::Right, &[(3, 0, 3)]);
        assert_eq!(app.get_score(), 16); // Plus an 8.
        press(&mut app, MoveDirection::Right, &[(3, 0, 3)]);
        assert_eq!(app.get_score(), 16); // No merge, no points.
    }

    #[test]
    fn a_move_without_merges_scores_nothing() {
        let mut app = app_with(&[(0, 0, 1), (0, 1, 2)]);
        press(&mut app, MoveDirection::Right, &[(3, 0, 1), (3, 1, 2)]);
        assert_eq!(app.get_score(), 0);
    }

    #[test]
    fn a_press_that_moves_nothing_spawns_nothing_and_stays_idle() {
        let mut app = app_with(&[(3, 0, 1), (3, 1, 2), (2, 1, 1)]);
        app.start_move(MoveDirection::Right);
        assert!(app.board_idle());
        run_until_idle(&mut app);
        assert_eq!(board_of(&app), vec![(2, 1, 1), (3, 0, 1), (3, 1, 2)]);
    }

    #[test]
    fn every_move_that_changes_the_board_spawns_exactly_one_piece() {
        // Random games: every press either changes nothing at all, or leaves no two pieces on
        // one slot and adds exactly one 2 or 4 to the board's total (merges keep the total).
        fn dir(i: usize) -> MoveDirection {
            match i % 4 {
                0 => MoveDirection::Up,
                1 => MoveDirection::Right,
                2 => MoveDirection::Down,
                _ => MoveDirection::Left,
            }
        }
        // Total tile value, counted in 2s (a value-`v` piece is worth 2^(v-1) of them).
        fn total(board: &[(usize, usize, usize)]) -> usize {
            board.iter().map(|&(_, _, v)| 1usize << (v - 1)).sum()
        }

        let mut games_over = 0;
        for game in 0..20 {
            let mut app = App2048::new();
            for i in 0..300 {
                let before = board_of(&app);
                app.start_move(dir(i * (game + 1) + i / 3));
                let moved = !app.board_idle();
                run_until_idle(&mut app);
                let after = board_of(&app);

                let mut slots: Vec<_> = after.iter().map(|&(x, y, _)| (x, y)).collect();
                slots.dedup();
                assert_eq!(slots.len(), after.len(), "overlapping pieces: {after:?}");

                if moved {
                    let gained = total(&after) - total(&before);
                    assert!(gained == 1 || gained == 2, "{before:?} -> {after:?}");
                } else {
                    assert_eq!(after, before);
                }

                // The game ends exactly when no move is left, and then refuses every press.
                assert_eq!(app.is_game_over(), !app.can_move());
                if app.is_game_over() {
                    for d in 0..4 {
                        app.start_move(dir(d));
                        run_until_idle(&mut app);
                        assert_eq!(board_of(&app), after);
                    }
                    games_over += 1;
                    break;
                }
            }
        }
        // These blind strategies lose quickly, so the game-over checks above really run.
        assert!(games_over >= 10, "only {games_over} of 20 games ended");
    }

    #[test]
    fn a_press_during_a_move_is_ignored() {
        let mut app = app_with(&[(0, 0, 1), (1, 0, 1), (2, 0, 1), (3, 0, 1)]);
        app.start_move(MoveDirection::Right);
        app.step(1.0 / 60.0);
        // Would merge the two 4s into an 8 if it were accepted.
        app.start_move(MoveDirection::Right);
        run_until_idle(&mut app);
        take_spawn(&mut app, &[(2, 0, 2), (3, 0, 2)], true);
    }

    #[test]
    fn a_right_press_on_a_mixed_board_moves_every_row() {
        // Row 0: [2, _, 2, 4] -> [_, _, 4, 4]
        // Row 1: [8, 8, 8, _] -> [_, _, 8, 16]
        // Row 2: [_, _, _, _] -> empty
        // Row 3: [2, 4, 8, 16] -> unchanged
        let mut app = app_with(&[
            (0, 0, 1),
            (2, 0, 1),
            (3, 0, 2),
            (0, 1, 3),
            (1, 1, 3),
            (2, 1, 3),
            (0, 3, 1),
            (1, 3, 2),
            (2, 3, 3),
            (3, 3, 4),
        ]);
        press(
            &mut app,
            MoveDirection::Right,
            &[
                (0, 3, 1),
                (1, 3, 2),
                (2, 0, 2),
                (2, 1, 3),
                (2, 3, 3),
                (3, 0, 2),
                (3, 1, 4),
                (3, 3, 4),
            ],
        );
        assert_eq!(app.get_score(), 4 + 16);
    }

    #[test]
    fn two_max_value_pieces_still_merge_but_stay_capped() {
        use crate::piece::MAX_VALUE;
        assert_eq!(
            move_right(&[(0, 0, MAX_VALUE), (1, 0, MAX_VALUE)]),
            vec![(3, 0, MAX_VALUE)]
        );
    }

    #[test]
    fn merging_two_max_value_pieces_scores_the_real_doubled_tile() {
        use crate::piece::MAX_VALUE;
        let mut app = app_with(&[(0, 0, MAX_VALUE), (1, 0, MAX_VALUE)]);
        app.set_move_targets(&MoveDirection::Right);
        settle(&mut app);
        assert_eq!(app.get_score(), 4096);
    }
}
