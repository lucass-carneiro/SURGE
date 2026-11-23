use surge_core::vulkan::{font_database::FontDatabase, sprite_database::SpriteDatabase};
use surge_sdk::{CanvasError, CreateInfo, InstanceInfo, SpriteCanvas, TextRun};

/// The single `&mut dyn SpriteCanvas` the player hands to every module callback, routing sprite
/// calls to the sprite database and text calls to the font database (§3.3 of the text-rendering
/// design). It borrows both databases for the duration of one callback; the player keeps them as
/// owned fields and draws each in turn.
pub struct Canvas<'a> {
    pub sprites: &'a mut SpriteDatabase,
    pub fonts: &'a mut FontDatabase,
}

impl SpriteCanvas for Canvas<'_> {
    fn add_instance(&mut self, aii: &InstanceInfo) {
        self.sprites.add_instance(aii);
    }

    fn upload_texture(&mut self, texture_file: &str) -> Result<usize, CanvasError> {
        Ok(self.sprites.upload_texture(texture_file)?)
    }

    fn load_font(&mut self, path: &str) -> Result<usize, CanvasError> {
        Ok(self.fonts.load_font(path)?)
    }

    fn add_text_run(&mut self, run: &TextRun) {
        self.fonts.add_text_run(run);
    }

    // Both databases are built from the same `CreateInfo`, so either answers.
    fn get_create_info(&self) -> CreateInfo {
        self.sprites.get_create_info()
    }
}
