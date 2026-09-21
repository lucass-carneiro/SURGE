# Welcome to SURGE

**SURGE** is the **S**uper **U**nder**R**ated **G**ame **E**ngine

A prototype game engine made for fun (*and profit ?*). It is a Rust workspace that renders 2D sprites with Vulkan (via [`ash`](https://crates.io/crates/ash)).

# Table of Contents
1. [Philosophy](#philosophy)
2. [Repository layout](#repository-layout)
3. [Building and running](#building-and-running)
    1. [Dependencies](#dependencies)
    2. [Development](#development)
    3. [Hot reloading](#hot-reloading)
    4. [Release builds and staging](#release-builds-and-staging)
4. [Configuration](#configuration)

# Philosophy

**SURGE** is divided in three logical components: a *player*, a *core* and *modules*. The *core* contains the engine's facilities: the window, the Vulkan setup and the sprite renderer. The *player* owns the event loop and runs a *module*. Modules are the games: they use the services the *core* provides, through a small API, to draw sprites and to react to the user's keyboard and mouse.

A module never links the engine. It depends only on `surge-sdk`, which holds the `SurgeApp` trait modules implement and the `SpriteCanvas` trait they draw through.

# Repository layout

| Crate | Role |
|-------|------|
| `surge-sdk` | The module-facing API and shared plain data: `SurgeApp`, `SpriteCanvas`, sprite types, config parsing, asset-path resolution and the dev-mode IPC. |
| `surge-core` | The engine library: window and Vulkan setup and the sprite renderer. Modules never link it. |
| `surge-player` | Exposes `run(app, config)` (the event loop and frame) and the dev-mode player binary. |
| `surge-modules/*` | The games. Each one is a library plus two binaries: a dev binary and a release binary. |
| `stager/` | Python tool that produces a self-contained release folder for a module. |

# Building and running

## Dependencies

1. `git` and `git-lfs` (PNG and SVG assets are stored with LFS)
2. A Rust toolchain (edition 2024)
3. The Vulkan SDK. `glslang` must be on `PATH`: `surge-core/build.rs` compiles the GLSL shaders with it, and the build fails if it is missing.
4. `python3` for the stager
5. A Vulkan 1.3 capable driver. The Khronos validation layers are enabled by default (the `validation_layers` feature of `surge-core`), so they must be installed too.

```bash
git clone https://github.com/lucass-carneiro/SURGE
cd SURGE
git lfs pull
```

## Development

The player reads `config.toml` from the **current working directory**, so always run from the repository root. `config.toml` is gitignored; copy one of the module configs to start (`surge-modules/<module>/config.toml`) and point `app_folder` at `target/debug`.

```bash
cargo check --workspace
cargo build                # target/debug/surge-player plus one dev binary per module
cargo run -p surge-player  # from the repository root
cargo test --workspace
```

## Hot reloading

In dev mode the player runs the module as a child process and talks to it over a loopback socket. Press **F5** in the player window to stop the module, respawn its (possibly rebuilt) binary and reload it. If the module crashes, the window freezes on the last frame and its title shows `MODULE CRASHED` until a successful F5. Dev mode supports Linux and Windows.

## Release builds and staging

A release module is a single process with the player loop linked in.

```bash
cargo build --release --features release-bin
python3 stager/stager.py list
python3 stager/stager.py stage surge-mod-2048      # -> staging-surge-mod-2048/ (add -o to overwrite)
```

The staged folder holds the module's `<module>_release` binary, its `config.toml` and its `assets/`. Run the binary with the staged folder as its working directory.

# Configuration

`config.toml` has the sections `[window]`, `[resolution]`, `[renderer]`, `[clear_color]` and `[startup_app]`. See `surge-modules/*/config.toml` for complete examples.
