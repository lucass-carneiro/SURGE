# Text Rendering (TTF + HarfRust shaping + glyph atlas)

**Status:** Draft (pass 1, review-corrected). The four structural decisions in §3.0 were settled with
the author; the items in §6 (Open decisions) are deliberately left for review before Step 0 starts. A
harsh design-review pass (2026-11-22, two independent reviewers + crate-fact verification against
docs.rs) corrected the font-parsing framing (§3.2 / §Step 0), the atlas upload/sync rationale
(§3.2 / §Step 5), and several over-claims, and added atlas gutters and dropped-glyph memoization. No
code has been written.

---

## 1. What is being implemented

A text-rendering subsystem for the SURGE engine that lets a module draw strings in a TTF/OTF font:

- **Shaping** (string + font + size → positioned glyph IDs) with **HarfRust**, running **module-side
  in `surge-sdk`**, so a module can *measure* text (width, ascent, descent, line height) and lay out
  its own UI around it.
- **Rasterization + caching** of glyph bitmaps into a GPU **glyph atlas**, and drawing each shaped
  glyph as a textured quad — the exact mechanism `sprite_database` already uses for sprites. This
  lives in a new **`surge-core/src/vulkan/font_database`** module, a sibling of `sprite_database`.

Scope for this first version (v1) is deliberately narrow: a **single shaped run** — one font, one
pixel size, one line, left-to-right, no wrapping — placed at a point with an alignment anchor
(left / center / right, top / baseline / bottom). Everything else is explicitly deferred (§6.5).

## 2. What problem it solves

Today the engine has **no text**. A module that wants to show a string must bake it into a PNG ahead
of time — one sprite per possible value. That does not scale: any dynamic string (a score, a timer, a
name, a debug overlay) is impossible without pre-rendering every value as artwork. A module needs to
render arbitrary strings at runtime from a font file, and it needs to know how big that text will be
so it can position and size the UI around it.

## 3. Design overview — how it all fits together

### 3.0 The four settled decisions (and why)

The architecture is shaped by one hard constraint in the existing code: **in dev mode the module is a
separate process and the player↔module wire is one-way per turn** — `wire.rs` states "the module has
no way to ask the player anything mid-turn." A module therefore cannot query the renderer for text
metrics synchronously. That single fact drives decision (1).

| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| 1 | Where shaping runs | **Module-side, in `surge-sdk`** | The module can't query the renderer mid-turn (the wire forbids it), so it holds the font and measures locally; the only alternative — the player shipping metrics back at load time — is clumsier and still can't do full glyph positioning. The module shapes, measures, lays out, and ships a *positioned glyph run*. |
| 2 | How glyph quads reach the screen | **Dedicated `font_database`** (own mutable atlas, own pipeline/shader) | A glyph atlas is *mutated incrementally* as glyphs are cached; the sprite DB's texture array is "upload once, never mutate". Mirroring `sprite_database` keeps the structure the author asked for and keeps the dynamic-atlas lifecycle out of the sprite path. |
| 3 | Rasterizer stack | **skrifa (outlines) + zeno (coverage)** | skrifa and HarfRust both parse with the fontations `read-fonts` stack (each wraps it in its own `Font` type), so the two sides share a parsing family without sharing a font object. zeno is the path rasterizer from the swash family; returns a grayscale coverage mask. skrifa's outline does not pipe directly into zeno — a small adapter translates it (§3.2). No hinting — fine for GPU-scaled game text. |
| 4 | v1 scope | **Minimal single-run** | Keeps the first commit chain small and testable; layout (wrapping, bidi, rich text, SDF) is a later plan. |

### 3.1 The data path, end to end

```
MODULE (surge-sdk, process A in dev / linked in release)
  Font::load(canvas, "fonts/X.ttf")
      ├─ canvas.load_font("fonts/X.ttf")  ──► records path, returns cumulative FontId
      └─ parse HarfRust face from the same path (for shaping)      (both sides parse the file)
  let shaped = font.shape("Score: 42", 32.0)      // HarfRust → positioned glyphs (font units; SDK scales to px, §3.5)
  shaped.width(), shaped.ascent(), ...            // module uses these to lay out its UI
  canvas.draw_text(&shaped, origin, anchor, color, z)   // SDK applies anchor offset,
                                                        // emits a TextRun into the canvas

WIRE (dev mode only): CanvasOps gains
  font_loads: Vec<String>            // like texture_uploads
  text_runs:  Vec<TextRun>           // { font_id, px_size, color, z, glyphs: [{glyph_id, x, y}] }

PLAYER (surge-core font_database, process B in dev / same process in release)
  load_font(path)  ──► skrifa parses outlines, assigns FontId (== module's, same order)
  add_text_run(run):
      for each glyph:
          key = (font_id, glyph_id, round(px_size))
          on cache miss: skrifa outline → zeno coverage bitmap → pack into atlas (shelf allocator)
                         → record a partial buffer→image copy into the persistent atlas image
          append an InstanceRecord { model(quad at pen + bearing, sized to bitmap),
                                     color, subtexture_data = atlas cell UVs, z }
  draw():
      flush pending atlas uploads (barrier; device drain on a miss frame)
      bind font pipeline + atlas descriptor, set letterboxed viewport (shared with sprites),
      draw `occupancy` instances, reset occupancy
```

The module shapes and knows every glyph's **pen position on the baseline** (design-space, y-down).
The player adds the **rasterization bearing** (known only after rasterizing: left bearing, top
bearing) to place the quad's top-left. This split is deliberate: advances/positions are a shaping
concern (module), bitmap placement is a raster concern (player), and they compose cleanly.

### 3.2 What is new vs. reused

**Reused wholesale (patterns and building blocks):**
- The instanced-quad draw: `FrameGlobals` UBO (view+proj), per-instance SSBO addressed by buffer
  reference, `PushConstants` with the SSBO device address, `FRAMES_IN_FLIGHT` ring of SSBOs — all
  mirror `sprite_database`.
