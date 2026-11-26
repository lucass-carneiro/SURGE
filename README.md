# Welcome to SURGE

**SURGE** is the **S**uper **U**nder**R**ated **G**ame **E**ngine

A prototype game engine made for fun (*and profit ?*).

# Table of Contents
1. [Philosophy](#philosophy)
2. [Repository layout](#repository-layout)
3. [Building and running](#building-and-running)
    1. [Dependencies](#dependencies)
    2. [Getting the sources](#getting-the-sources)
    3. [Build options](#build-options)
    4. [The CWD contract](#the-cwd-contract)
    5. [Development](#development)
    6. [Hot reloading](#hot-reloading)
    7. [Tests](#tests)
4. [Release builds and staging](#release-builds-and-staging)
5. [Configuration](#configuration)
6. [Documentation](#documentation)

# Philosophy

**SURGE** is divided in three logical components: a *player*, a *core* and *modules*. The *core*
contains the engine's facilities: the window, the Vulkan setup, the sprite renderer and the text
renderer. The *player* owns the event loop and runs a *module*. Modules are the games: they use the
services the *core* provides, through a small API, to draw sprites and text and to react to the
user's keyboard and mouse.

A module is written against `surge-sdk` alone — the crate that holds the `SurgeApp` trait modules
implement and the `SpriteCanvas` trait they draw through. It never names `surge-core`, and the
engine is never a compile-time dependency of the game logic.

How that boundary is *linked* depends on the build:

- **Dev build.** The module is a standalone binary that depends only on `surge-sdk`. The player is
  a separate process; the two talk over a loopback socket, which is what makes hot reloading
  possible.
- **Release build.** With the module's `release-bin` feature, the module additionally depends on
  `surge-player` (and so, transitively, on `surge-core`) and links the player loop directly into
  itself, producing a single self-contained process with no IPC.

Text is split across the boundary the same way: shaping happens module-side in `surge-sdk` (via
[`harfrust`](https://crates.io/crates/harfrust)), and the resulting positioned glyphs travel to the
player, which rasterizes them ([`skrifa`](https://crates.io/crates/skrifa) outlines,
[`zeno`](https://crates.io/crates/zeno) coverage), caches them and packs them into a glyph atlas.

# Repository layout

| Crate | Role |
|-------|------|
| `surge-sdk` | The module-facing API and shared plain data: `SurgeApp`, `SpriteCanvas`, sprite types, text shaping and run types, config parsing, asset-path resolution and the dev-mode IPC wire format. |
| `surge-core` | The engine library: window and Vulkan setup, the sprite renderer (`sprite_database`) and the text renderer (`font_database`). Modules never name it. |
| `surge-player` | Exposes `run(app, config)` (the event loop and frame) and the dev-mode player binary that spawns and drives a module process. |
| `surge-modules/*` | The games. Each one is a library plus two binaries: a dev binary and a release binary. |
| `stager/` | Python tool that produces a self-contained release folder for a module. |
| `docs/` | Design plans (`docs/design/`) and code reviews (`docs/code_reviews/`). |

The modules currently in the workspace:

| Module | What it is |
|--------|------------|
| `surge-mod-2048` | A playable 2048, with sprites and a text score display. The reference "real game". |
| `surge-mod-default` | A sprite-only demo: a moving, sprite-sheet-animated face. The module the player's reload test builds. |
| `surge-mod-text` | A text-only demo that exercises the whole shaping → wire → rasterization → atlas → draw path, including a live counter that pushes new glyphs through the cache at runtime. |

# Building and running

## Dependencies

1. `git` and `git-lfs` — PNG, SVG and TTF assets are stored with LFS (see `.gitattributes`).
2. A Rust toolchain (edition 2024).
3. The Vulkan SDK. **`glslang` must be on `PATH`**: `surge-core/build.rs` compiles the GLSL shaders
   with it, and the build hard-fails if it is missing or if the GLSL does not compile.
4. A Vulkan 1.3 capable driver.
5. `python3` for the stager.
6. *Optional:* the Khronos validation layers, needed only for builds that enable the
   `validation_layers` feature. They are **not** enabled by default.

## Getting the sources

```bash
git clone https://github.com/lucass-carneiro/SURGE
cd SURGE
git lfs pull
```

## Build options

### Feature flags

| Feature | Crate | Default | What it does |
|---------|-------|---------|--------------|
| `validation_layers` | `surge-core`, forwarded by `surge-player` and by every module | off | Enables the Vulkan validation layers and the debug messenger. It also gates several `VulkanError` variants and one unit test behind `cfg`. Forwarded under the same name everywhere, so `--features validation_layers` works from any of the three. For a *module* it only takes effect alongside `release-bin`, since that is what pulls `surge-player` in. |
| `release-bin` | each `surge-modules/*` | off | Pulls in the optional `surge-player` dependency and enables the module's `<module>_release` binary, which lists it in `required-features`. Without it only the dev binary builds. |

**Why `validation_layers` is deliberately not a default.** Cargo features are additive and unified
per invocation, so a default feature would be pulled into every workspace-wide build — including the
release build that produces the staged binaries — and `release-bin` could not switch it back off.
Staging therefore hard-fails on a binary that still contains the layers (the stager scans the binary
for the `VK_LAYER_KHRONOS_validation` string).

### Binaries

| Binary | Built by | What it is |
|--------|----------|------------|
| `target/debug/surge-player` | `cargo build` | The dev player. Reads `config.toml`, spawns the module named there as a child process and drives it over IPC. |
| `target/debug/surge_mod_<name>` | `cargo build` | A module's dev binary. Not launched by hand — the player spawns it. |
| `target/release/surge_mod_<name>_release` | `cargo build --release --features release-bin` | A module's self-contained release binary, player loop linked in. This is what the stager ships. |

### Commands

```bash
cargo check --workspace                        # fastest correctness loop
cargo build                                    # debug: surge-player + one dev binary per module
cargo build --features validation_layers       # same, with Vulkan validation enabled
cargo run -p surge-player                      # normal dev run; MUST be from the repo root
cargo run -p surge-player --features validation_layers
cargo test --workspace                         # all tests; none needs a GPU or a window
cargo build --release --features release-bin   # <module>_release binaries, required before staging
```

## The CWD contract

The player resolves nearly everything relative to the process working directory, so **always run
from the repository root** during development.

- **`config.toml` is read from the CWD**, with no command-line override. The root `config.toml` is
  **gitignored** — it is a per-developer working copy. The checked-in copies live at
  `surge-modules/<module>/config.toml`; copy one to the repo root and point `app_folder` at
  `target/debug` to get started.
- **SPIR-V is never read from disk at runtime.** `surge-core/build.rs` compiles both databases'
  shaders — `sprite_database/shaders/sprite.{vert,frag}` and `font_database/shaders/font.{vert,frag}`
  — into its own Cargo `OUT_DIR`, and each database pulls the bytes in at compile time with
  `include_bytes!`. The SPIR-V ships inside the binary: there is no `shaders/` directory and nothing
  for the stager to copy.
- **Module assets** go through `surge_sdk::resolve_asset_path(module_name, relative)`, which tries
  `<exe_dir>/assets/` first (the staged layout) and falls back to the repo-root-relative
  `surge-modules/<module_name>/assets/` (dev). The same module therefore runs correctly both from
  the repo root in dev and from a staged folder.

## Development

```bash
cp surge-modules/surge-mod-2048/config.toml config.toml
# then edit [startup_app].app_folder to "target/debug"
cargo build
cargo run -p surge-player
```

Logging goes through `env_logger` and defaults to the `debug` level; set `RUST_LOG` to override it,
e.g. `RUST_LOG=info cargo run -p surge-player`. In dev mode both processes initialize it and the
module inherits the environment, so the terminal carries interleaved player and module output.

## Hot reloading

In dev mode the player runs the module as a child process and talks to it over a loopback socket.
Press **F5** in the player window to stop the module, respawn its (possibly rebuilt) binary and
reload it — so the edit loop is `cargo build` in one terminal, F5 in the window. If the module
crashes, the window freezes on the last frame and its title gains ` — MODULE CRASHED` until a
successful F5. Dev mode supports Linux and Windows.

## Tests

Tests live in `surge-core`, `surge-sdk`, `surge-player` and `surge-mod-2048`. No test needs a GPU or
a window: Vulkan-facing logic is kept in pure helpers so it can be tested directly.

```bash
cargo test --workspace
cargo test -p surge-core font                    # narrow by crate and filter
cargo test --workspace --features validation_layers   # also runs the one debug-messenger test
```

`surge-player`'s reload test builds `surge-mod-default` itself through a nested `cargo build`, so
that one is slower than the rest.

# Release builds and staging

A release module is a single process with the player loop linked in. Staging turns it into a
self-contained, CWD-correct folder.

```bash
cargo build --release --features release-bin     # produces target/release/<module>_release
python3 stager/stager.py list                    # lists the modules it can stage
python3 stager/stager.py stage surge-mod-2048    # -> staging-surge-mod-2048/
python3 stager/stager.py stage surge-mod-2048 -o # same, overwriting an existing folder
```

`stager.py` must itself be **invoked from the repository root** — it verifies the CWD is the SURGE
root by checking `Cargo.toml` and its workspace members, and exits otherwise. It also refuses to
stage a module whose release binary has not been built, has no `config.toml`, or still carries the
Vulkan validation layers.

Without `-o`, an existing `staging-<module>/` folder is left untouched and the stager exits
successfully.

The staged folder contains exactly three things:

```
staging-surge-mod-2048/
├── surge_mod_2048_release   # the module's single release binary
├── config.toml              # copied from surge-modules/surge-mod-2048/config.toml
└── assets/                  # the module's assets, minus assets/svg/
```

`assets/svg/` is skipped: it holds the source artwork the PNGs were exported from, not something the
module loads at runtime. Shaders are not copied because they are embedded in the binary. Nothing
else — no player binary, no engine library — is needed.

The staged binary still reads `config.toml` from the CWD, so **run it with the staging folder as the
working directory**:

```bash
cd staging-surge-mod-2048
./surge_mod_2048_release
```

`staging-*` folders are gitignored.

# Configuration

`config.toml` is parsed by `surge_sdk::config::parse_config`. Every section and key below is
required.

| Section | Key | Type | Meaning |
|---------|-----|------|---------|
| `[window]` | `name` | string | The window title. The player appends ` — MODULE CRASHED` to it while a dev module is faulted. |
| | `windowed` | bool | `true` for a window, `false` for borderless fullscreen on the primary monitor. |
| `[resolution]` | `width` | u32 | Design resolution width. |
| | `height` | u32 | Design resolution height. |
| `[renderer]` | `vsync` | bool | `true` requests the FIFO present mode; `false` requests IMMEDIATE and falls back to FIFO with a warning if the driver lacks it. |
| | `cap_fps` | bool | Sleep at the end of each frame to hold `fps_cap`. Only consulted when `vsync` is `false`. |
| | `fps_cap` | u32 | The target frame rate used when `cap_fps` applies. |
| `[clear_color]` | `r`, `g`, `b`, `a` | f64 | The background color, each component in `0.0..=1.0`. |
| `[startup_app]` | `app_folder` | string | Directory holding the module's dev binary, resolved relative to the CWD. Typically `target/debug`. Unused by a release binary, which has its module linked in. |
| | `app_name` | string | The dev binary's file name, e.g. `surge_mod_2048`. |

`[resolution]` is the **design resolution**: it is the coordinate space modules place sprites and
text in, and it is the initial window size in windowed mode. The renderers project it onto the
swapchain with letterboxing, so the aspect ratio is preserved whatever the real surface size is.
`SurgeApp::cursor_moved_event` undoes that mapping and hands modules positions already in design
coordinates, which means a position over a letterbox bar falls outside `0..width` / `0..height`.

Two values are rejected at load time, before the window is created: a zero `width` or `height`, and
a `fps_cap` of `0` while `cap_fps` is `true`.

See `surge-modules/*/config.toml` for complete examples.

# Documentation

- `docs/design/` — design plans, one per subsystem. Each one states what is being implemented, the
  problem it solves, the overall design, and an implementation plan broken into commit-sized steps.
- `docs/code_reviews/` — numbered code reviews, `review_<n>.md`.
- `AGENTS.md` — guidance for coding agents working in this repository.
