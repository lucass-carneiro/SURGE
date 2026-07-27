# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository status

**`README.md` is stale.** It documents a previous C++/CMake/vcpkg incarnation of SURGE (OpenGL, `SURGE_*` CMake flags, submodules). None of that applies: this is now a Rust workspace using Vulkan via `ash`. Only the "Philosophy" section (player / core / modules split) still describes the design. Do not follow the README's build instructions.

## Build & run

Requires `glslang` on `PATH` (from the Vulkan SDK) — `surge-core/build.rs` shells out to it and hard-fails the build if it is missing or the GLSL does not compile.

```bash
cargo check --workspace          # fastest correctness loop
cargo build                      # debug: target/debug/surge-player + libsurge_mod_*.so
cargo build --release            # required before staging
cargo run -p surge-player        # MUST be run from the repo root (see CWD contract)
```

There are currently no tests anywhere in the workspace. If you add some, the usual `cargo test -p <crate> <filter>` / `-- --exact <test::path>` applies.

### The CWD contract

The player resolves nearly everything relative to the process working directory, so **always run from the repo root** during development:

- `config.toml` is read from CWD. It is **gitignored** — the checked-in copies live at `surge-modules/<module>/config.toml`, and the root one is a per-developer working copy (typically a module's config with `app_folder` repointed at `target/debug`).
- SPIR-V is loaded from `shaders/sprite_vert.spv` and `shaders/sprite_frag.spv`. The repo-root `shaders/` directory holds **symlinks** into `surge-core/src/vulkan/sprite_database/shaders/`, where `build.rs` writes the compiled `.spv` (themselves gitignored). On Windows use `extra/ln.ps1 <src> <dst>` to recreate these links.
- Module asset paths are whatever the module hardcodes. `surge-mod-2048` uses repo-root-relative paths (`surge-modules/surge-mod-2048/assets/...`), so it only runs correctly from the root in dev; staging flattens this to `assets/`.

`stager/stager.py` produces a self-contained, CWD-correct release layout:

```bash
python3 stager/stager.py list
python3 stager/stager.py stage surge-mod-2048 [-o]   # -> staging-surge-mod-2048/
```

It requires `cargo build --release` to have run and must itself be invoked from the repo root. It copies the player binary, the module dylib, the module's own `config.toml`, `shaders/`, and `assets/`.

### Feature flags

- `surge-core/validation_layers` — **on by default**. Enables Vulkan validation layers + debug messenger, and gates several `VulkanError` variants behind `cfg`.
- `surge-player/hot_reloading` — declared but **not implemented**. The game loop and the shutdown path have `// Handle hot reloading` / `// Refresh HR key state` placeholders. Hot reloading is the engine's stated purpose but does not exist yet in the Rust port.

PNG and SVG assets are tracked with **git-lfs** (see `.gitattributes`).

## Architecture

Three-layer split, mirroring the README's philosophy:

- **`surge-core`** — the engine library: window/Vulkan setup, config parsing, sprite renderer, dynamic app loading. Everything else depends on it.
- **`surge-player`** — the executable. Owns the `winit` event loop and drives the frame; contains no game logic.
- **`surge-modules/*`** — games. Each is a `crate-type = ["rlib", "dylib"]` crate loaded at runtime by the player. Glob-included as workspace members, so a new directory under `surge-modules/` is picked up automatically.

### Module ABI

A module is loaded by `surge_core::app::load_from_dylib(folder, name)`, which `dlopen`s `lib<name>.so` (Linux) / `<name>.dll` (Windows) and resolves a single symbol:

```rust
#[unsafe(no_mangle)]
pub fn surge_register_app() -> Box<dyn SurgeApp> { ... }
```

Note the config's `app_name` uses the **library** name with underscores (`surge_mod_2048`), not the Cargo package name.

`LoadedApp` holds `app: Box<dyn SurgeApp>` **before** `_library: Library` — field order is load-bearing, since the trait object's vtable lives in the library and must be dropped first. Preserve it.

Modules call `surge_core::cli::init_env_logger()` themselves in `on_load`; each dylib has its own `log` crate state.

`surge-player` also has a direct `path` dependency on `surge-mod-default`, which is vestigial — module loading is entirely dynamic.

### Frame lifecycle (`surge-player/src/main.rs`)

`ApplicationHandler` maps onto the engine loop:

- `resumed` → create window, `VulkanContext::new`, `SpriteDatabase::new`, then `app.on_load(spd)`.
- `about_to_wait` → the whole frame: optional swapchain recreation, `app.update(dt, spd)`, acquire image, `cmd_begin` → `cmd_render_begin` → `spd.draw()` → `cmd_render_end` → `cmd_end` → `cmd_submit` → `present_swpc`, then a manual FPS-cap sleep (only when `vsync = false` and `cap_fps = true`).
- `exiting` → `app.on_unload()`, then **`destroy_swapchain()` explicitly**, because winit drops the surface before the `VulkanContext` would otherwise be dropped.

Rendering pauses entirely on minimize *and on focus loss*. On resize (or a suboptimal swapchain), the player recreates the swapchain **and rebuilds the `SpriteDatabase` from scratch**, which is why `on_swapchain_recreate` exists: its job is to re-upload every texture, since the old GPU textures died with the old database.

### Vulkan layer (`surge-core/src/vulkan/`)

`VulkanContext` is one large struct in `mod.rs`; its `impl` blocks are split across `ctx_*.rs` by concern (`ctx_new_drop`, `ctx_swpc`, `ctx_command`, `ctx_graphics_pipeline`, `ctx_shader`). Free-function helpers used during init live in `instance.rs`, `device.rs`, `image.rs`, `command.rs`. When adding context methods, put them in the matching `ctx_*.rs` rather than growing `mod.rs`.

The context is shared as `Arc<RefCell<VulkanContext>>`. GPU resource wrappers (`buffer::Buffer`, `texture::Texture`) each clone that `Arc` and free themselves in `Drop` — RAII, no manual teardown. Beware: methods borrow the `RefCell` per call, so chained `context.borrow()` inside a `borrow_mut()` scope will panic at runtime.

`FRAMES_IN_FLIGHT = 2` drives the per-frame duplication of semaphores, fences, command buffers, and the instance SSBOs.

### Sprite database — the only draw path

`SpriteDatabase` (`vulkan/sprite_database/mod.rs`) is an immediate-mode 2D batcher. Per frame a module calls `add_instance(&InstanceInfo)` any number of times; `draw()` issues **one instanced draw of 6 vertices** and resets occupancy to zero. Nothing persists between frames — a sprite not re-added disappears.

- Vertex positions and UVs are baked into `sprite.vert` (no vertex buffers). Per-instance data (`model`, `color`, `subtexture_data`, `material`) comes from an SSBO addressed via a **buffer device address passed in a push constant** (`GL_EXT_buffer_reference`).
- `ci.max_sprites` (hardcoded to **32** in the player) caps *both* live instances per frame *and* total uploaded textures — they share the descriptor count. Exceeding either logs a warning and silently drops the request.
- `texture_id` in `InstanceInfo` is the **index in upload order**; `upload_texture` accepts PNG only (8- or 16-bit, expanded to RGBA).
- `subtexture_info` selects a sub-rect for sprite-sheet animation, converted to a UV transform (derivation in `extra/subimage.wls`). An out-of-bounds sub-rect falls back to the full texture with a warning.
- Coordinates: orthographic, origin at the **top-left**, +x right, **+y down**, z in `[0, 1]`. The depth test is `GREATER_OR_EQUAL`, so **larger z draws behind** — backgrounds get the higher z (2048 uses board `0.5`, pieces `0.0`).

Shaders are GLSL 460 in `sprite_database/shaders/`; edit the `.vert`/`.frag` and rebuild — `build.rs` reruns `glslang`. If you change the SSBO/UBO/push-constant layout, update both the shader struct and the matching `#[repr]` Rust struct in `sprite_database/mod.rs`.

### Config (`surge-core/src/config/`)

`config.toml` deserializes into `EngineConfig` via serde. Unknown keys are silently ignored (e.g. `[window] allow_resizes` appears in the shipped configs but has no field — the window is hardcoded `.with_resizable(false)`). `windowed = false` means borderless fullscreen on the primary monitor.

### `surge-mod-2048` (work in progress)

The active development target. `App2048` (`app_2048.rs`) drives a `VecDeque<BoardState>` queue of `Idle` / `Move(dir)` / `Resolve`; input is only accepted while `board_idle()`. `Piece` stores both a logical slot (`c_slot`/`t_slot`) and a pixel position (`c_pos`/`t_pos`), lerping toward the target each frame in `update_pos`, which returns `true` when settled — the `Move` state pops once every piece has arrived. `BoardGeometry` maps 4×4 slots to pixels for the fixed 500×800 window.

Per-direction target selection lives in its own file: only `right_ops.rs` (`set_move_target_right`) exists so far. `Up`/`Down`/`Left` push states but set no targets, and merging is not implemented. Note the state push order differs between the arrow-key arms in `lib.rs` — `Up` pushes `Resolve` before `Move`; the others push `Move` first. Follow the `Right` arm as the reference when adding the remaining directions.
