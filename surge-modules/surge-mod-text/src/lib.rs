//! A minimal module whose only job is to render text, exercising the full shaping → wire →
//! rasterization → atlas → draw path end to end. It draws nothing but text: a few fixed sample
//! lines at different sizes, anchors, and colors, plus a per-frame counter whose changing digits
//! push new glyphs through the atlas cache at runtime.

use surge_sdk::{
    Font, SpriteCanvas, SurgeApp, TextAnchor, TextCanvasExt,
    nalgebra::{Vector2, Vector4},
};

const MODULE_NAME: &str = "surge-mod-text";
const FONT_ASSET: &str = "fonts/NotoSans-Regular.ttf";

pub struct AppText {
    /// Re-registered in `on_load` and `on_swapchain_recreate`, because the player restarts font
    /// ids at 0 whenever it rebuilds its font database. `None` only before the first load.
    font: Option<Font>,
    /// Frames drawn so far, shown as a live counter.
    frame_counter: u64,
}

#[unsafe(no_mangle)]
pub fn surge_register_app() -> Box<dyn SurgeApp> {
    Box::new(AppText {
        font: None,
        frame_counter: 0,
    })
}

fn load_font(canvas: &mut dyn SpriteCanvas) -> Option<Font> {
    let path = surge_sdk::resolve_asset_path(MODULE_NAME, FONT_ASSET);
    match Font::load(canvas, path.to_str().unwrap()) {
        Ok(font) => Some(font),
        Err(e) => {
            log::error!("failed to load font `{}`: {e}", path.display());
            None
        }
    }
}

impl SurgeApp for AppText {
    fn on_load(&mut self, canvas: &mut dyn SpriteCanvas) {
        log::info!("Text app startup");
        self.font = load_font(canvas);
    }

    fn on_swapchain_recreate(&mut self, canvas: &mut dyn SpriteCanvas) {
        self.font = load_font(canvas);
    }

    fn update(&mut self, _dt: f32, canvas: &mut dyn SpriteCanvas) {
        self.frame_counter += 1;

        let Some(font) = self.font.as_ref() else {
            return;
        };

        const WHITE: Vector4<f32> = Vector4::new(0.92, 0.92, 0.95, 1.0);
        const TEAL: Vector4<f32> = Vector4::new(0.30, 0.80, 0.75, 1.0);
        const AMBER: Vector4<f32> = Vector4::new(0.98, 0.75, 0.25, 1.0);

        // Title, horizontally centered near the top.
        let title = font.shape("SURGE text", 64.0);
        canvas.draw_text(
            &title,
            Vector2::new(400.0, 70.0),
            TextAnchor::TopCenter,
            WHITE,
            0.5,
        );

        // Left- and right-anchored lines against the window edges, to show the anchor math.
        let left = font.shape("anchored left", 28.0);
        canvas.draw_text(
            &left,
            Vector2::new(20.0, 200.0),
            TextAnchor::TopLeft,
            WHITE,
            0.5,
        );

        let right = font.shape("anchored right", 28.0);
        canvas.draw_text(
            &right,
            Vector2::new(780.0, 200.0),
            TextAnchor::TopRight,
            WHITE,
            0.5,
        );

        // A colored line in a different size.
        let colored = font.shape("colored glyphs", 40.0);
        canvas.draw_text(
            &colored,
            Vector2::new(400.0, 300.0),
            TextAnchor::TopCenter,
            TEAL,
            0.5,
        );

        // A live counter: its changing digits keep producing cache hits (and a few early misses)
        // in the glyph atlas while the demo runs.
        let counter = font.shape(&format!("frame: {}", self.frame_counter), 32.0);
        canvas.draw_text(
            &counter,
            Vector2::new(400.0, 420.0),
            TextAnchor::TopCenter,
            AMBER,
            0.5,
        );
    }
}
