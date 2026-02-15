use crate::{InstanceInfo, board_geometry::BoardGeometry};
use nalgebra::{Vector2 as Vec2, Vector4 as Vec4};
use rand::Rng;

#[derive(Debug, PartialEq, Clone, Copy)]
pub(crate) struct Piece {
    value: usize,        // Piece value
    c_slot: Vec2<usize>, // Current slot
    t_slot: Vec2<usize>, // Target slot
    c_pos: Vec2<f32>,    // Current pixel pos.
    t_pos: Vec2<f32>,    // Target pixel pos.
}

impl Piece {
    pub(crate) fn new_random(bg: &BoardGeometry) -> Self {
        let mut rng = rand::rng();
        let value = rng.random_range(1..=2);
        let x = rng.random_range(0..4);
        let y = rng.random_range(0..4);

        let slot = Vec2::new(x, y);
        let pos = bg.slot_to_pos(slot);

        Piece {
            value,
            c_slot: slot,
            t_slot: slot,
            c_pos: pos,
            t_pos: pos,
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

    pub(crate) fn update_pos(&mut self) -> bool {
        // TODO: Better position update ?
        let r = self.t_pos - self.c_pos;
        if r.norm() > 1.0e-3 {
            self.c_pos += 0.1 * r;
            false
        } else {
            self.c_slot = self.t_slot;
            self.c_pos = self.t_pos;
            true
        }
    }

    pub(crate) fn get_c_slot(&self) -> Vec2<usize> {
        self.c_slot
    }
}