- `make_ortho_projection`, `make_view`, `make_model_matrix`, `letterbox`, `subtexture_uv_transform`
  — these are already pure and reusable; the plan **promotes the shared ones** (`letterbox`, the
  projection/view, the subtexture transform) so both databases call one copy rather than duplicating.
- `Buffer` (staging) and the immediate-command-buffer upload pattern (`cmd_immediate_begin/end/submit`,
  deferred `flush`). The atlas copy reuses only the raw `cmd_copy_buffer_to_image` call
  (`texture.rs:149`); it **cannot** reuse `Texture::record_upload_from_buffer` (`texture.rs:104-162`),
  whose whole-image `UNDEFINED → TRANSFER_DST → SHADER_READ_ONLY` path *discards* existing contents and
  copies the full extent. The atlas needs a genuinely new sub-region copy with
  `SHADER_READ_ONLY ↔ TRANSFER_DST` transitions (see "Genuinely new" below and §5).
- The build.rs glslang step: two new shaders (`font.vert`, `font.frag`) are added to the same array,
  compiled to `OUT_DIR`, and `include_bytes!`'d — identical to the sprite shaders, so the SPIR-V still
  ships inside the player binary (no files on disk, per the CWD contract).

**Genuinely new:**
- A **persistent, mutable** `R8_UNORM` atlas image with incremental sub-region uploads and the
  layout-transition/hazard handling that implies (vs. the sprite DB's immutable whole-texture upload).
- A **shelf (skyline-lite) atlas allocator**: pure CPU bookkeeping, unit-tested without a GPU. Each
  packed cell carries a **≥1px transparent gutter** on every side, and sampled UVs are inset by half a
  texel, so bilinear sampling of a GPU-scaled glyph never bleeds into its neighbours.
- A **glyph cache**: `HashMap<(FontId, GlyphId, u32 px), GlyphEntry>`; `GlyphEntry` = atlas cell rect
  (texels) + bitmap size + left/top bearing. On atlas overflow a **sentinel entry** (zero size, draws
  nothing) is cached so a dropped glyph is a cache *hit*, never re-rasterized or re-uploaded (§Step 5).
- A **skrifa-outline → zeno adapter**: skrifa draws a glyph outline into a `PathSink` (y-up, font
  design space); the adapter re-emits those commands as `zeno::Command`s (flipping to y-down) and
  rasterizes with `zeno::Mask`. skrifa and zeno don't compose directly — this small translation layer
  is new code, unit-testable on the coverage output. The quad's left/top bearing come from zeno's
  `Placement.left/top` rather than being hand-derived.
- The **HarfRust shaping layer** in `surge-sdk` (`Font`, `ShapedText`), plus the anchor math.
- The **font fragment shader**: samples single-channel coverage and writes
  `vec4(color.rgb, color.a * coverage)` — so text color comes from the per-run color and the atlas
  only stores coverage.

**The parse-once-per-side rule (font parsing).** HarfRust and skrifa both parse with the fontations
`read-fonts` stack, which is zero-copy and lazy: the parsed font borrows the font bytes and reads
tables on demand, so the cost of "parsing" is really one in-memory copy of the font bytes plus a
trivial table-directory setup. The plan keeps that cost to its unavoidable minimum:

- **Cross-process (dev mode) is inherent.** The module shapes and the player rasterizes in *different
  processes*; a borrowed, bytes-referencing font object cannot cross a socket, so each side loads its
  own copy. This is the price of module-side shaping (decision #1), not of the crate choice — any two
  libraries would do the same.
- **Within a side, parse once.** The SDK builds **one HarfRust font per file** and reads both shaped
  positions *and* vertical metrics from it (no skrifa on the SDK side, §Step 3); the player's
  `font_database` holds **one skrifa font per file** from which outlines are drawn (§Step 4).
- **No cross-crate type sharing is required (correction to the pass-1 framing).** The two sides never
  hand the *same* font object to both crates: HarfRust lives only in `surge-sdk`, skrifa only in
  `surge-core`, and current HarfRust and skrifa each wrap `read-fonts` in their **own** `Font` type
  rather than re-exporting a bare shared `read_fonts::FontRef`. So there is **no correctness
  requirement** that the two crates resolve to the same `read-fonts` version — two copies in the tree
  would still build and run. Step 0's `cargo tree -d` check is therefore **dependency hygiene, not a
  correctness gate** (a single copy keeps the tree small and keeps the §6 escape hatch — which *would*
  share types — viable). As of this writing HarfRust 0.16.1 and skrifa 0.50.0 both depend on
  `read-fonts` 0.47.x, so the tree can unify, but nothing breaks if a future bump diverges them.

So the only duplication that remains is one lazy, zero-copy byte buffer per font per process in dev —
negligible for the fonts in play, and accepted deliberately (§5). §6 records the escape hatch if a
profile ever says otherwise.

### 3.3 Player-side ownership and the composite canvas

Today the player stores `sprite_database: Option<SpriteDatabase>` and passes
`&mut dyn SpriteCanvas` (the DB itself) to every `SurgeApp` callback (`lib.rs:150,179,304`).
Text needs a second database, so the plan introduces a **borrowed composite** passed to the
callbacks:

```rust
struct Canvas<'a> { sprites: &'a mut SpriteDatabase, fonts: &'a mut FontDatabase }
impl SpriteCanvas for Canvas<'_> { /* sprite calls → sprites, text calls → fonts */ }
```

The player keeps both DBs as owned fields, builds a `Canvas` borrow at each of the three callback
sites, and in `frame()` draws `sprites.draw()` then `fonts.draw()` into the same render pass. The
shared depth buffer uses a `GREATER_OR_EQUAL` test (clear depth 0.0, `sprite_database/mod.rs:411`), so
equal-z ties resolve to text-on-top because fonts draw second. Note the depth buffer only orders
*opaque occlusion*: because both sprites and coverage text are alpha-blended, correct blending of
overlapping *translucent* fragments across the two passes is still draw-order dependent (text always
last), not resolved by z — the usual single-depth-buffer transparency caveat, inherited from the
existing sprite pipeline. This is the smallest
change that keeps the single-`&mut dyn SpriteCanvas` callback shape the whole codebase already relies
on. In dev mode, `remote_app::apply_ops` replays `font_loads` then `text_runs` onto this same
composite via the extended trait — no new transport.

### 3.4 The module-facing API (`surge-sdk`)

```rust
// Registers the font with the renderer (ships the path, gets a FontId) AND loads a HarfRust
// face for shaping. Must be called in on_load AND on_swapchain_recreate, exactly like texture
// uploads, because the player rebuilds both databases on recreate and restarts FontIds at 0.
let font = Font::load(canvas, "fonts/Inter.ttf")?;

let shaped = font.shape("Score: 42", 32.0);   // single run, LTR
let w = shaped.width();                        // design px; lay out UI around this
let h = shaped.line_height();

// Anchor says how `origin` maps onto the text box; the SDK resolves it from the measured
// metrics so the module never does baseline math itself.
canvas.draw_text(&shaped, origin, TextAnchor::Center, color, z);
```

`draw_text` is an **extension-trait default method** over `SpriteCanvas`, so the anchor math lives in
one place and off the trait object. The trait itself gains only two object-safe primitives, mirroring
the sprite ones:

```rust
fn load_font(&mut self, path: &str) -> Result<usize, CanvasError>;  // like upload_texture
fn add_text_run(&mut self, run: &TextRun);                          // like add_instance
```

### 3.5 Coordinate conventions (the main correctness risk)

HarfRust and skrifa use font-design conventions (y-up, units-per-em); the engine is y-down,
origin top-left, design-resolution pixels. The SDK is the single place that converts:
`px_per_unit = px_size / units_per_em`; advances accumulate along +x; the baseline sits at
`origin.y`; ascent is above it (smaller y). The player, after zeno rasterization, gets a bitmap whose
top bearing is the distance from the baseline up to the bitmap's top row, so the quad's top-left y is
`baseline_y − top_bearing`. Pen x/y are **rounded to integer design-px** before placement (v1
snapping; §6.3). Getting this wrong yields upside-down or baseline-drifting text, so Step 0 de-risks
it with a rasterize-one-glyph spike and Step 5 pins it under test.

## 4. Implementation plan (small, self-contained commits)

Each step compiles and is testable on its own. Per the repo's test contract, no step needs a GPU or a
window — GPU-facing logic is kept in pure helpers (packer, scaling, anchors, layout structs). Tests
go in separate files from module code.

### Step 0: Dependency spike — prove the external crates before wiring any GPU ✅
Pin `harfrust`, `skrifa`, and `zeno` to known-good versions and record them. Run `cargo tree -d` and
prefer a tree with a single `read-fonts`, but treat that as **dependency hygiene, not a correctness
gate**: HarfRust is used only in `surge-sdk` and skrifa only in `surge-core`, they never share a font
object, and each wraps `read-fonts` in its own `Font` type, so two `read-fonts` copies would still
build and run (§3.2). A single copy keeps the tree small and keeps the §6 escape hatch viable. (As of
this writing HarfRust 0.16.1 and skrifa 0.50.0 both resolve `read-fonts` 0.47.x.) Do **not** add a
direct `read-fonts` dependency or require version unification to proceed. Add the **Noto Sans TTF** to
the repo via git-lfs for tests to use (font file location for tests is the one open item in §6.1).
Write a throwaway integration test that: loads the font, shapes `"Ag"` (HarfRust output is in font
units — scale to px in the test), asserts two glyphs with increasing pen x and a plausible width;
rasterizes one glyph through the skrifa-outline→zeno adapter (§3.2) and asserts a non-empty coverage
bitmap with sane bearings read from zeno's `Placement`. This isolates the y-up↔y-down convention and
the font-unit scaling before any Vulkan code depends on them. **Verification:** the spike test passes;
`cargo tree -d` recorded (single `read-fonts` preferred, not required); versions recorded in the
Resolution.

**Resolution:** Done. Settled the three §6.1-adjacent open items with the author first: the test font
is a copy of the system `NotoSans-Regular.ttf` (607 KiB static TTF, Latin/Greek/Cyrillic, OFL)
committed at repo-root `test-assets/fonts/NotoSans-Regular.ttf`; tests read it via `include_bytes!`
(no CWD/asset-layout dependency); the throwaway spike lives in `surge-core/tests/font_spike.rs` with
`harfrust`/`skrifa`/`zeno` added as **surge-core dev-dependencies** (they move to their real homes —
harfrust→surge-sdk, skrifa+zeno→surge-core deps — in Steps 3–4).

- **git-lfs:** added `*.ttf filter=lfs diff=lfs merge=lfs -text` to `.gitattributes`; the font stages
  as a 131-byte LFS pointer.
- **Versions pinned** (exact, `=`): `harfrust 0.16.1`, `skrifa 0.50.0`, `zeno 0.3.3`. They resolve a
  **single** `read-fonts v0.47.0` and `font-types v0.12.7`; `cargo tree -d` shows none of
  read-fonts/font-types/skrifa/harfrust/zeno duplicated. Hygiene goal met (not required for
  correctness, per §3.2).
- **Shaping spike:** `"Ag"` shapes to exactly 2 glyphs with strictly increasing pen-x and a plausible
  advance width. Confirmed harfrust's default scale is identity, so positions come out in **font
  units**; the test applies `px_per_unit = px_size / units_per_em` (§3.5). API shape:
  `harfrust::Font::new(bytes, 0)` → `ShaperFont::new(&font)` → `Buffer::push_str` +
  `guess_segment_properties()` → free fn `harfrust::shape(..)`; read `glyph_infos()`/`glyph_positions()`.
- **Raster spike + convention finding (the main §3.5 risk):** the skrifa→zeno adapter rasterizes `'A'`
  @ 64px to a non-empty `41×46` Alpha coverage mask. **skrifa emits y-up** (outline y-range
  `[0, +45.89]`, baseline at 0). The adapter **negates y** to hand zeno the engine's y-down space and
  renders with zeno's **default `Origin::TopLeft`**. With that, an above-baseline glyph has a
  **negative** `Placement.top` (here `-46`): it is the signed baseline→top-row offset (up is negative),
  so quad-top-left `y = baseline_y + placement.top` and the baseline-to-top bearing is `-placement.top`
  (`46`, ~cap height — sane). `Placement.left` is the left bearing. This **matches the plan's §3.2
  "flipping to y-down"** wording and is the convention Step 5 must adopt. (A tried-and-rejected
  `Origin::BottomLeft` + no-flip approach yields `top=0` here, because zeno's non-explicit-size render
  path leaves its internal `height` at 0 in the BottomLeft branch — a dead end. Noted so it isn't
  re-attempted.)
- **Verification run:** `cargo test -p surge-core --test font_spike` → 2 passed, no warnings;
  `cargo test --workspace` green (dev-deps don't touch the normal build graph).

### Step 1: Wire types (`surge-sdk`, no GPU) ✅
Add `font_loads: Vec<String>` and `text_runs: Vec<TextRun>` to `CanvasOps`; define `TextRun` and
`PositionedGlyph`; bump `WIRE_VERSION` (2 → 3). **Verification:** extend the existing `CanvasOps`
postcard round-trip test (`every_request_and_response_variant_round_trips`, `wire.rs`) to cover fonts +
runs and a `TextRun`, mirroring the `InstanceInfo` round-trip test (which lives in `sprite.rs`).

**Resolution:** Done. `TextRun` and `PositionedGlyph` live in a new `surge-sdk/src/text.rs` module
(sibling of `sprite.rs`), re-exported from `lib.rs`. `TextRun` = `{ font_id: usize, px_size: f32,
color: Vector4<f32>, z: f32, glyphs: Vec<PositionedGlyph> }`; `PositionedGlyph` = `{ glyph_id: u32,
x: f32, y: f32 }` (pen position on the baseline, design-px y-down, per §3.1/§3.5). Both derive
`PartialEq` (unlike `InstanceInfo`) so tests compare directly. `CanvasOps` gained `font_loads` and
`text_runs`; every existing construction uses `CanvasOps::default()`, so only the wire test's struct
literal needed updating. `WIRE_VERSION` bumped 2 → 3.

- **Deviation from the plan's verification note:** the doc suggested mirroring the `InstanceInfo`
  round-trip test *in `sprite.rs`* (i.e. inline). Per AGENTS.md's "separate test and module code" rule
  (confirmed with the author), the new `TextRun` round-trip test instead lives in a dedicated
  `surge-sdk/tests/text_run_round_trip.rs` (requiring a `postcard` dev-dependency). The `wire.rs`
  `CanvasOps` round-trip test was extended in place as specified (it now populates `font_loads` and
  `text_runs`), since the plan says to extend that existing test.
- **Verification run:** `cargo test -p surge-sdk` → 17 unit + 1 integration test pass, including the
  extended `every_request_and_response_variant_round_trips` and the new
  `text_run_round_trips_through_postcard`; `cargo check --workspace` green (player's `apply_ops` takes
  `CanvasOps` by value and simply ignores the two new fields until Step 6).

### Step 2: Canvas trait + proxy (`surge-sdk`, no GPU) ✅
Add `load_font` and `add_text_run` to `SpriteCanvas`; implement on `CanvasProxy` (cumulative
`next_font_id`, reset in `cache_create_info` alongside `next_texture_id`; push runs into `ops`).
Update the object-safety witness in `canvas.rs` and the `CountingCanvas` test double in
`remote_app.rs`. **Verification:** proxy tests for cumulative font IDs across drains and reset on
recache (mirror the texture-ID tests).

**Resolution:** Done. `SpriteCanvas` gained `load_font(&mut self, path: &str) -> Result<usize,
CanvasError>` and `add_text_run(&mut self, run: &TextRun)`, both object-safe, so the existing
`const _: Option<&mut dyn SpriteCanvas> = None;` witness in `canvas.rs` needed no edit and now
covers them. `CanvasProxy` grew a cumulative `next_font_id` (reset in `cache_create_info` beside
`next_texture_id`); `load_font` pushes onto `ops.font_loads` and hands back the pre-increment index
fire-and-forget like `upload_texture`, and `add_text_run` clones the run into `ops.text_runs`.

- **Deviation (surge-core touched):** the step text names only `surge-sdk` and `remote_app.rs`, but
  adding two *required* trait methods also broke `impl SpriteCanvas for SpriteDatabase`
  (`surge-core`), which the player passes directly as `&mut dyn SpriteCanvas`. The real text routing
  is the Step 6 composite `Canvas`, so — confirmed with the author — `SpriteDatabase`'s two methods
  are temporary `unimplemented!("fonts are routed through the composite canvas")` stubs. They are
  unreachable before Step 6 (dev-mode `apply_ops` still ignores `font_loads`/`text_runs`, and no
  module calls `Font::load` yet), so the panic can only ever fire on a genuine wiring mistake, never
  silently drop text. The `CountingCanvas` double in `remote_app.rs` instead counts `font_loads` and
  `text_runs`.
- **Verification run:** three new `proxy::tests` — `font_index_is_cumulative_across_drains`,
  `font_index_resets_when_a_new_create_info_is_cached`, `text_runs_drain_in_order` (mirroring the
  texture-ID tests) — pass; `cargo test -p surge-sdk` → 20 unit + 1 integration green;
  `cargo check --workspace` green; `cargo test -p surge-player --no-run` builds the `CountingCanvas`
  double cleanly.

### Step 3: Shaping layer (`surge-sdk`, no GPU) ✅
Add `Font` (HarfRust face + scaled metrics + FontId), `ShapedText` (positioned glyphs + width +
ascent/descent/line height), `Font::load(canvas, path)`, `font.shape(text, px)`, `TextAnchor`, and the
`draw_text` extension-trait default method (anchor offset → `TextRun` → `add_text_run`). **Parse-once-
per-side rule:** `Font` owns one byte buffer and one HarfRust font built from it; HarfRust shapes from
it in **font units** (no ppem scaling is built in — the SDK applies `px_per_unit = px_size /
units_per_em`, §3.5, to positions *and* metrics). Vertical metrics (ascent/descent/line-gap) come from
the *same* font; specify the source explicitly and unit-test it: prefer the `OS/2` typographic metrics
(`sTypoAscender/sTypoDescender/sTypoLineGap`) when `fsSelection.USE_TYPO_METRICS` is set, else fall
back to `hhea`, reading whichever HarfRust exposes (font extents) or going through `read-fonts` tables
directly. The SDK side does **not** pull in skrifa, so it parses the font exactly once.
**Verification:** unit tests for font-unit→px scaling, metric-selection (typo vs hhea), measured width of a known string
in the bundled font, and anchor-offset resolution (center of width W puts origin at −W/2); all
pure/GPU-free.

**Resolution:** Done. New `surge-sdk/src/shaping.rs` module (sibling of `text.rs`), re-exported from
`lib.rs`. `harfrust = "=0.16.1"` moved from a `surge-core` dev-dependency to a real `surge-sdk`
dependency; no skrifa on the SDK side, so the SDK parses each font exactly once.

- **`Font`** owns one `harfrust::Font` (which owns the font bytes via `read-fonts`' shared `Arc`
  blob — no self-referential struct, no separate byte buffer). It stores `font_id`, `units_per_em`,
  and the vertical metrics in *font units*. `Font::load(canvas, path)` records the load
  (`canvas.load_font`), reads the file (`std::fs::read`), and parses; `Font::from_bytes(bytes,
  font_id)` is the public disk-free entry point (tests `include_bytes!` the bundled font through it).
  Per the texture convention, `load` takes an **already-resolved** path (the module calls
  `resolve_asset_path` itself), confirmed with the author; the SDK stays free of module-name
  plumbing.
- **Metric source (settled explicitly).** `harfrust::Font` *is* `read-fonts`' model font and exposes
  `.metrics()` → a `Metrics` carrying `use_typo_metrics`, `typo_line`, and `hhea_line`. The pure
  `select_vertical_metrics(use_typo, typo, hhea)` helper implements the plan's rule — OS/2 typo line
  when `fsSelection.USE_TYPO_METRICS` is set, else `hhea`, falling back to whichever is present — and
  normalizes the descender's sign to a non-negative magnitude. **No direct `read-fonts` dependency
  was added** (everything is reached through harfrust's re-export), and no skrifa is pulled in, so the
  parse-once-per-side rule holds. Noto Sans resolves `upem=1000`, ascent≈34.2px / descent≈9.4px /
  gap=0 at 32px.
- **`shape(text, px)`** builds a fresh `ShaperFont` per call (as the Step 0 spike does — it borrows
  the `Font`, so it cannot be stored alongside it), shapes one LTR run, and scales positions by
  `px_per_unit = px / units_per_em` (§3.5): advances accumulate along +x, HarfRust's y-up `y_offset`
  is negated into the engine's y-down space (baseline at local y = 0). `ShapedText` exposes
  `width/ascent/descent/line_gap/line_height/metrics/px_size/font_id/glyphs/len/is_empty`.
- **`TextAnchor`** is a flat 9-variant enum (`TopLeft … Center … BottomRight`; `Center` =
  horizontal-center + baseline), chosen with the author over a two-axis struct to match the doc's
  `TextAnchor::Center` spelling. **`draw_text`** is a default method on the `TextCanvasExt` extension
  trait (blanket-impl'd for every `SpriteCanvas`): it resolves the anchor offset against the measured
  box, adds the origin to each local glyph position, and emits a `TextRun` via `add_text_run`.
- **Deviation (test placement):** per the author's choice, the Step 3 tests live in a dedicated
  `surge-sdk/tests/shaping.rs` (public API only), not inline — matching Step 1's split. Two pure
  helpers (`select_vertical_metrics`, `anchor_offset`) plus `px_per_unit` are exposed `#[doc(hidden)]
  pub` so the integration test can drive both metric-selection branches and every anchor case
  deterministically with one font.
- **Verification run:** `cargo test -p surge-sdk` → 20 unit + 12 shaping + 1 round-trip tests pass
  (scaling, both metric-selection branches, real-font metrics, measured width / glyph count, empty
  string, anchor offsets, `draw_text`→`TextRun` placement, and `Font::load` disk+id wiring).
  `cargo test --workspace` green; `cargo clippy -p surge-sdk --tests` reports nothing new (the four
  warnings are all pre-existing); `cargo tree -d` still shows a single `read-fonts v0.47.0`.

### Step 4: `font_database` skeleton (`surge-core`, GPU but empty) ✅
New GLSL `font.vert`/`font.frag` + two build.rs entries. The pure **shelf atlas allocator** (with
tests), packing each cell with a ≥1px gutter (§3.2). `FontDatabase::new/drop`: R8 atlas image +
sampler (**LINEAR** filter, for GPU scaling) + pipeline + descriptor sets + instance SSBOs (mirroring
`sprite_database`), `load_font` (reads the file once into one byte buffer + one skrifa font, from which
skrifa draws outlines — the player side's one parsed font per file), and a `draw()` that renders zero
instances. Promote the shared helpers (`letterbox`, projection/view, subtexture transform) — currently
**private** in `sprite_database` — to a common location both DBs use. **Verification:** packer unit
tests (fills a shelf, wraps to the next, reports overflow, keeps the gutter between cells); a
GPU-struct-layout test pinning the font `InstanceRecord` size/align to the GLSL
(mirror sprite DB's `gpu_struct_layout_matches_glsl`).

**Resolution:** Done. New `surge-core/src/vulkan/font_database/` module (sibling of
`sprite_database`), registered in `vulkan/mod.rs`. `skrifa = "=0.50.0"` moved from a dev-dependency to
a real `surge-core` dependency; `harfrust` and `zeno` stay dev-dependencies for the Step 0 spike
(`zeno` moves to a real dep in Step 5). The four settled decisions below were confirmed with the
author before coding.

- **Shared helpers → new `render_common` module.** Per the author's choice, `make_ortho_projection`,
  `make_view`, `make_model_matrix`, `letterbox`, and `subtexture_uv_transform` moved from
  `sprite_database` (where `letterbox`/`subtexture_uv_transform` were private) into a new
  `surge-core/src/vulkan/render_common.rs`; both databases import them. The two matrix/projection
  helpers stay `pub`, `letterbox`/`subtexture_uv_transform` are now `pub(crate)`. Their unit tests
  (subtexture, letterbox, the `ndc_z` depth-range/ordering tests) moved with them to
  `render_common_tests.rs`. `window_to_design` (player cursor mapping, sprite-specific) and
  `full_texture_uv_transform` stay in `sprite_database`; `window_to_design` now imports `letterbox`.
- **`FontDatabase::new(context, ci: CreateInfo)`** reuses the sprite `CreateInfo` (author's choice,
  for symmetry with `SpriteDatabase::new`), reading only `window_width/height` for the projection and
  ignoring the sprite-only fields. Budgets are player-internal `const`s (§6.6): `ATLAS_SIZE = 1024`
  (one 1 MiB `R8_UNORM` page), `ATLAS_GUTTER = 1`, `MAX_GLYPH_INSTANCES = 4096`, `MAX_FONTS = 16`.
- **Atlas held as a raw image inline** (author's choice): `FontDatabase` owns the atlas as
  `vk::Image` + `vk_mem::Allocation` + `vk::ImageView`, created and transitioned
  `UNDEFINED → SHADER_READ_ONLY_OPTIMAL` once in `new()` via the immediate command buffer, destroyed
  in `drop()`. It is deliberately **not** a `Texture`, whose upload path discards the whole image;
  Step 5 needs the raw image handle for sub-region copies. With zero instances drawn, the atlas's
  undefined contents are never sampled.
- **Descriptors / pipeline mirror the sprite DB but simpler.** Single atlas, so set 1 is a plain
  `SAMPLER` + one `SAMPLED_IMAGE` (no bindless array, no `VARIABLE_DESCRIPTOR_COUNT`/`PARTIALLY_BOUND`).
  The font `InstanceRecord` drops the sprite's trailing `material` field (96 bytes vs 112). The
  `LINEAR` atlas sampler uses `CLAMP_TO_EDGE`. The pipeline is alpha-blended with the same
  `GREATER_OR_EQUAL` depth test as sprites, so the two share the depth buffer (text drawn second wins
  equal-z ties, §3.3). The `font.frag` samples single-channel coverage and writes
  `vec4(color.rgb, color.a * coverage)` (§3.2).
- **`load_font(path)`** reads the file once (`std::fs::read`), validates it parses with
  `skrifa::FontRef::new` (so a bad font fails at load, not mid-frame), and stores the one byte buffer
  per file (the `FontRef` borrows those bytes, so it is rebuilt on demand in Step 5 rather than stored
  self-referentially). Returns the `FontId` (upload order, matching the module side). New
  `VulkanError` variants: `FontFileIOError`, `FontParseError`, `FontDatabaseFull`.
- **Deviation (test placement):** per AGENTS.md's "separate test and module code" rule, the new unit
  tests live in dedicated files included as `#[cfg(test)] #[path = "…"] mod tests`
  (`render_common_tests.rs`, `font_database/atlas_allocator_tests.rs`, `font_database/tests.rs`)
  rather than inline `#[cfg(test)] mod tests` as `sprite_database` still does. The allocator carries a
  module-level `#![allow(dead_code)]` because `allocate` is only exercised by tests until Step 5 packs
  glyphs; `InstanceRecord` and `LoadedFont.bytes` carry targeted `#[allow(dead_code)]` for the same
  reason.
- **Verification run:** `cargo test -p surge-core` → 24 unit (5 new packer tests, the font
  `gpu_struct_layout_matches_glsl`, the moved `render_common` tests) + 2 spike tests pass;
  `cargo test --workspace` green; `cargo check -p surge-core` and `cargo clippy -p surge-core --tests`
  report nothing new from the added files; `cargo tree -d` still shows a single `read-fonts v0.47.0`
  (skrifa/harfrust/zeno/read-fonts/font-types unduplicated). `draw()` renders `occupancy = 0`
  instances; `add_text_run` and rasterization arrive in Step 5.

### Step 5: Rasterization + cache + atlas upload + draw (`surge-core`) ✅
skrifa→zeno glyph rasterization (through the §3.2 adapter) keyed by `(FontId, GlyphId, round(px))`;
`GlyphEntry` cache; incremental atlas upload via a **sub-region** `cmd_copy_buffer_to_image` with
`SHADER_READ_ONLY → TRANSFER_DST → SHADER_READ_ONLY` transitions on the persistent atlas image, and a
device drain on a miss frame to serialize the write against the `FRAMES_IN_FLIGHT` frames still
sampling the atlas. **This is a write-after-read hazard on one persistent image — a *different* hazard
from the sprite DB's drain, which exists only because its descriptor set is rewritten each flush and is
not `UPDATE_AFTER_BIND` (`sprite_database/mod.rs:663-673`); the atlas descriptor never changes, so do
not cite that rationale** (§3.1, §5). On atlas overflow, insert a **sentinel `GlyphEntry`** (zero size,
draws nothing) into the cache so the dropped glyph becomes a cache *hit* and is never re-rasterized or
re-drained. `add_text_run` builds instances with the y-up↔y-down conversion from §3.5; `draw()` emits
them under the letterboxed viewport. **Verification:** pure tests for glyph-cache keying/bucketing, for
the pen→quad placement math (baseline, top bearing, rounding), and that a second reference to a dropped
glyph is a cache hit that rasterizes nothing.

**Resolution:** Done. Two new GPU-free modules under `font_database/` keep all the fiddly logic out
of the Vulkan path and under test: `rasterizer.rs` (the skrifa-outline→zeno-coverage adapter,
`rasterize_glyph(bytes, glyph_id, px) -> Option<RasterizedGlyph>`, returning `None` for an unknown
id / undrawable / empty-outline glyph) and `glyph_cache.rs` (keying, the shelf-atlas packing, the
sentinel policy, and the pure placement/UV math). `zeno = "=0.3.3"` moved from a dev-dependency to a
real `surge-core` dependency; `harfrust` stays a dev-dependency (SDK-only in production). The tree
still resolves a single `read-fonts v0.47.0`.

- **Cache + sentinels.** `GlyphCache` wraps a `HashMap<GlyphKey, GlyphEntry>` over the
  `ShelfAllocator`; `GlyphKey = (font_id, glyph_id, px)` where `px = px_bucket(px_size)` =
  `px_size.round().max(1.0) as u32` (integer bucket, §6.3, clamped to ≥1 so a degenerate size never
  asks for a zero-pixel glyph). `get_or_insert(key, rasterize)` calls the rasterize closure **exactly
  once on a miss** and never on a hit; on an empty rasterization *or* atlas overflow it caches a
  zero-size **sentinel** `GlyphEntry` (so the glyph is a cache hit forever after, never re-rasterized
  or re-drained), and on a successful pack returns `(cell, coverage)` for the caller to upload. The
  borrow split in `add_text_run` (closure reads `self.fonts`, `get_or_insert` mutates
  `self.glyph_cache`) relies on disjoint-field borrows so the closure only runs on a miss.
- **Placement (§3.5).** `glyph_quad(pen_x, pen_y, entry)` snaps the pen to integer design-px
  (`pen.round()`), adds zeno's `left`/`top` bearings (top is the signed baseline→top-row offset,
  negative above the baseline, exactly as the Step 0 spike pinned), and sizes the quad to the atlas
  cell. The quad is placed with the promoted `make_model_matrix`.
- **Atlas-cell UVs — deviation from the "reuse `subtexture_uv_transform`" note.** The shared helper
  works in whole texels and cannot express the §3.2 half-texel inset, so the font path uses a small
  dedicated `glyph_cell_uv(cell, atlas_size)` that insets the sampled rect by half a texel on every
  side (offset `+0.5`, scale `−1.0` texel). This keeps the bilinear footprint of a GPU-scaled glyph
  **strictly inside its written cell**, which also means the `≥1px` allocator gutter never needs to be
  sampled. **Consequently the atlas is *not* cleared** (Step 4 only layout-transitions it; the gutter
  texels stay undefined but are provably never read). This diverges from the literal "transparent
  gutter" wording in §3.2/§5; see the open question below for the author.
- **Incremental upload.** `stage_glyph_upload` copies each miss's coverage into a host-visible
  `TRANSFER_SRC` staging buffer and records `(buffer, cell)` in `pending_glyph_uploads`.
  `flush_glyph_uploads` (first thing in `draw`) does one `SHADER_READ_ONLY→TRANSFER_DST` barrier, one
  `cmd_copy_buffer_to_image` per staged glyph into its sub-region (`image_offset`/`image_extent` =
  cell), one `TRANSFER_DST→SHADER_READ_ONLY` barrier, then `device_wait_idle()` **before** submitting
  the immediate command buffer, to serialize the write against in-flight frames still sampling the
  persistent atlas (the write-after-read hazard — deliberately *not* the sprite DB's
  descriptor-rewrite drain, §3.1). No-op on a frame with no misses. The `InstanceRecord` struct is now
  populated (its `#[allow(dead_code)]` and the allocator/`LoadedFont.bytes` dead-code allows were
  removed).
- **Error handling.** `add_text_run` is infallible (trait-shaped). An out-of-range `font_id` drops the
  whole run with a warning; a full per-frame instance buffer (`MAX_GLYPH_INSTANCES`) drops the rest of
  the run; a glyph that packs but whose staging buffer fails to allocate (OOM) is `mark_dropped`
  (sentinel) so its reserved-but-unwritten cell is never sampled.
- **Deviation (test placement):** per AGENTS.md, tests are in sibling files included via
  `#[cfg(test)] #[path = "…"] mod tests` — `glyph_cache_tests.rs` (8 tests: px bucketing, hit/miss and
  rasterize-once, same-bucket sharing, empty→sentinel, overflow→sentinel-hit, `mark_dropped`, the
  `glyph_quad` snap/bearing math, and the `glyph_cell_uv` half-texel inset) and `rasterizer_tests.rs`
  (3 tests over the bundled Noto Sans: a visible glyph's coverage + sane bearings, a space → `None`,
  an out-of-range id → `None`). All GPU-free.
- **Verification run:** `cargo test -p surge-core` → 35 unit (11 new) + 2 spike tests pass;
  `cargo test --workspace` green; `cargo clippy -p surge-core --tests` adds no new warning from the
  Step 5 files; `cargo tree -d` still shows a single `read-fonts v0.47.0`. The player wiring that
  actually calls `add_text_run`/`draw` with a real device (the composite `Canvas`) is Step 6, so the
  GPU upload/draw path is exercised end-to-end there, not yet here.

### Step 6: Player wiring (`surge-player`)
Introduce the `Canvas` composite (§3.3); build `FontDatabase` in `init_graphics` and the
swapchain-recreate branch; draw both DBs in `frame()`; handle `font_loads` + `text_runs` in
`remote_app::apply_ops`. **Verification:** extend the headless reload test if it asserts canvas
shape; a dev-mode round-trip test that a `TextRun` survives proxy→wire→`apply_ops` onto a counting
canvas.

### Step 7: Text-demo module + docs
Add a new module, **`surge-mod-text`**, whose only job is to render some text on screen and exercise
the full path end to end. It is kept as minimal as possible — a bare `SurgeApp` that draws nothing but
text, with dev/release `[[bin]]`s, its own `config.toml`, and an `assets/fonts/` folder holding
the Noto Sans TTF (resolved through `resolve_asset_path`, like any module asset). In `on_load` /
`on_swapchain_recreate` it registers the font (`Font::load`), and in `update` it shapes a short string
and draws it with a couple of anchors/sizes/colors so placement and the atlas cache are both
visible. The workspace picks it up automatically via the `surge-modules/*` glob. Update
`CLAUDE.md`/`AGENTS.md` build notes: the two new shaders in build.rs, and the git-lfs entry for the
`.ttf`. **Verification:** `cargo run -p surge-player` (configured for `surge-mod-text`) shows correctly-placed,
legible text — as crisp as the fractional letterbox scale allows (the R8 atlas is linearly sampled, so
glyphs are pixel-crisp only when design-px maps ~1:1 to device-px; scale-independent crispness via
SDF/hinting is deferred, §6.5); `cargo test --workspace` green.

## 5. Self-critique (harsh, per AGENTS.md)

- **The font bytes are held once per side (not "parsed twice").** The parse-once-per-side rule
  (§3.2) keeps each process to one lazy, zero-copy `read-fonts` parse: the SDK side feeds HarfRust and
  reads metrics from one HarfRust font with no skrifa; the player side holds one skrifa font for
  outlines. What remains is the *cross-process* copy — module (shaping) and player (raster) are
  different processes in dev, and a borrowed, bytes-referencing font cannot cross a socket — which is inherent to
  module-side shaping (decision #1), not to the crate choice. `read-fonts` being zero-copy makes this
  one byte buffer per font per process, negligible for the fonts in play. Accepted for v1; the escape
  hatch (ship outlines so the player never parses the font) is recorded in §6.
- **Measurement vs. rasterization can disagree by a sub-pixel.** The module measures with continuous
  px advances; the player rasterizes at `round(px)`. Glyph *positions* are unaffected (they come from
  shaping, not from the raster bucket), so only each glyph's *bitmap* bearing/size can differ by <1px.
  Accepted, documented.
- **A device drain per cache-miss frame is blunt.** Reusing the texture-upload stall is correct but
  stalls the GPU whenever a new glyph appears (warmup, or the first frame a new string is shown). For
  games with a mostly-static glyph set this is a handful of early stalls; for a ticker of ever-new
  glyphs it is bad. A barrier-only ring-buffer upload is the refinement, deferred.
- **Extending `SpriteCanvas` rather than adding a `TextCanvas` trait** means every impl (real DB,
  proxy, test doubles, composite) must grow two methods with no default. That is churn, but defaults
  that silently drop text would be a worse footgun. The name `SpriteCanvas` also becomes a slight
  misnomer; renaming it is out of scope and noisy, so it stays.
- **Gamma.** `color.a * coverage` with straight alpha blending in the existing framebuffer gives the
  usual slightly-thin AA text, consistent with how sprites already blend. Gamma-correct coverage is a
  deferred refinement, not a v1 goal.
- **The y-up↔y-down conversion is the one place a subtle bug hides.** It is concentrated in the SDK
  scaling (Step 3) and the player placement (Step 5), both pure and unit-tested, and de-risked by the
  Step 0 spike — but it is the first thing to check if text renders inverted or drifting.

## 6. Open decisions (for review before Step 0)

These are defaults the author chose but did not want to commit silently; say the word to change any.

1. **Test font location.** The font is **Noto Sans** (OFL), committed via git-lfs, settled. The demo
   module ships it under its own `assets/fonts/` (Step 7). The one undecided detail is where the
   *unit tests* in `surge-sdk` (Step 3) and `surge-core` (Step 0) read a font from, since those
   crates have no module assets folder. Proposed: `include_bytes!` a single committed copy of the
   Noto TTF into the tests, so they don't depend on CWD or a module's asset layout. Acceptable, or
   keep one shared on-disk fixture referenced by a workspace-relative path?
2. **Miss-frame stall vs. barrier-only upload.** v1 reuses the existing device-drain-on-upload
   (simple, correct, matches textures). Acceptable, or should the barrier-only ring buffer be in v1?
3. **px bucketing for the glyph cache.** Proposed: round to integer design-px. Alternative: 0.25-px
   buckets (crisper at the cost of more atlas entries), or exact f32 (risks cache bloat if a caller
   passes jittery sizes). Integer px is the default.
4. **v1 scope confirmation.** Single run, single font, single size, single line, LTR, point-anchored
   (left/center/right × top/baseline/bottom). No wrapping, multi-line, rich text, RTL/bidi, SDF,
   hinting, subpixel AA, or color/bitmap/emoji fonts (skrifa draws monochrome outlines only — a COLR/
   CBDT/sbix glyph would rasterize to an empty mask). Confirm this is the line for v1.
5. **Atlas size and overflow policy.** Default: one fixed 1024×1024 R8 atlas (1 MiB), cells packed with
   a ≥1px gutter (usable area slightly under the full 1 MiB); on overflow, log and draw nothing for the
   glyph, **caching a sentinel so it is not re-rasterized or re-drained every frame** (§Step 5; mirrors
   `max_textures`). Multi-page/eviction is deferred.
6. **Font budgets location.** `FontDatabase`'s budgets (atlas size, max fonts, max glyph instances
   per frame) are player-internal constants, like the sprite DB's hardcoded `CreateInfo` in
   `lib.rs:75`, not config or wire fields. Confirm they need not be module-configurable in v1.

**Escape hatch (not part of v1, recorded so it isn't rediscovered later).** The inherent cross-process
font copy (§3.2, §5) can be removed entirely by doing *all* font work on the module side, which already
holds the font for shaping, and shipping the player **glyph outlines** instead of glyph IDs: the
module extracts the outline with skrifa from the same font bytes (this is the one place HarfRust and
skrifa would coexist in a single crate, so *there* a single `read-fonts` in the tree becomes a real
requirement), the player rasterizes the path
with zeno only (no skrifa, no font file deployed player-side), and outlines being resolution-
independent are sent once per glyph and rastered at any px. The cost is heavier wire payload on
cache-miss, CPU rasterization moving into the module process, and the player losing a cross-module
glyph cache — and it partly inverts decision #2. Pursue only if font load time or memory ever shows up
in a profile. The current design keeps shaping module-side / rasterization player-side as decided.

> **Settled during review:** font is **Noto Sans**; the demo is a **new, minimal `surge-mod-text`
> module** that draws only text, and no existing module is touched. This plan refers to no module
> other than `surge-mod-text`. The font-parsing approach is **keep the current design** — parse once
> per side (one HarfRust font in the SDK, one skrifa font in the player), cross-process copy accepted;
> a single `read-fonts` in the tree is preferred for hygiene but is **not** required for correctness
> (§3.2, §Step 0). The escape hatch above is explicitly deferred.
