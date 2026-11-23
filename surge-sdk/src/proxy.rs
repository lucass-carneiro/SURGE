use crate::{
    canvas::{CanvasError, SpriteCanvas},
    sprite::{CreateInfo, InstanceInfo},
    text::TextRun,
    wire::CanvasOps,
};

/// The dev-mode [`SpriteCanvas`]: records what a module does during one turn so it can be shipped
/// to the player as a single [`CanvasOps`], and answers `get_create_info` from a cached copy.
///
/// Pure data structure — no socket I/O — so the dispatch loop owns all the transport concerns.
#[derive(Debug)]
pub struct CanvasProxy {
    create_info: CreateInfo,
    /// Cumulative across turns and reset only by `cache_create_info`, mirroring the real backend:
    /// the player rebuilds its `SpriteDatabase` exactly when a new `CreateInfo` is pushed, and the
    /// database's upload index restarts from zero there.
    next_texture_id: usize,
    /// Cumulative across turns and reset only by `cache_create_info`, exactly like
    /// `next_texture_id`: the player rebuilds its `FontDatabase` on a new `CreateInfo` and restarts
    /// its font index from zero there.
    next_font_id: usize,
    ops: CanvasOps,
}

impl CanvasProxy {
    pub fn new(create_info: CreateInfo) -> Self {
        Self {
            create_info,
            next_texture_id: 0,
            next_font_id: 0,
            ops: CanvasOps::default(),
        }
    }

    /// Cache the `CreateInfo` pushed with `OnLoad`/`OnSwapchainRecreate`. The player is about to
    /// hand the module a freshly built database, so the upload counter restarts too.
    pub fn cache_create_info(&mut self, create_info: CreateInfo) {
        self.create_info = create_info;
        self.next_texture_id = 0;
        self.next_font_id = 0;
    }

    /// Drain everything recorded since the last drain. The upload counter is untouched.
    pub fn take_ops(&mut self) -> CanvasOps {
        std::mem::take(&mut self.ops)
    }
}

impl SpriteCanvas for CanvasProxy {
    fn add_instance(&mut self, aii: &InstanceInfo) {
        self.ops.instances.push(*aii);
    }

    /// Fire-and-forget: always `Ok`. A real failure is discovered player-side when the
    /// `CanvasOps` is applied, and goes through the crash path rather than back through here.
    fn upload_texture(&mut self, texture_file: &str) -> Result<usize, CanvasError> {
        self.ops.texture_uploads.push(texture_file.to_owned());
        let id = self.next_texture_id;
        self.next_texture_id += 1;
        Ok(id)
    }

    /// Fire-and-forget, like `upload_texture`: always `Ok`, with a real failure surfacing
    /// player-side through the crash path.
    fn load_font(&mut self, path: &str) -> Result<usize, CanvasError> {
        self.ops.font_loads.push(path.to_owned());
        let id = self.next_font_id;
        self.next_font_id += 1;
        Ok(id)
    }

    fn add_text_run(&mut self, run: &TextRun) {
        self.ops.text_runs.push(run.clone());
    }

