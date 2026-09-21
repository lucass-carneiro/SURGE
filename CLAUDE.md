# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Workflow rules

These apply to every change, not just plan steps:

- **Suggest a commit message; never commit.** After finishing a change, give a suggested commit message of exactly two short sentences: the first summarizes *what changed*, the second *why*. Do not run `git commit` — the user commits.
- **Mark finished plan steps as done.** When a step of the implementation plan in `docs/design/module-abi-and-linking.md` is completed, mark it `**[DONE]**` right after its number in that doc, in the same change as the step's code. An unmarked step is still open.

## Build & run

Requires `glslang` on `PATH` (from the Vulkan SDK) — `surge-core/build.rs` shells out to it and hard-fails the build if it is missing or the GLSL does not compile.

```bash
cargo check --workspace          # fastest correctness loop
cargo build                      # debug: target/debug/surge-player + one dev binary per module (surge_mod_*)
cargo build --release --features release-bin   # required before staging: <module>_release binaries
cargo run -p surge-player        # MUST be run from the repo root (see CWD contract)
cargo test --workspace           # tests live in surge-core and surge-sdk; `cargo test -p <crate> <filter>` narrows
```

### The CWD contract

The player resolves nearly everything relative to the process working directory, so **always run from the repo root** during development:

- `config.toml` is read from CWD. It is **gitignored** — the checked-in copies live at `surge-modules/<module>/config.toml`, and the root one is a per-developer working copy (typically a module's config with `app_folder` repointed at `target/debug`).
- SPIR-V is **not** read from disk at runtime. `surge-core/build.rs` compiles `sprite_database/shaders/sprite.{vert,frag}` with `glslang` into its own Cargo `OUT_DIR`, and `sprite_database/mod.rs` pulls the bytes in at compile time with `include_bytes!(concat!(env!("OUT_DIR"), "/sprite_vert.spv"))`, so the SPIR-V ships inside the player binary itself. There is no `shaders/` directory, no symlinks, and nothing for the stager to copy.
- Module asset paths go through `surge_sdk::resolve_asset_path(module_name, relative)`, which tries `<exe_dir>/assets/` first (staged layout) and falls back to the repo-root-relative `surge-modules/<module_name>/assets/` (dev), so a module runs correctly from the repo root in dev and from a staged folder.

`stager/stager.py` produces a self-contained, CWD-correct release layout:

```bash
python3 stager/stager.py list
python3 stager/stager.py stage surge-mod-2048 [-o]   # -> staging-surge-mod-2048/
```

It requires `cargo build --release --features release-bin` to have run and must itself be invoked from the repo root. It copies the module's single `<module>_release` binary, the module's own `config.toml`, and `assets/` (minus the `assets/svg/` source artwork) — nothing else (shaders are embedded in the binary). The staged binary still reads `config.toml` from CWD, so run it with the staging folder as the working directory.

### Feature flags

- `surge-core/validation_layers` — **on by default**. Enables Vulkan validation layers + debug messenger, and gates several `VulkanError` variants behind `cfg`.
- `<module>/release-bin` — pulls in the optional `surge-player` dependency and enables the module's `<module>_release` `[[bin]]` (`required-features`). Without it only the dev binary builds.

PNG and SVG assets are tracked with **git-lfs** (see `.gitattributes`).

## Architecture

Four crates, mirroring the README's philosophy (player / core / modules) plus the shared SDK:

- **`surge-sdk`** — the module-facing API and shared plain data: the `SurgeApp` trait, the `SpriteCanvas` trait and sprite types (`InstanceInfo`, `CreateInfo`, …), `KeyEvent`, config types/`parse_config`, `resolve_asset_path`, `init_env_logger`, and the dev-mode IPC (wire types, `CanvasProxy`, `run_dev_module`). Modules depend on this and nothing else from the engine.
- **`surge-core`** — the engine library: window/Vulkan setup, sprite renderer. Implements `SpriteCanvas` for `SpriteDatabase`. Depends on `surge-sdk`; modules never link it.
- **`surge-player`** — a `[lib]` exposing `run(app, config)` (the `winit` event loop and frame; no game logic) plus the dev-mode `[[bin]]` (`main.rs`, `remote_app.rs`, `proc.rs`).
- **`surge-modules/*`** — games. Each is an `rlib` with two binaries (below). Glob-included as workspace members, so a new directory under `surge-modules/` is picked up automatically.

### Module ABI

There is no dynamic loading and no shared Rust ABI: a module is an ordinary crate exposing `pub fn surge_register_app() -> Box<dyn SurgeApp>` from its `lib.rs`, and each module has two entry points that call it:

- **Dev (`src/bin/dev.rs`, always built)** — `surge-player` reads `config.toml`, and `RemoteApp` spawns `<startup_app.app_folder>/<startup_app.app_name>` (e.g. `target/debug/surge_mod_2048`; the *binary* name with underscores, not the Cargo package name) as a **child process** and drives it over a TCP loopback socket (`127.0.0.1`, OS-assigned port passed as `argv[1]`, `TCP_NODELAY` set on both ends, `postcard`-encoded `Request`/`Response`, handshake checked against `WIRE_VERSION`). The module's `main()` is `init_env_logger()` + `surge_sdk::run_dev_module(...)`. Each `update` is a blocking round trip; the module's `SpriteCanvas` is a `CanvasProxy` that buffers `add_instance`/`upload_texture` calls and ships them back as `CanvasOps`. `proc::DeathGuard` ties the child's life to the player's so it dies with a crashed player too — `PR_SET_PDEATHSIG` on Linux, a Job Object (`JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`) on Windows. **Linux and Windows only** (`proc.rs`'s child-lifecycle mechanism has no macOS/BSD branch, so dev mode doesn't build there).
- **Release (`src/bin/release.rs`, behind `release-bin`)** — one process: `main()` parses `config.toml` and calls `surge_player::run(surge_register_app(), config)`. No IPC, no child.

In dev mode the player handles **F5** itself (never forwarded to the module): `RemoteApp::reload` stops the child, respawns the binary from the same path (so a rebuilt binary is picked up), and re-runs `on_load`. If the module dies unexpectedly, `is_faulted()` becomes true, the player freezes on the last presented frame and appends `— MODULE CRASHED` to the window title until a successful F5. That is the whole "hot reloading" story; there is no in-process reload.

Because dev-mode modules are separate processes, each has its own `log` state — the dev binary calls `surge_sdk::init_env_logger()` itself, and `surge_player::run` does it for release binaries.

### Frame lifecycle (`surge-player/src/lib.rs`)

`ApplicationHandler` maps onto the engine loop:

- `resumed` → create window, `VulkanContext::new`, `SpriteDatabase::new`, then `app.on_load(spd)`.
- `about_to_wait` → the whole frame: if the module `is_faulted()`, return immediately (window frozen); otherwise optional swapchain recreation, acquire image (this must precede `update`, which writes that frame's instance SSBO), `app.update(dt, spd)` — if that call leaves the module faulted the frame is dropped there, unpresented, so the window keeps the last good image — then `cmd_begin` → `cmd_render_begin` → `spd.draw()` → `cmd_render_end` → `cmd_end` → `cmd_submit` → `present_swpc`, then a manual FPS-cap sleep (only when `vsync = false` and `cap_fps = true`).
- F5 (in `window_event`) → `app.reload()`; a `true` return schedules a swapchain recreate, whose `on_swapchain_recreate` re-uploads textures into the fresh database.
- `exiting` → `app.on_unload()`, then **`destroy_swapchain()` explicitly**, because winit drops the surface before the `VulkanContext` would otherwise be dropped.

Rendering pauses entirely while the window has zero size (minimized); focus changes do not affect rendering, and any non-zero `Resized` resumes. On resize (or a suboptimal swapchain), the player recreates the swapchain **and rebuilds the `SpriteDatabase` from scratch**, which is why `on_swapchain_recreate` exists: its job is to re-upload every texture, since the old GPU textures died with the old database.

### Vulkan layer (`surge-core/src/vulkan/`)

`VulkanContext` is one large struct in `mod.rs`; its `impl` blocks are split across `ctx_*.rs` by concern (`ctx_new_drop`, `ctx_swpc`, `ctx_command`, `ctx_graphics_pipeline`, `ctx_shader`). Free-function helpers used during init live in `instance.rs`, `device.rs`, `image.rs`, `command.rs`. When adding context methods, put them in the matching `ctx_*.rs` rather than growing `mod.rs`.

The context is shared as `Arc<RefCell<VulkanContext>>`. GPU resource wrappers (`buffer::Buffer`, `texture::Texture`) each clone that `Arc` and free themselves in `Drop` — RAII, no manual teardown. Beware: methods borrow the `RefCell` per call, so chained `context.borrow()` inside a `borrow_mut()` scope will panic at runtime.

`FRAMES_IN_FLIGHT = 2` drives the per-frame duplication of semaphores, fences, command buffers, and the instance SSBOs.

### Sprite database — the only draw path

`SpriteDatabase` (`vulkan/sprite_database/mod.rs`) is an immediate-mode 2D batcher. Per frame a module calls `add_instance(&InstanceInfo)` any number of times; `draw()` issues **one instanced draw of 6 vertices** and resets occupancy to zero. Nothing persists between frames — a sprite not re-added disappears.

- Vertex positions and UVs are baked into `sprite.vert` (no vertex buffers). Per-instance data (`model`, `color`, `subtexture_data`, `material`) comes from an SSBO addressed via a **buffer device address passed in a push constant** (`GL_EXT_buffer_reference`).
- `ci.max_sprites` (hardcoded to **32** in the player) caps *both* live instances per frame *and* total uploaded textures — they share the descriptor count. Exceeding the instance cap logs a warning and silently drops the request; exceeding the texture cap makes `upload_texture`/`upload_decoded_texture` return `VulkanError::TextureDatabaseFull` (modules see it as a `CanvasError` through `SpriteCanvas::upload_texture`, and both modules `.unwrap()` it, so this panics).
- `texture_id` in `InstanceInfo` is the **index in upload order**; `upload_texture` accepts PNG only (8- or 16-bit, expanded to RGBA).
- `subtexture_info` selects a sub-rect for sprite-sheet animation, converted to a UV transform (derivation in `extra/subimage.wls`). An out-of-bounds sub-rect falls back to the full texture with a warning.
- Coordinates: orthographic, origin at the **top-left**, +x right, **+y down**, z in `[0, 1]`. The depth test is `GREATER_OR_EQUAL`, so **larger z draws behind** — backgrounds get the higher z (2048 uses board `0.5`, pieces `0.0`).

Shaders are GLSL 460 in `sprite_database/shaders/`; edit the `.vert`/`.frag` and rebuild — `build.rs` reruns `glslang`. If you change the SSBO/UBO/push-constant layout, update both the shader struct and the matching `#[repr]` Rust struct in `sprite_database/mod.rs`.

### Config (`surge-sdk/src/config.rs`)

`config.toml` deserializes into `EngineConfig` via serde. Unknown keys are silently ignored (there is no `deny_unknown_fields`), so a stale or misspelled key does nothing; the window is hardcoded `.with_resizable(false)` and there is no MSAA setting. `windowed = false` means borderless fullscreen on the primary monitor.

### `surge-mod-2048` (work in progress)

The active development target. `App2048` (`app_2048.rs`) drives a `VecDeque<BoardState>` queue of `Idle` / `Move(dir)` / `Resolve`; input is only accepted while `board_idle()`. `Piece` stores both a logical slot (`c_slot`/`t_slot`) and a pixel position (`c_pos`/`t_pos`), lerping toward the target each frame in `update_pos`, which returns `true` when settled — the `Move` state pops once every piece has arrived. `BoardGeometry` maps 4×4 slots to pixels for the fixed 500×800 window.

Per-direction target selection lives in its own file: only `right_ops.rs` (`set_move_target_right`) exists so far. `Up`/`Down`/`Left` push states but set no targets, and merging is not implemented. Note the state push order differs between the arrow-key arms in `lib.rs` — `Up` pushes `Resolve` before `Move`; the others push `Move` first. Follow the `Right` arm as the reference when adding the remaining directions.
