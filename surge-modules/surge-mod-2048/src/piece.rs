use crate::{InstanceInfo, board_geometry::BoardGeometry};
use rand::Rng;
use surge_sdk::nalgebra::{Vector2 as Vec2, Vector4 as Vec4};

/// The highest piece value: a texture index, `11` being the 2048 tile (the last uploaded texture).
pub(crate) const MAX_VALUE: usize = 11;

/// What happens to a piece when the current move is resolved.
#[derive(Debug, PartialEq, Clone, Copy)]
pub(crate) enum MergeRole {
    /// Nothing: the piece just moves (or stays).
    None,

    /// Another piece slides into this one, which then doubles (its value goes up by one).
    Absorbs,

    /// This piece slides into another one and then disappears.
    Consumed,
}

#[derive(Debug, PartialEq, Clone, Copy)]
pub(crate) struct Piece {
    value: usize,        // Piece value
    c_slot: Vec2<usize>, // Current slot
    t_slot: Vec2<usize>, // Target slot
    c_pos: Vec2<f32>,    // Current pixel pos.
    t_pos: Vec2<f32>,    // Target pixel pos.
    merge: MergeRole,    // Role in the pending merge, applied on Resolve
}

// Exponential approach rate (1/s).
const MOVE_RATE: f32 = 6.3212;

// A moving piece snaps onto its target once it is this close to the target (in pixels).
const SNAP_DISTANCE: f32 = 0.5;

impl Piece {
    pub(crate) fn new_random(bg: &BoardGeometry) -> Self {
        let mut rng = rand::rng();
        let x = rng.random_range(0..4);
        let y = rng.random_range(0..4);

        Piece::new(bg, Piece::random_value(), Vec2::new(x, y))
    }

    pub(crate) fn random_value() -> usize {
        if rand::rng().random_bool(0.9) { 1 } else { 2 }
    }

    /// A settled piece of the given value (a texture index) sitting in `slot`.
    /// `slot` is given as `(col, row)`, counted from 0
    pub(crate) fn new(bg: &BoardGeometry, value: usize, slot: Vec2<usize>) -> Self {
        let pos = bg.slot_to_pos(slot);

        Piece {
            value,
            c_slot: slot,
            t_slot: slot,
            c_pos: pos,
            t_pos: pos,
            merge: MergeRole::None,
        }
    }

    pub(crate) fn get_sprite_instance_info(&self, bg: &BoardGeometry) -> InstanceInfo {
        InstanceInfo {
            position: self.c_pos,
            scale: Vec2::from_element(bg.get_piece_dims()),
            z: bg.get_piece_z(),
            texture_id: self.value,
            color_multiplier: Vec4::from_element(1.0),
            subtexture_info: None,
        }
    }

    pub(crate) fn set_move_target(&mut self, bg: &BoardGeometry, slot: Vec2<usize>) {
        if self.c_slot != slot {
            self.t_slot = slot;
            self.t_pos = bg.slot_to_pos(slot);
        }
    }

    pub(crate) fn update_pos(&mut self, dt: f32) -> bool {
        let r = self.t_pos - self.c_pos;
        if r.norm() > SNAP_DISTANCE {
            // Frame-rate independent: the same fraction of the gap closes per second at any fps.
            self.c_pos += (1.0 - (-MOVE_RATE * dt).exp()) * r;
            false
        } else {
            self.c_slot = self.t_slot;
            self.c_pos = self.t_pos;
            true
        }
    }

    pub(crate) fn is_moving(&self) -> bool {
        self.t_slot != self.c_slot
    }

    pub(crate) fn get_c_slot(&self) -> Vec2<usize> {
        self.c_slot
    }

    pub(crate) fn get_value(&self) -> usize {
        self.value
    }

    pub(crate) fn get_merge(&self) -> MergeRole {
        self.merge
    }

    pub(crate) fn set_merge(&mut self, merge: MergeRole) {
        self.merge = merge;
    }

    /// Applies an `Absorbs` merge (the value doubles) and clears the role. Returns the points the
    /// merge scores: the merged tile's number (a 4 for two 2s), or 0 if this piece did not absorb.
    /// The points follow the real doubling, so two 2048s still score 4096 though the piece stays
    /// capped at `MAX_VALUE`. `Consumed` pieces are removed by the caller, not here.
    pub(crate) fn resolve_merge(&mut self) -> u32 {
        let points = if self.merge == MergeRole::Absorbs {
            let points = 1 << (self.value + 1);
            self.value = (self.value + 1).min(MAX_VALUE);
            points
        } else {
            0
        };
        self.merge = MergeRole::None;
        points
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_full_width_slide_settles_in_about_a_second() {
        let bg = BoardGeometry::new();
        let mut piece = Piece::new(&bg, 1, Vec2::new(0, 0));
        piece.set_move_target(&bg, Vec2::new(3, 0));

        let dt = 1.0 / 60.0;
        let frames = (0..600).position(|_| piece.update_pos(dt)).unwrap();
        assert!(frames <= 70, "took {frames} frames");
        assert_eq!(piece.get_c_slot(), Vec2::new(3, 0));
        assert_eq!(piece.c_pos, bg.slot_to_pos(Vec2::new(3, 0)));
    }
}