    fn get_create_info(&self) -> CreateInfo {
        self.create_info
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::sprite::{BlendingMode, TextureFilteringLevel, TextureFilteringMode};
    use crate::text::PositionedGlyph;

    fn create_info(max_sprites: u32) -> CreateInfo {
        CreateInfo {
            blending_mode: BlendingMode::Alpha,
            texture_filtering_mode: TextureFilteringMode::Nearest,
            texture_filtering_level: TextureFilteringLevel::X16,
            max_sprites,
            max_textures: max_sprites,
            window_width: 500.0,
            window_height: 800.0,
        }
    }

    fn instance(texture_id: usize) -> InstanceInfo {
        InstanceInfo {
            position: nalgebra::Vector2::new(1.0, 2.0),
            scale: nalgebra::Vector2::new(3.0, 4.0),
            z: 0.5,
            texture_id,
            color_multiplier: nalgebra::Vector4::new(1.0, 1.0, 1.0, 1.0),
            subtexture_info: None,
        }
    }

    fn text_run(font_id: usize) -> TextRun {
        TextRun {
            font_id,
            px_size: 32.0,
            color: nalgebra::Vector4::new(1.0, 1.0, 1.0, 1.0),
            z: 0.5,
            glyphs: vec![PositionedGlyph {
                glyph_id: 7,
                x: 0.0,
                y: 0.0,
            }],
        }
    }

    #[test]
    fn drain_returns_buffered_calls_in_order() {
        let mut proxy = CanvasProxy::new(create_info(32));

        assert_eq!(proxy.upload_texture("a.png").unwrap(), 0);
        assert_eq!(proxy.upload_texture("b.png").unwrap(), 1);
        proxy.add_instance(&instance(0));
        proxy.add_instance(&instance(1));
        proxy.add_instance(&instance(0));

        let ops = proxy.take_ops();
        assert_eq!(ops.texture_uploads, ["a.png", "b.png"]);
        let ids: Vec<_> = ops.instances.iter().map(|i| i.texture_id).collect();
        assert_eq!(ids, [0, 1, 0]);

        // Draining empties the buffers.
        let ops = proxy.take_ops();
        assert!(ops.texture_uploads.is_empty() && ops.instances.is_empty());
    }

    #[test]
    fn upload_index_is_cumulative_across_drains() {
        let mut proxy = CanvasProxy::new(create_info(32));

        assert_eq!(proxy.upload_texture("a.png").unwrap(), 0);
        assert_eq!(proxy.take_ops().texture_uploads, ["a.png"]);

        // No new `CreateInfo` in between: the player's database is the same one, so the next index
        // continues from the first rather than restarting at this turn's buffer position.
        assert_eq!(proxy.upload_texture("b.png").unwrap(), 1);
        assert_eq!(proxy.take_ops().texture_uploads, ["b.png"]);
    }

    #[test]
    fn upload_index_resets_when_a_new_create_info_is_cached() {
        let mut proxy = CanvasProxy::new(create_info(32));
        proxy.upload_texture("a.png").unwrap();
        proxy.upload_texture("b.png").unwrap();
        proxy.take_ops();

        // The player rebuilt its database; the real backend's first upload is index 0 again.
        proxy.cache_create_info(create_info(64));
        assert_eq!(proxy.upload_texture("a.png").unwrap(), 0);
        assert_eq!(proxy.upload_texture("b.png").unwrap(), 1);
    }

    #[test]
    fn font_index_is_cumulative_across_drains() {
        let mut proxy = CanvasProxy::new(create_info(32));

        assert_eq!(proxy.load_font("a.ttf").unwrap(), 0);
        assert_eq!(proxy.take_ops().font_loads, ["a.ttf"]);

        // No new `CreateInfo` in between: the player's font database is the same one, so the next
        // index continues rather than restarting at this turn's buffer position.
        assert_eq!(proxy.load_font("b.ttf").unwrap(), 1);
        assert_eq!(proxy.take_ops().font_loads, ["b.ttf"]);
    }

    #[test]
    fn font_index_resets_when_a_new_create_info_is_cached() {
        let mut proxy = CanvasProxy::new(create_info(32));
        proxy.load_font("a.ttf").unwrap();
        proxy.load_font("b.ttf").unwrap();
        proxy.take_ops();

        // The player rebuilt its font database; the first font load is index 0 again.
        proxy.cache_create_info(create_info(64));
        assert_eq!(proxy.load_font("a.ttf").unwrap(), 0);
        assert_eq!(proxy.load_font("b.ttf").unwrap(), 1);
    }

    #[test]
    fn text_runs_drain_in_order() {
        let mut proxy = CanvasProxy::new(create_info(32));

        proxy.add_text_run(&text_run(0));
        proxy.add_text_run(&text_run(1));

        let ops = proxy.take_ops();
        let ids: Vec<_> = ops.text_runs.iter().map(|r| r.font_id).collect();
        assert_eq!(ids, [0, 1]);

        // Draining empties the buffer.
        assert!(proxy.take_ops().text_runs.is_empty());
    }

    #[test]
    fn get_create_info_answers_from_the_cache() {
        let mut proxy = CanvasProxy::new(create_info(32));
        assert_eq!(proxy.get_create_info().max_sprites, 32);

        // Nothing but the cache is consulted: the proxy holds no stream, so there is no I/O to
        // perform. Recaching is the only way the answer changes.
        proxy.cache_create_info(create_info(64));
        assert_eq!(proxy.get_create_info().max_sprites, 64);
        assert_eq!(proxy.get_create_info().max_sprites, 64);
    }
}
