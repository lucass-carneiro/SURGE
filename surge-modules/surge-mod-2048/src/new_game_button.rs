use crate::InstanceInfo;
use surge_sdk::{
    SubTextureInfo,
    nalgebra::{Vector2 as Vec2, Vector4 as Vec4},
    winit::event::{ElementState, MouseButton},
};

/// Texture index of `button.png`
pub(crate) const TEXTURE_ID: usize = 12;

/// Top-left corner on the 500x800 board: right-aligned under the SCORE/BEST boxes, whose pair
/// spans exactly the button's width (x = 358..496).
const POSITION: Vec2<f32> = Vec2::new(358.0, 68.0);

/// One half of the sheet, in pixels; drawn at 1:1.
const SIZE: Vec2<u32> = Vec2::new(138, 40);

/// In front of the board (larger z draws behind).
const Z: f32 = 0.0;

#[derive(Debug, Default)]
pub(crate) struct NewGameButton {
    /// The cursor is over the button.
    hovered: bool,

    /// The left button went down over the button and has not been released yet.
    held: bool,
}

impl NewGameButton {
    fn contains(point: Vec2<f32>) -> bool {
        let end = POSITION + SIZE.cast::<f32>();
        (POSITION.x..end.x).contains(&point.x) && (POSITION.y..end.y).contains(&point.y)
    }

    pub(crate) fn cursor_moved(&mut self, position: Vec2<f32>) {
        self.hovered = NewGameButton::contains(position);
    }

    pub(crate) fn mouse_button(&mut self, state: ElementState, button: MouseButton) -> bool {
        if button != MouseButton::Left {
            return false;
        }

        match state {
            ElementState::Pressed => {
                self.held = self.hovered;
                false
            }
            ElementState::Released => {
                let clicked = self.held && self.hovered;
                self.held = false;
                clicked
            }
        }
    }

    fn looks_pressed(&self) -> bool {
        self.held && self.hovered
    }

    pub(crate) fn get_sprite_instance_info(&self) -> InstanceInfo {
        let half = if self.looks_pressed() { 1 } else { 0 };

        InstanceInfo {
            position: POSITION,
            scale: SIZE.cast::<f32>(),
            z: Z,
            texture_id: TEXTURE_ID,
            color_multiplier: Vec4::from_element(1.0),
            subtexture_info: Some(SubTextureInfo {
                origin: Vec2::new(half * SIZE.x, 0),
                extent: SIZE,
            }),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const INSIDE: Vec2<f32> = Vec2::new(400.0, 90.0);
    const OUTSIDE: Vec2<f32> = Vec2::new(100.0, 90.0);

    fn click(button: &mut NewGameButton) -> bool {
        let pressed = button.mouse_button(ElementState::Pressed, MouseButton::Left);
        let released = button.mouse_button(ElementState::Released, MouseButton::Left);
        pressed || released
    }

    #[test]
    fn the_hit_box_is_the_drawn_rectangle() {
        assert!(NewGameButton::contains(Vec2::new(358.0, 68.0)));
        assert!(NewGameButton::contains(Vec2::new(495.9, 107.9)));
        assert!(!NewGameButton::contains(Vec2::new(357.9, 80.0)));
        assert!(!NewGameButton::contains(Vec2::new(496.0, 80.0)));
        assert!(!NewGameButton::contains(Vec2::new(400.0, 108.0)));
        assert!(!NewGameButton::contains(Vec2::new(400.0, 67.9)));
    }

    #[test]
    fn a_left_click_on_the_button_fires_on_release() {
        let mut button = NewGameButton::default();
        button.cursor_moved(INSIDE);
        assert!(!button.mouse_button(ElementState::Pressed, MouseButton::Left));
        assert!(button.mouse_button(ElementState::Released, MouseButton::Left));
    }

    #[test]
    fn clicks_elsewhere_or_with_other_buttons_do_nothing() {
        let mut button = NewGameButton::default();
        assert!(!click(&mut button)); // No cursor position seen yet.
        button.cursor_moved(OUTSIDE);
        assert!(!click(&mut button));

        button.cursor_moved(INSIDE);
        assert!(!button.mouse_button(ElementState::Pressed, MouseButton::Right));
        assert!(!button.mouse_button(ElementState::Released, MouseButton::Right));
    }

    #[test]
    fn dragging_off_before_release_cancels_and_dragging_on_does_not_click() {
        let mut button = NewGameButton::default();
        button.cursor_moved(INSIDE);
        button.mouse_button(ElementState::Pressed, MouseButton::Left);
        button.cursor_moved(OUTSIDE);
        assert!(!button.mouse_button(ElementState::Released, MouseButton::Left));

        button.mouse_button(ElementState::Pressed, MouseButton::Left);
        button.cursor_moved(INSIDE);
        assert!(!button.mouse_button(ElementState::Released, MouseButton::Left));
    }

    #[test]
    fn the_sprite_shows_the_pressed_half_only_while_held_over_the_button() {
        let half = |b: &NewGameButton| b.get_sprite_instance_info().subtexture_info.unwrap();
        let mut button = NewGameButton::default();

        button.cursor_moved(INSIDE);
        assert_eq!(half(&button).origin, Vec2::new(0, 0));

        button.mouse_button(ElementState::Pressed, MouseButton::Left);
        assert_eq!(half(&button).origin, Vec2::new(138, 0));
        assert_eq!(half(&button).extent, Vec2::new(138, 40));

        button.cursor_moved(OUTSIDE);
        assert_eq!(half(&button).origin, Vec2::new(0, 0));
        button.cursor_moved(INSIDE);
        assert_eq!(half(&button).origin, Vec2::new(138, 0));

        button.mouse_button(ElementState::Released, MouseButton::Left);
        assert_eq!(half(&button).origin, Vec2::new(0, 0));
    }

    #[test]
    fn the_button_fits_its_texture_and_the_window() {
        let info = NewGameButton::default().get_sprite_instance_info();
        let sub = info.subtexture_info.unwrap();
        assert!(sub.origin.x + sub.extent.x <= 276 && sub.extent.y <= 40);
        assert!(info.position.x + info.scale.x <= 500.0);
        assert!(info.position.y + info.scale.y <= 800.0);
    }
}
