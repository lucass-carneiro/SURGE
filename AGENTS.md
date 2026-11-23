# AGENTS.md

This file provides guidance to coding agents when working with code in this repository.

# Workflow rules

- **Suggest a commit message; never commit.** After finishing a change, give a suggested commit message of exactly two short sentences: the first summarizes *what changed*, the second *why*.
- Always ask questions instead of making autonomous decisions
- Be a harsh critic of your own conclusions

# Coding style guidelines

- Do not add verbose comments in generated source code. Only add comments that explain why something is needed when the context is not clear.
- Code comments with explanations should very rarely be longer than 1 or 2 sentences.
- Code comments should not make explicit references to design plans and docs.
- Comments should be self-contained and self standing.
- When creating test cases for a module, never mix test code and module code. Separate module code and unit tests in different files.

# Documents

- Code reviews live in `docs/code_reviews/`, numbered `review_<n>.md`
- Design plans live in `docs/design/`.

Design plans should have the following general section structure:
  1. What is being implemented.
  2. What problem the implementation solves.
  3. General design overview, AKA, how it all fits together.
  4. Implementation plan split into small steps that each represent an appropriately sized self-contained commit of the work phases.

When a step is completed, add a ✅ to the end of its heading (e.g. `### Step 0: ... ✅`). Below the step's original text, add a `**Resolution:**` paragraph: what was done, deviations from the plan, and verification results (tests, measurements). Keep the original step text unchanged. A heading without ✅ means the step isn't done yet.

# Build & run

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

# The CWD contract

The player resolves nearly everything relative to the process working directory, so **always run from the repo root** during development:

- `config.toml` is read from CWD. It is **gitignored** — the checked-in copies live at `surge-modules/<module>/config.toml`, and the root one is a per-developer working copy (typically a module's config with `app_folder` repointed at `target/debug`).
- SPIR-V is **not** read from disk at runtime. `surge-core/build.rs` compiles both databases' shaders — `sprite_database/shaders/sprite.{vert,frag}` and `font_database/shaders/font.{vert,frag}` — with `glslang` into its own Cargo `OUT_DIR`, and each database's `mod.rs` pulls the bytes in at compile time with `include_bytes!(concat!(env!("OUT_DIR"), "/sprite_vert.spv"))`, so the SPIR-V ships inside the player binary itself. There is no `shaders/` directory, no symlinks, and nothing for the stager to copy.
- Module asset paths go through `surge_sdk::resolve_asset_path(module_name, relative)`, which tries `<exe_dir>/assets/` first (staged layout) and falls back to the repo-root-relative `surge-modules/<module_name>/assets/` (dev), so a module runs correctly from the repo root in dev and from a staged folder.

`stager/stager.py` produces a self-contained, CWD-correct release layout:

```bash
python3 stager/stager.py list
python3 stager/stager.py stage surge-mod-2048 [-o]   # -> staging-surge-mod-2048/
```

It requires `cargo build --release --features release-bin` to have run and must itself be invoked from the repo root. It copies the module's single `<module>_release` binary, the module's own `config.toml`, and `assets/` (minus the `assets/svg/` source artwork) — nothing else (shaders are embedded in the binary). The staged binary still reads `config.toml` from CWD, so run it with the staging folder as the working directory.

# Feature flags

- `surge-core/validation_layers` — **on by default**. Enables Vulkan validation layers + debug messenger, and gates several `VulkanError` variants behind `cfg`.
- `<module>/release-bin` — pulls in the optional `surge-player` dependency and enables the module's `<module>_release` `[[bin]]` (`required-features`). Without it only the dev binary builds.

PNG, SVG, and TTF assets are tracked with **git-lfs** (see `.gitattributes`).
