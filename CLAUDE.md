# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Workflow rules

These apply to every change, not just plan steps:

- **Suggest a commit message; never commit.** After finishing a change, give a suggested commit message of exactly two short sentences: the first summarizes *what changed*, the second *why*. Do not run `git commit` — the user commits.
- **Code reviews live in `docs/code_reviews/`**, numbered `review_<n>.md` (the first is `review_0.md`). A review is a point-in-time record: mark findings fixed in place (struck-through heading plus a `**Fixed.**` note) rather than deleting them.
- **Mark finished plan steps as done.** When a step of the implementation plan in `docs/design/module-abi-and-linking.md` is completed, mark it `**[DONE]**` right after its number in that doc, in the same change as the step's code. An unmarked step is still open.

## Build & run

Requires `glslang` on `PATH` (from the Vulkan SDK) — `surge-core/build.rs` shells out to it and hard-fails the build if it is missing or the GLSL does not compile.

```bash
cargo check --workspace          # fastest correctness loop
cargo build                      # debug: target/debug/surge-player + one dev binary per module (surge_mod_*)
cargo build --release --features release-bin   # required before staging: <module>_release binaries
cargo run -p surge-player        # MUST be run from the repo root (see CWD contract)
cargo test --workspace           # tests live in surge-core, surge-sdk, surge-player and surge-mod-2048; `cargo test -p <crate> <filter>` narrows.
                                 # surge-player's reload test builds surge-mod-default itself (nested `cargo build`)
                                 # No test needs a GPU or a window: Vulkan-facing logic is kept in pure helpers so it can be tested.
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

Four crates, mirroring the README's philosophy (player / core / modules) plus the shared SDK. Design history for the module ABI lives in `docs/design/module-abi-and-linking.md`.

- **`surge-sdk`** — the module-facing API and shared plain data: the `SurgeApp` trait (`on_load`, `on_swapchain_recreate`, `update`, the input callbacks, `on_unload`, plus the player-facing `reload`/`is_faulted`, which default to "not reloadable"/"never faults"), the `SpriteCanvas` trait and sprite types (`InstanceInfo`, `CreateInfo`, …), `KeyEvent`, config types/`parse_config`, `resolve_asset_path`, `init_env_logger`, and the dev-mode IPC (wire types, `CanvasProxy`, `run_dev_module`). It re-exports `nalgebra` and `winit`, so modules use those through it. Modules depend on this and nothing else from the engine.
- **`surge-core`** — the engine library: Vulkan setup, sprite renderer, error types (`errors::VulkanError`). Implements `SpriteCanvas` for `SpriteDatabase`. Depends on `surge-sdk`; modules never link it.
- **`surge-player`** — a `[lib]` exposing `run(app, config) -> ExitCode` (the `winit` event loop and frame; no game logic; it prints the logo and initializes the logger) and `fail(err)` (reports a pre-`run` error and returns the failure exit code), plus the dev-mode `[[bin]]` (`main.rs`, `remote_app.rs`, `proc.rs`).
- **`surge-modules/*`** — games. Each is an `rlib` with two binaries (below). Glob-included as workspace members, so a new directory under `surge-modules/` is picked up automatically. Two exist: `surge-mod-2048` (the game in progress) and `surge-mod-default` (a renderer demo: depth ordering, alpha blending, motion and a sprite-sheet animation, with its state in `AppDefault`).

### Module ABI

There is no dynamic loading and no shared Rust ABI: a module is an ordinary crate exposing `pub fn surge_register_app() -> Box<dyn SurgeApp>` from its `lib.rs`, and each module has two entry points that call it:

- **Dev (`src/bin/dev.rs`, always built)** — `surge-player` reads `config.toml`, and `RemoteApp` spawns `<startup_app.app_folder>/<startup_app.app_name>` (e.g. `target/debug/surge_mod_2048`; the *binary* name with underscores, not the Cargo package name) as a **child process** and drives it over a TCP loopback socket (`127.0.0.1`, OS-assigned port passed as `argv[1]`, `TCP_NODELAY` set on both ends, `postcard`-encoded `Request`/`Response`, handshake checked against `WIRE_VERSION`). The module's `main()` is `init_env_logger()` + `surge_sdk::run_dev_module(...)`. Each `update` is a blocking round trip; the module's `SpriteCanvas` is a `CanvasProxy` that buffers `add_instance`/`upload_texture` calls and ships them back as `CanvasOps`. `proc::DeathGuard` ties the child's life to the player's so it dies with a crashed player too — `PR_SET_PDEATHSIG` on Linux, a Job Object (`JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`) on Windows. **Linux and Windows only**: `proc.rs` and `remote_app.rs` are `cfg`-gated to those targets (no macOS/BSD child-lifecycle mechanism), so elsewhere the dev player is written to compile but to exit at start-up with an "only supported on Linux and Windows" error (not verified on those platforms). Release binaries have no such limit.
- **Release (`src/bin/release.rs`, behind `release-bin`)** — one process: `main()` parses `config.toml` (reporting failure through `surge_player::fail`) and calls `surge_player::run(surge_register_app(), config)`. No IPC, no child.

In dev mode the player handles **F5** itself (never forwarded to the module; only a fresh press counts, not a repeat): `RemoteApp::reload` stops the child (`SIGTERM`, then `SIGKILL` after a grace period), respawns the binary from the same path (so a rebuilt binary is picked up), re-handshakes, and the player then rebuilds its canvas and re-runs `on_load`. On a clean exit `RemoteApp::on_unload` sends `Shutdown` and reaps the child the same way. A test in `remote_app.rs` covers the crash-then-reload path headlessly. If the module dies unexpectedly, `is_faulted()` becomes true, the player freezes on the last presented frame and appends `— MODULE CRASHED` to the window title until a successful F5. That is the whole "hot reloading" story; there is no in-process reload.

Because dev-mode modules are separate processes, each has its own `log` state — the dev binary calls `surge_sdk::init_env_logger()` itself, and `surge_player::run` does it for the player process (so release binaries get it there). `init_env_logger` defaults to `debug` but honours `RUST_LOG`.

### Frame lifecycle (`surge-player/src/lib.rs`)

`ApplicationHandler` maps onto the engine loop. Callbacks cannot return errors, so a fatal one is recorded (`SurgeContext::fail`), the loop is asked to exit, and `run` returns it as a failure exit code.

- `resumed` → create the window, then `init_graphics` (`VulkanContext::new`, `SpriteDatabase::new`), then `app.on_load(spd)`.
- `about_to_wait` → `frame()`, the whole frame:
  1. If the module `is_faulted()` (the title gains `— MODULE CRASHED`) or rendering is paused, switch to `ControlFlow::Wait` and return; the time spent asleep is discarded rather than handed to `update` as `dt`.
  2. If a swapchain recreate is pending: `recreate_swapchain`, build a **new** `SpriteDatabase`, then `app.on_swapchain_recreate(spd)`.
  3. `request_swpc_img`: waits the frame's fence, then acquires. This must precede `update`, which writes that frame's instance SSBO. `Ok(None)` means the swapchain is out of date: skip the frame and recreate next tick.
  4. `app.update(dt, spd)`. If that leaves the module faulted the frame is dropped there, unpresented, so the window keeps the last good image.
  5. `cmd_begin` → `cmd_render_begin` → `spd.draw()` → `cmd_render_end` → `cmd_end` → `cmd_submit` → `present_swpc`. An out-of-date or suboptimal result schedules a recreate for the next tick.
  6. A manual FPS-cap sleep, only when `vsync = false` and `cap_fps = true`.
- F5 (in `window_event`) → `app.reload()`; a `true` return schedules a swapchain recreate, whose `on_swapchain_recreate` re-uploads textures into the fresh database.
- `exiting` → `app.on_unload()`, then **`destroy_swapchain()` explicitly**, because winit drops the surface before the `VulkanContext` would otherwise be dropped.

Rendering pauses entirely while the window has zero size (minimized); focus changes do not affect rendering, and any non-zero `Resized` resumes. A `Resized` to a size different from the last observed one (this also happens once when entering borderless fullscreen) schedules the swapchain recreate, which **rebuilds the `SpriteDatabase` from scratch**, which is why `on_swapchain_recreate` exists: its job is to re-upload every texture, since the old GPU textures died with the old database. `config.resolution` is a fixed *design* resolution and is never overwritten by the real window size.

### Vulkan layer (`surge-core/src/vulkan/`)

`VulkanContext` is one large struct in `mod.rs`; its `impl` blocks are split across `ctx_*.rs` by concern (`ctx_new_drop`, `ctx_swpc`, `ctx_command`, `ctx_graphics_pipeline`, `ctx_shader`; the last also holds the `GraphicsPipelineBuilder`). Free-function helpers used during init live in `instance.rs`, `device.rs`, `image.rs`, `command.rs`. When adding context methods, put them in the matching `ctx_*.rs` rather than growing `mod.rs`.

Swapchain notes: the extent is `current_extent` unless the surface reports the `u32::MAX` sentinel; a recreate passes the old swapchain as `old_swapchain` and destroys it once the new one exists; the depth image is sized to the swapchain, not to the design resolution.

The context is shared as `Arc<RefCell<VulkanContext>>`. GPU resource wrappers (`buffer::Buffer`, `texture::Texture`) each clone that `Arc` and free themselves in `Drop` — RAII, no manual teardown. Beware: methods borrow the `RefCell` per call, so chained `context.borrow()` inside a `borrow_mut()` scope will panic at runtime.

`FRAMES_IN_FLIGHT = 2` drives the per-frame duplication of the acquire semaphores (`present_completed_sem`), fences, command buffers, and the instance SSBOs. `render_finished_sem` is deliberately different: one per **swapchain image**, since the presentation engine holds it until that image is re-acquired.

Colour is linear end to end: 8-bit textures are `R8G8B8A8_SRGB` (decoded on sampling), 16-bit ones are linearised on the CPU, an sRGB swapchain format is required (UNORM is a warned, too-dark fallback), and `clear_color` is converted to linear because clears are written raw. Host-visible buffers are persistently mapped; read the pointer through `Buffer::mapped_ptr()` (never cache it) and call `Buffer::flush()` after writing.

`surge-core` also has a `cli` module (just the startup logo).

### Sprite database — the only draw path

`SpriteDatabase` (`vulkan/sprite_database/mod.rs`) is an immediate-mode 2D batcher. Per frame a module calls `add_instance(&InstanceInfo)` any number of times; `draw()` issues **one instanced draw of 6 vertices** and resets occupancy to zero. Nothing persists between frames — a sprite not re-added disappears.

- Vertex positions and UVs are baked into `sprite.vert` (no vertex buffers). Per-instance data (`model`, `color`, `subtexture_data`, `material`) comes from an SSBO addressed via a **buffer device address passed in a push constant** (`GL_EXT_buffer_reference`).
- Two separate budgets, both hardcoded to **32** in the player (`CreateInfo`): `max_sprites` caps live instances per frame, `max_textures` caps uploaded textures (and sizes the descriptor array). Exceeding the instance cap, or using an out-of-range `texture_id`, logs a warning and drops that request; exceeding the texture cap makes `upload_texture`/`upload_decoded_texture` return `VulkanError::TextureDatabaseFull` (modules see it as a `CanvasError` through `SpriteCanvas::upload_texture`, and both modules `.unwrap()` it, so this panics).
- `texture_id` in `InstanceInfo` is the **index in upload order**; `upload_texture` accepts PNG only, and only RGBA once decoded (8- or 16-bit; other colour types are rejected with `UnsupportedTextureColorType`). Upload only *records* the GPU copy: the next `draw()` submits every pending upload at once and rewrites the descriptor set behind a `device_wait_idle`, so upload in bursts (`on_load`/`on_swapchain_recreate`), not mid-frame.
- `subtexture_info` selects a sub-rect for sprite-sheet animation, converted to a UV transform by `subtexture_uv_transform` (derivation in `extra/subimage.wls`). A rect that is not fully inside the texture (ending exactly on the edge is inside) falls back to the full texture with a warning.
- `draw()` letterboxes the design resolution (`window_width`/`window_height`) into the real swapchain extent with the viewport and scissor, so borderless fullscreen keeps the aspect ratio; the bars keep the clear colour.
- Coordinates: orthographic, origin at the **top-left**, +x right, **+y down**, z in `[0, 1]`. The depth test is `GREATER_OR_EQUAL`, so **larger z draws behind** — backgrounds get the higher z (2048 uses board `0.5`, pieces `0.0`).

Shaders are GLSL 460 in `sprite_database/shaders/`; edit the `.vert`/`.frag` and rebuild — `build.rs` reruns `glslang`. If you change the SSBO/UBO/push-constant layout, update both the shader struct and the matching `#[repr]` Rust struct in `sprite_database/mod.rs`.

### Config (`surge-sdk/src/config.rs`)

`config.toml` deserializes into `EngineConfig` via serde, then `validate()` rejects a zero resolution and `cap_fps = true` with `fps_cap = 0`. Unknown keys are silently ignored (there is no `deny_unknown_fields`), so a stale or misspelled key does nothing; the window is hardcoded `.with_resizable(false)` and there is no MSAA setting. `windowed = false` means borderless fullscreen on the primary monitor.

### `surge-mod-2048` (work in progress)

The active development target. `App2048` (`app_2048.rs`) drives a `VecDeque<BoardState>` queue of `Idle` / `Move(dir)` / `Resolve`; input is only accepted while `board_idle()` (an empty queue also counts as idle). Each `update` advances that state machine first and only then submits the board and piece sprites, so what is drawn is always the current frame. `Piece` stores both a logical slot (`c_slot`/`t_slot`) and a pixel position (`c_pos`/`t_pos`), approaching the target exponentially at a frame-rate-independent rate in `update_pos`, which returns `true` when settled — the `Move` state pops once every piece has arrived. `Piece::value` is a texture index (`1..=2`), not a tile value. `BoardGeometry` maps 4×4 slots to pixels for the fixed 500×800 window.

Per-direction target selection lives in its own file: only `right_ops.rs` (`set_move_target_right`) exists so far. `Up`/`Down`/`Left` push states but set no targets, and merging, spawning after a move, score and win/lose are not implemented. Note the state push order differs between the arrow-key arms in `lib.rs` — `Up` pushes `Resolve` before `Move`; the others push `Move` first. Follow the `Right` arm as the reference when adding the remaining directions.

The module has unit tests for `BoardGeometry`, `reconstruct_board` and `set_move_target_right`. `App2048::with_pieces` and `Piece::new` let a test place pieces on chosen slots instead of the random start.
