# SURGE — critical review

**Scope:** whole workspace at `b265e28` (branch `v1.4.0-rs`). 4,277 lines of Rust, 0 tests.

**Method (pass 2, converged):** every source file, manifest, shader, build script, and the stager read in
full. Pass 1's findings were then re-verified independently rather than assumed, and the following were
checked *empirically* against a real device (RTX 4090 Laptop, Mesa/NVIDIA, Wayland, Vulkan SDK 1.4.321):

- `cargo check --workspace` (clean, 0 warnings) and `cargo check -p surge-core --no-default-features` (fails).
- The player was **built and run** with validation layers active; the full log was inspected.
- The debug messenger was proven to work end-to-end by injecting a known VUID violation
  (`layerCount = 0`) and observing it reported. That injection was reverted; the tree is unmodified.
- `nalgebra`'s orthographic convention and the exact NDC depth of each `z` value were computed numerically.
- `size_of`/`align_of` of the GPU-facing structs were measured.
- The `png` crate's `EXPAND | ALPHA` output was measured against hand-built grayscale and RGB PNGs.
- Queue families, surface support, SPIR-V version/capabilities, PNG headers, `git ls-files`,
  `git check-ignore`, and idle CPU usage were all measured rather than inferred.

**Verdict up front:** the Vulkan layer is competent in outline and unsound in detail. There is one genuine
dangling-pointer UB, one CPU/GPU data race that defeats the very buffering it was written to provide, a
projection matrix that silently discards half its documented depth range, a texture path that issues a
GPU buffer over-read on a whole class of input, two GPU-facing structs with unspecified layout, and a
fresh clone of this repo cannot run at all. The 2048 module is roughly 15% of a game. The player's error
strategy is `.unwrap()` on every single engine call (29 of them), which discards the entire hand-written
`errors` module.

**The good news, stated fairly:** a full run of the app produces **zero** validation errors or warnings.
The per-image `render_finished_sem` indexing — the single most commonly botched piece of swapchain
synchronisation — is implemented correctly and commented correctly. The Rust/GLSL struct sizes really do
agree today. Most of what is below is latent, not currently firing.

---

## Corrections to the first pass

- **H7 was overstated.** `depthBoundsTestEnable = VK_TRUE` without the `depthBounds` feature is a genuine
  spec violation, but the claim that "with validation layers on by default, this fires on every run" is
  **false**. It was not reported in a full run. Neither was a deliberately injected
  `minDepthBounds = 5.0` on the same struct — while an injected `layerCount = 0` *was* reported
  immediately through the same messenger. So the layer works and simply does not flag this pipeline's
  depth-bounds state. H7 is demoted to **M22**.
- Everything else from pass 1 (C1–C5, H1–H6, H8–H10, M1–M13, and the LOW list) was re-checked and
  **holds**. Where a claim is now backed by a measurement rather than by reading, the evidence is quoted
  inline.

---

## CRITICAL

### C1. ~~Dangling raw pointer in the pipeline builder~~ — FIXED — `ctx_graphics_pipeline.rs`

**Fixed.** `set_color_attachment_format` no longer writes the raw pointer field at all — it now only
stores the owned `vk::Format` value and bumps `color_attachment_count`. The pointer is constructed exactly
once, in `create_graphics_pipeline`, right before the pipeline is created, via ash's safe
`.color_attachment_formats(&[fmt])` setter bound to `builder.color_attachment_format`:

```rust
let mut render_info = builder
    .render_info
    .color_attachment_formats(std::slice::from_ref(&builder.color_attachment_format));
```

`builder` is a by-value parameter that is never moved again after this point in the function, so the
pointer stays valid for the remainder of the call, through `push_next(&mut render_info)`. Because the
safe setter ties the slice to `PipelineRenderingCreateInfo<'a>`'s lifetime parameter, the borrow checker
now enforces this — the original bug (assigning the pointer inside the builder method, before the final
move into `create_graphics_pipeline`) is no longer expressible without a compile error. Verified with
`cargo check --workspace` (clean) and a real run with validation layers active: shader modules are
created, the pipeline is built, and shader modules are unloaded with zero validation errors reported.

Original finding kept below for the record.

```rust
pub fn set_color_attachment_format(mut self, format: vk::Format) -> Self {
    self.color_attachment_format = format;
    self.render_info.p_color_attachment_formats = &self.color_attachment_format; // ← into a local
    self                                                                          // ← then moved
}
```

The pointer is taken into the by-value `self`, which is then moved out of the function, moved again by
`pb = match ci.blending_mode {...}`, and moved a third time into `create_graphics_pipeline(pb)`. Every
move is a memcpy to a new address; the stored pointer keeps aiming at the original, now-dead stack slot.
`vkCreateGraphicsPipelines` then dereferences it.

This "works" only because the compiler often elides the moves and the dead stack slot happens to still
hold `B8G8R8A8_UNORM`. Under a different optimization level, an added builder method, or a reordered call
chain, the pipeline gets created with a garbage color attachment format.

Note the type is `vk::PipelineRenderingCreateInfo<'a>` — ash's lifetime *would* have caught this had you
used the safe `.color_attachment_formats(&[fmt])` setter instead of writing the raw field. The same shape
is used correctly elsewhere (`blending.p_attachments = &builder.blend_attachment`, where `builder`
outlives the call), which makes this one easy to miss on re-read.

### C2. ~~A fresh clone cannot run~~ — FIXED — `.gitignore:5` vs `shaders/`

**Fixed.** `build.rs` now compiles the SPIR-V into Cargo's `OUT_DIR`, and
`sprite_database/mod.rs` embeds it at compile time via
`include_bytes!(concat!(env!("OUT_DIR"), "/sprite_vert.spv"))`. The shaders ship inside the
binary — there is no `shaders/` directory, no symlinks, and no runtime file to be missing.
`extra/ln.ps1` and the stager's shader-copy step were removed as dead weight. Original
finding kept below for the record.

```
$ git ls-files shaders/          # → 0 files
$ ls shaders/                    # → 2 symlinks, both untracked
$ git check-ignore -v shaders/sprite_vert.spv
.gitignore:5:*.spv    shaders/sprite_vert.spv
```

`*.spv` matches the **symlink names** in `shaders/`, so the two symlinks the runtime depends on are
untracked. A `git clone` produces no `shaders/` directory at all, and
`load_shader_module("shaders/sprite_vert.spv")` fails on first run with a bare `Err` that `main.rs`
`.unwrap()`s into a panic. The recovery procedure (`ln -s` / `extra/ln.ps1`) exists nowhere in the repo's
own docs — only in `CLAUDE.md`, and only for Windows.

The root cause is the design decision to make the runtime shader path a repo-root-relative symlink into
`surge-core/src/`. Compiling into `OUT_DIR` and either embedding via `include_bytes!` or resolving
relative to the executable removes the whole class.

### C3. ~~CPU writes the instance SSBO before waiting on its fence~~ — FIXED — `main.rs:73-85` + `ctx_swpc.rs:10-14`

**Fixed.** `about_to_wait` in `main.rs` now calls `request_swpc_img()` (which waits on
`frame_fences[current_frame]`) *before* `app.update()`, instead of after. The CPU no longer
overwrites `instance_records_ssbos[current_frame]` until the fence has confirmed the GPU is done
reading it. `SurgeApp::update()`'s signature (`&mut self, dt: f32, spd: &mut SpriteDatabase`) never
needed the swapchain image index, so moving the acquire earlier required no other changes. Original
finding kept below for the record.

The per-frame SSBO duplication is documented with exactly the right intent:

```rust
/// We need to make sure that we don't write to a buffer
/// while it is being read by the GPU. For that reason,
/// we need as many buffers as there are frames in flight
instance_records_ssbos: [Buffer; FRAMES_IN_FLIGHT],
```

But the frame ordering defeats it. In `about_to_wait`:

1. `app.update(dt, spd)` → `add_instance()` writes `instance_records_ssbos[current_frame]`
   (`sprite_database/mod.rs:544-562`)
2. *then* `request_swpc_img()` → `wait_for_fences(frame_fences[current_frame])`

The fence that guarantees the GPU has finished reading buffer `[current_frame]` (from the submission two
frames ago) is waited on **after** the CPU has already overwritten it. This is a real host/device data
race on `HOST_ACCESS_SEQUENTIAL_WRITE` mapped memory, with no barrier or availability operation of any
kind. Symptom: intermittent one-frame sprite corruption/flicker that is worse the faster you run. Fix is
to hoist the fence wait above `app.update`.

### C4. ~~GPU-facing structs have unspecified field layout~~ — FIXED — `sprite_database/mod.rs:93-110`

**Fixed.** `FrameGlobals` now carries `#[repr(C)]`, and `InstanceRecord` was changed from `#[repr(align(16))]`
to `#[repr(C, align(16))]`. Field order (and therefore offsets) is now guaranteed by the language instead
of being an rustc-layout-algorithm coincidence. A new regression test,
`sprite_database::tests::gpu_struct_layout_matches_glsl`, pins `size_of::<FrameGlobals>() == 128` and
`size_of::<InstanceRecord>() == 112` / `align_of::<InstanceRecord>() == 16` against the std140/std430
layout the shader expects (`sprite.vert:4-13`), so a future reordered or added field that desyncs Rust
from GLSL now fails `cargo test` instead of failing silently. Verified with `cargo test -p surge-core` and
a real run with validation layers active — no UBO/SSBO layout validation errors, same as before the change.
Original finding kept below for the record.

```rust
struct FrameGlobals { _view: Matrix4<f32>, _proj: Matrix4<f32> }          // no repr at all

#[repr(align(16))]                                                        // align, but not C
struct InstanceRecord { _model: …, _color: …, _subtexture_data: …, _material_id: u32 }
```

Both are written byte-for-byte into memory the GPU reads under a fixed std140/std430 layout
(`sprite.vert:4-14`). Without `#[repr(C)]`, rustc is free to reorder fields; `-Z randomize-layout`, a
future rustc, or adding one field of different alignment silently desynchronizes Rust from GLSL.

Measured: `InstanceRecord` is `size=112 align=16` against a std430 array stride of 112, and `FrameGlobals`
is `size=128 align=4` against a std140 block size of 128. They agree **today**, which is precisely why
this will not be noticed until it breaks.

`PushConstants` two structs below *does* carry `#[repr(C)]`, so the omission is inconsistent, not a
deliberate policy. The `_`-prefixed field names are the tell: the compiler considers every one of them
dead because nothing in Rust ever reads them.

### C5. ~~`texture_id` is never bounds-checked~~ — FIXED — `sprite_database/mod.rs:502-565`

**Fixed.** `add_instance` now checks `aii.texture_id >= self.uploaded_textures.len()` up front and,
on failure, logs a warning and drops the request — the same "ignore and warn" pattern already used a
few lines down for a full database and for a subtexture that doesn't fit. Since `upload_texture` never
lets `uploaded_textures.len()` exceed `ci.max_sprites`, this one check covers both failure modes: an
index into unwritten `PARTIALLY_BOUND` descriptor space, and an out-of-bounds descriptor access past
`max_sprites`. It also fixes the panic on the subtexture path, which indexed `uploaded_textures`
directly before this check existed. `add_instance` keeps its `()` return type — no caller changes
required. Original finding kept below for the record.

`add_instance` writes `aii.texture_id as u32` straight into `_material_id`, which the fragment shader
uses as `textures[nonuniformEXT(material)]` into a `max_sprites`-sized runtime array
(`sprite.frag:5,16`). Two distinct failures:

- `texture_id >= uploaded_textures.len()` → reads a `PARTIALLY_BOUND` descriptor that was never written.
  Undefined contents; on some drivers, a hang.
- `texture_id >= max_sprites` → out-of-bounds descriptor access. Device lost.

Neither is checked. Meanwhile the *subtexture* path at line 513 indexes
`self.uploaded_textures[aii.texture_id]` and **panics** on the same bad input. So the same mistake either
aborts the process or corrupts the GPU depending on whether the caller passed `Some(subtexture_info)`. A
`texture_id` that indexes "upload order" with no handle type, no validation, and two different failure
modes is the weakest part of the public API.

### C6. ~~The projection is OpenGL-convention: half the documented depth range is silently clipped~~ — FIXED — `sprite_database/mod.rs:757`

**Fixed.** `make_ortho_projection` now builds the projection matrix directly for Vulkan's `[0, 1]`
NDC/depth convention instead of using `nalgebra::Matrix4::new_orthographic` (which emits the GL
`[-1, 1]` convention). Composed with `make_view`'s camera, the full documented `z` range in `[0, 1]`
now maps onto valid Vulkan depth `[0, 1]` instead of only the `[0, 0.5]` half. Two unit tests
(`sprite_database/mod.rs` `tests` module) run the actual `proj * view * model` chain end-to-end and
assert the full `z` range lands in bounds and preserves front-to-back ordering. Original finding kept
below for the record.

```rust
nalgebra::Matrix4::new_orthographic(0.0, width, 0.0, height, 0.0, 1.0)
```

`nalgebra`'s `new_orthographic` emits the **GL** convention, mapping z to NDC `[-1, 1]`. Vulkan clips to
`[0, 1]`. Measured end-to-end through the actual `proj * view * model` chain the engine builds:

| `z` passed to `add_instance` | resulting NDC z | outcome |
|---|---|---|
| 0.00 | +1.000 | drawn, frontmost |
| 0.25 | +0.500 | drawn |
| 0.50 |  0.000 | drawn, backmost — exactly on the clip plane |
| 0.75 | −0.500 | **clipped, never rendered** |
| 1.00 | −1.000 | **clipped, never rendered** |

`make_ortho_projection`'s own doc comment, and `CLAUDE.md`, both state "z in `[0, 1]`". The real usable
range is `[0, 0.5]`, and anything above 0.5 vanishes with no warning, no validation error, and no
plausible symptom to debug from.

Worse, the reason `[0, 0.5]` works at all is an accident. `make_view` places the camera at `z = 1` looking
at `z = 0`, contributing a `−1` translation in z. Without that unrelated choice, `z = 0` would itself map
to NDC `−1` and *nothing* would render. Two independent decisions happen to cancel into a half-usable
depth range.

Both shipped modules park their background at exactly `z = 0.5` (`board_geometry.rs:19`,
`surge-mod-default/src/lib.rs:39`) — i.e. exactly on the clip plane, one nudge from disappearing. The
depth clear is `0.0` with `GREATER_OR_EQUAL`, so `z = 0.5` passes only by the `EQUAL` half of the
comparison.

Fix: build the ortho matrix for Vulkan's `[0, 1]` depth directly (or post-multiply by the standard
`z' = (z + w) / 2` correction) and drop the compensating camera offset. This changes the meaning of every
`z` any module passes, so it wants doing before more modules exist.

---

## HIGH

### H1. `ERROR_OUT_OF_DATE_KHR` is a panic, not a resize

`acquire_next_image` (`ctx_swpc.rs:26`) and `queue_present` (`ctx_swpc.rs:62`) map every `VkResult`
failure into a `VulkanError`, which `main.rs:85` / `main.rs:136` `.unwrap()`. `ERROR_OUT_OF_DATE_KHR` is
not an error — it is the normal, expected signal to recreate the swapchain, and it is guaranteed to arrive
on monitor changes, DPI changes, compositor restarts, and fullscreen transitions. Every one of those is a
hard crash today. The `recreate_swapchain` machinery exists and is simply never reachable via the path
that most commonly needs it.

### H2. Suboptimal-on-present is silently discarded — `main.rs:87` vs `main.rs:135`

```rust
self.recreate_swapchain = swpc_img_data.suboptimal;   // line 87: read from *acquire*
…
.present_swpc(&mut swpc_img_data)                     // line 135: writes suboptimal, then discarded
```

`present_swpc` takes `&mut SwapchainImageData` specifically so it can report suboptimal
(`ctx_swpc.rs:58`), but by then line 87 has already run and `swpc_img_data` is dropped at end of scope.
The `&mut` parameter is pure ceremony. Separately, line 87 uses `=` rather than `|=`, so it also *clears*
a recreation request rather than accumulating one — safe only by accident of winit's current
event/`about_to_wait` interleaving.

### H3. `windowed = false` is structurally broken

Four consumers, three different notions of extent, and only one of them tracks reality:

| Consumer | Source |
|---|---|
| swapchain | surface capabilities, clamped (`image.rs:24-33`) |
| depth image | `config.resolution` (`image.rs:121-125`) |
| viewport/scissor | `ci.window_width/height` = `config.resolution` (`sprite_database/mod.rs:688-703`) |
| render area | swapchain extent (`ctx_command.rs:149-152`) |

In borderless fullscreen the surface is the monitor size while `config.resolution` stays at whatever the
TOML says (500×800 for 2048). You then render into a render area larger than the depth attachment — a VUID
violation — through a viewport sized to the config. Every shipped `config.toml` sets `windowed = true`, so
this has probably never been exercised. See also **H13**, which is the same mismatch reached by a path
that *does* fire, every single launch.

### H4. The presentation queue is never validated; formats and present modes are never enumerated

`grep` across the whole crate finds exactly one surface query —
`get_physical_device_surface_capabilities` (`image.rs:20`). There is no:

- `get_physical_device_surface_support` — you present on the graphics queue having never confirmed it can
  present to this surface (VUID-vkQueuePresentKHR-pSwapchains-01292).
- `get_physical_device_surface_formats` — `B8G8R8A8_UNORM` + `SRGB_NONLINEAR` are hardcoded at
  `image.rs:35-36`.
- `get_physical_device_surface_present_modes` — `IMMEDIATE` is used unconditionally when `vsync = false`
  (`image.rs:46-50`); only `FIFO` is guaranteed by spec.

Each is a swapchain-creation failure or a validation error on hardware that differs from your dev machine.
Note this is not merely an oversight of ordering: `create_logical_device` runs at `ctx_new_drop.rs:74`,
*before* the surface is created at line 78, so the physical-device selection is structurally incapable of
considering presentation support.

### H5. `vkGetDeviceQueue` on a family with no created queues — `device.rs:203-217, 258-260`

```rust
if indices.transfer != indices.graphics && indices.transfer != indices.compute { push transfer }
if indices.compute  != indices.graphics && indices.compute  != indices.transfer { push compute }
…
let compute_queue  = device.get_device_queue(indices.compute, 0);
let transfer_queue = device.get_device_queue(indices.transfer, 0);
```

When `compute == transfer != graphics` — a common layout on AMD — **both** guards evaluate false, so
neither family gets a `DeviceQueueCreateInfo`, yet both are fetched. That is
VUID-vkGetDeviceQueue-queueFamilyIndex-00384. The handles are currently bound to
`_compute_queue`/`_transfer_queue` and thrown away, so it's a validation error rather than a crash — until
someone uses them.

Confirmed not to trigger on the dev machine (its families do not collide that way), which is consistent
with the clean validation run and is exactly why it will be found by someone else.

### H6. `get_queue_family_indices` picks the *last* match — `device.rs:120-130`

The loop has no `break`, so each index ends up as the highest-numbered family carrying the bit, not the
first or the most suitable. Measured on the dev machine's own GPU:

```
family 0: GRAPHICS | COMPUTE | TRANSFER | SPARSE   (16 queues)
family 1: TRANSFER | SPARSE                        (2)
family 2: COMPUTE  | TRANSFER | SPARSE             (8)
family 3: TRANSFER | SPARSE | VIDEO_DECODE         (1)
family 4: TRANSFER | SPARSE | VIDEO_ENCODE         (2)
family 5: TRANSFER | SPARSE | OPTICAL_FLOW_NV      (1)
→ selected: graphics = 0, compute = 2, transfer = 5
```

The "transfer queue" this engine selects on its own development hardware is the single-queue **optical-flow**
family. Harmless only because both handles are discarded. There is also no "not found" signal — the struct
defaults to `0`, so a device with no matching family silently yields family 0 instead of an error.

### H8. The staged 2048 build cannot start — `surge-mod-2048/src/lib.rs:27-40` vs `stager/stager.py:151-166`

The module hardcodes `surge-modules/surge-mod-2048/assets/board.png`. The stager copies that tree to
`staging-surge-mod-2048/assets/`. Nothing rewrites the path. `upload_texture(...).unwrap()` therefore
panics on the first frame of every staged release build. The stager's entire purpose is producing a
"self-contained, CWD-correct release layout" and it produces one that does not run.

The two modules are mutually inconsistent about this: `surge-mod-default` uses `assets/awesomeface.png`
(staged-correct, **dev-broken** — verified, there is no `assets/` at repo root), 2048 uses repo-root paths
(dev-correct, staged-broken). Neither works in both layouts, and there is no asset-path API in
`surge-core` to resolve it.

### H9. ~~`--no-default-features` does not compile~~ — FIXED

**Fixed.** `ctx_new_drop.rs:44` now calls `instance::build_instance(...)` (was unqualified
`build_instance(...)`), and `instance.rs:137` now takes `entry: &ash::Entry` (was bare `&Entry`).
`cargo check -p surge-core --no-default-features` compiles clean. Original finding kept below for
the record.

```
$ cargo check -p surge-core --no-default-features
surge-core/src/vulkan/ctx_new_drop.rs:44: error[E0425]: cannot find function `build_instance` in this scope
surge-core/src/vulkan/instance.rs:137:   error[E0425]: cannot find type `Entry` in this scope
```

Line 44 calls `build_instance(...)` unqualified where the `cfg(validation_layers)` branch correctly writes
`instance::build_instance(...)`; line 137 takes `&Entry` where the sibling function takes `&ash::Entry`.
The non-validation build has evidently never been compiled. Since `validation_layers` is on by default,
**there is currently no way to build a release binary without validation layers** — you ship the debug
messenger to end users.

### H10. Two statically-linked copies of `surge-core` and `std` in one process

```
$ ldd target/debug/libsurge_mod_2048.so
    libstdc++ libgcc_s libm libc     # no Rust libraries at all
```

`surge-core` is an rlib, so it is statically linked into both `surge-player` and each module `.so` (each
of which is ~110–140 MB in debug, which is the same fact seen from a different angle). `&mut
SpriteDatabase`, `Arc<RefCell<VulkanContext>>`, and `Box<dyn SurgeApp>` all cross that boundary between
two independently-compiled copies of the same types, over an unstable ABI, with no version or ABI check on
the `surge_register_app` symbol — which is `extern "Rust"`, not `extern "C"`.

This works only while both sides are built by the identical rustc with identical flags and features.
Mixing a debug player with a release module, or bumping rustc without rebuilding both, produces silent
memory corruption rather than a load error. The duplicated-`log`-state workaround (each module calling
`init_env_logger` itself) is the visible symptom of this; the invisible ones are worse. Since hot
reloading — the engine's stated purpose — will make this boundary much hotter, an `extern "C"`
vtable-of-function-pointers ABI with an explicit version field is worth doing before, not after.

### H11. ~~Grayscale PNGs cause a GPU buffer over-read~~ — FIXED — `sprite_database/mod.rs:579-632`

**Fixed.** `upload_texture` now checks `reader.output_color_type()` right after `read_info()`, before the
staging buffer is even sized or allocated, and rejects the upload with the new
`VulkanError::UnsupportedTextureColorType` if the decoder's actual output isn't `ColorType::Rgba` — closing
the gap the `EXPAND | ALPHA` comment claimed was already closed.

```rust
// Ensure output is always RGBA regardless of source format
decoder.set_transformations(png::Transformations::EXPAND | png::Transformations::ALPHA);
```

The comment is wrong. `EXPAND` expands *palette* and sub-8-bit images; it does **not** expand grayscale to
RGB. `ALPHA` then adds one channel, yielding **GrayscaleAlpha — 2 channels, not 4**. Measured against the
png 0.18 decoder the engine actually uses:

```
gray8.png : staging buffer=32B  color=GrayscaleAlpha  → engine copies 64B  ==> OVERRUN by 32B
rgb8.png  : staging buffer=64B  color=Rgba            → engine copies 64B  ==> ok
```

The format match at line 618 only inspects `bit_depth`, never `color_type`, so a grayscale PNG is uploaded
as `R8G8B8A8_UNORM` and `immediate_upload_from_buffer` issues a `vkCmdCopyBufferToImage` for
`width × height × 4` bytes out of a `width × height × 2` staging buffer — a 2× over-read past the end of a
VMA allocation (VUID-vkCmdCopyBufferToImage-pRegions-00171). 16-bit grayscale over-reads identically.

`assert!(read_info.buffer_size() == image_size)` at line 615 does not catch it: both sides are the
*decoder's* size, not the size Vulkan is about to copy. Every shipped asset is RGBA (colour type 6) or
palette (colour type 3), so this is latent — but it is a data-dependent memory-safety bug in a `pub fn`
that accepts arbitrary files, guarded by a comment claiming it is handled.

Fix: match on `read_info.color_type` as well, and reject or convert anything that is not `Rgba`.

### H12. ~~Host-visible memory is written but never flushed~~ — FIXED — `buffer.rs`, `sprite_database/mod.rs`

**Fixed.** Added `Buffer::flush()`, wrapping `vmaFlushAllocation` over the whole allocation (a no-op on
coherent memory, essential otherwise) and returning the new `VulkanError::BufferFlushError` on failure. It
is called after each of the three mapped writes: once after the one-time `FrameGlobals` UBO write in
`SpriteDatabase::new`, once per texture in `upload_texture` right before the staging buffer is read by
`immediate_upload_from_buffer`'s copy command, and once per frame at the top of `draw()` for the current
frame's instance SSBO — a single flush covering every `add_instance` write made that frame, rather than
one per call. `draw()` now returns `Result<(), VulkanError>` to propagate that; its sole caller in
`main.rs` unwraps it, matching how every other fallible per-frame Vulkan call in that loop is handled.

```
$ grep -rn "flush_allocation\|invalidate_allocation" --include=*.rs .
(no matches)
```

Every write to the frame-globals UBO and the instance SSBOs goes through
`allocation_info.mapped_data` and is never followed by `vmaFlushAllocation`. VMA's
`HOST_ACCESS_SEQUENTIAL_WRITE` + `MemoryUsage::Auto` *prefers* `HOST_COHERENT` memory but does not
guarantee it; the whole point of `vmaFlushAllocation` is that it is a no-op on coherent memory and
essential on non-coherent memory. On a heap without `HOST_COHERENT` — entirely possible on other vendors,
integrated parts, and under memory pressure when VMA falls back to a different type — every sprite the
engine submits is invisible to the GPU. Nothing in the code checks which memory type it actually got.

### H13. `Resized` destroys `config.resolution` — and it fires on every single launch — `main.rs:240-259`

`WindowEvent::Resized` writes the observed size back into `self.engine_config.resolution`, which is the
authority for the depth image, the viewport, the scissor, and the ortho projection. The configured
resolution is not a setting; it is a variable the window manager overwrites.

This is not theoretical. A stock launch on the dev machine, straight from the run log:

```
[surge_player] Resizing window (500,800) -> (501,800)
[surge_core::vulkan::ctx_swpc] Recreating swapchain
[surge_core::vulkan::sprite_database] Creating sprite database
… 12 textures re-uploaded …
[surge_core::vulkan::sprite_database] Destroying sprite database
```

Every launch, before the first useful frame, the compositor hands back a size one pixel off, and the
engine responds by tearing down and rebuilding the swapchain, the depth image, the descriptor pool, the
pipeline, and all twelve textures. `.with_resizable(false)` does not prevent it.

One pixel is harmless. Fractional scaling or a HiDPI output is not: `BoardGeometry` hardcodes 500×800-based
pixel constants (`slot_dims = 121.0`, `x_slot_base = 15.0`, `y_slot_base = 315.0`, `piece_dims = 105.0`)
while `add_board_sprite` scales the background to `get_create_info().window_*` — which now follows the
*actual* surface. The board stretches and the pieces do not, so the tiles drift out of their slots. There
is no API for a module to learn that the resolution changed (see **M10**: `on_swapchain_recreate` takes
`&self`).

### H14. ~~`destroy_swapchain()`'s idempotency guard does not work~~ — FIXED — `ctx_new_drop.rs:174-192`

**Fixed.** `destroy_swapchain()` now clears `swapchain_data.image_views` and sets
`swapchain_data.swapchain` back to `vk::SwapchainKHR::null()` after destroying them, so the null check at
the top of the function is a real guard and a second call is a no-op instead of a double-free.

```rust
if self.swapchain_data.swapchain != vk::SwapchainKHR::null() {
    … destroy image views … destroy swapchain …
}   // ← handle is never set to null
```

The null check exists to make the call safe to repeat — that is its only purpose, since `Drop`
deliberately skips swapchain destruction and delegates to this method. But neither
`swapchain_data.swapchain` nor `swapchain_data.image_views` is cleared, so after `exiting` runs, the
context holds a full set of destroyed-but-non-null handles and a second call double-frees every one of
them. The invariant "call this exactly once, and only from `exiting`" is enforced entirely by a comment.

`recreate_swapchain` escapes this only because it immediately overwrites the whole `SwapchainData`
struct. Setting the handle to null and clearing the view vector is a two-line fix that makes the guard do
what it says.

---

## MEDIUM

**M1. ~57% of a core burned while paused or unfocused** — `main.rs:37-40`. `ControlFlow::Poll` plus an
early `return` from `about_to_wait` skips the FPS-cap sleep at the bottom of the same function. Measured:
`57.2% CPU` for a window that is rendering nothing. Combine with **M3** and the observed steady state of a
freshly launched player is "frozen, and hot".

**M2. Delta-time spike on unpause** — `main.rs:76`. `last_update_call_timer` is only reset after a
completed frame. Pause for 30 s, and the first frame back delivers `dt = 30.0` to `app.update`. Any module
doing `pos += v * dt` teleports.

**M3. Rendering stops on focus loss and may never resume** — `main.rs:249-271`. `Focused(false)` sets
`pause_rendering = true`, and only `Focused(true)` clears it — a non-zero `Resized` sets
`recreate_swapchain` but leaves the pause flag alone. Observed directly: launching the player from a
terminal leaves it in `Pausing` for the whole run, rendering nothing, because it never took focus. A
restore-from-minimize that does not deliver focus leaves the window permanently frozen.

**M4. `fps_cap = 0` panics** — `main.rs:141-148`. `1.0/0.0` → `inf` → `Duration::from_secs_f64(inf)`
panics. No config validation anywhere; `resolution.width/height = 0` is equally unguarded. Unreachable in
the shipped configs only because they all set `vsync = true`.

**M5. 2048's two starting pieces can occupy one slot** — `app_2048.rs:38-43`. `while piece_a == piece_b`
compares the *whole* `Piece` (a derived `PartialEq` over `usize` slots **and** `f32` positions **and**
`value`), so two pieces on the same slot with different values pass the check: P(same slot) × P(different
value) = 1/16 × 1/2 ≈ 1/32 of new games. `reconstruct_board` then does `board[y][x] = Some(idx)`,
overwriting — one piece vanishes from the move logic permanently, renders forever at its start slot, and
never responds to input. Compare slots, not pieces.

**M6. `update_pos` ignores `dt`** — `piece.rs:51-62`. `c_pos += 0.1 * r` is per-*frame*, and the engine's
computed delta never reaches it (`update(&mut self, _: f32, ...)` in `lib.rs:76` discards it before
`update_live_pieces_pos` is even called). Animation speed is a direct function of framerate.

**M7. `Cargo.lock` is gitignored** — `.gitignore:1`. This workspace ships a binary; the lock file belongs
in version control. Right now no two clones are guaranteed to build the same dependency graph, and
`vk-mem`/`ash` are exactly the crates where that matters.

**M8. ~~`max_sprites` conflates two unrelated budgets~~ — FIXED — `sprite_database/mod.rs`, `main.rs`.
`CreateInfo` now has an independent `max_textures: u32` field. The descriptor pool/set sizing for
`SAMPLED_IMAGE` and the `upload_texture` capacity check use `max_textures`; the instance SSBO sizing and
`add_instance` occupancy check keep using `max_sprites`. Both call sites in `main.rs` pass `max_sprites: 32,
max_textures: 32`, preserving current numeric behavior while decoupling the mechanism for future tuning.

**M9. `upload_texture` updates a live descriptor set** — `sprite_database/mod.rs:637-663`.
`update_descriptor_sets` on `mat_desc_set` with no `UPDATE_AFTER_BIND` flag and no fence wait. Safe only
because both call sites happen to run before any submission that binds the set. Nothing in the API says
so, and it is a `pub fn`.

**M10. `on_swapchain_recreate(&self, ...)` takes `&self`** — `app/mod.rs:37`. The player destroys and
rebuilds the entire `SpriteDatabase` on recreate, which reassigns every `texture_id` in upload order — yet
the module is handed an immutable receiver and cannot record the new mapping, nor the new resolution
(**H13**). `mouse_wheel_event(&self)` and `on_unload(&self)` are likewise `&self` while their siblings are
`&mut self`. Pick one.

**M11. The `errors` module is dead weight at the top level** — `main.rs` contains **29** `.unwrap()`
calls. Forty-plus carefully written `thiserror` variants, and every call site in the player discards them.
A missing `config.toml`, a missing dylib, a missing shader, and a lost device are all indistinguishable
panics-with-backtrace. Every one of those has a good message already written for it.

**M12. Each texture upload fully stalls the GPU** — `ctx_command.rs:332-361`. `cmd_immediate_submit`
creates a fence, submits, blocks up to 1 s, destroys the fence — per texture. 2048's startup does this 12
times serially.

**M13. `build.rs` writes into the source tree and hides its own errors** — `surge-core/build.rs`. Output
goes to `src/vulkan/sprite_database/shaders/*.spv` rather than `OUT_DIR` (breaks read-only checkouts and
cross-target builds, and survives `cargo clean`, so a stale `.spv` can be staged); there are no
`cargo:rerun-if-changed` directives; diagnostics use `println!` instead of `cargo::warning=`, so on failure
the user sees a bare `exit(1)` with the glslang output swallowed unless they pass `-vv`.
`o.status.code().unwrap()` also panics if glslang is killed by a signal. Nothing passes
`--target-env vulkan1.3`: the emitted modules are **SPIR-V 1.0** carrying
`OpCapability PhysicalStorageBufferAddresses` + `OpExtension "SPV_KHR_physical_storage_buffer"` against an
app that declares `API_VERSION_1_3`. That is legal and works, but it is an accident of glslang's default
target env rather than a decision.

**M14. No colour-space management anywhere.** The swapchain is `B8G8R8A8_UNORM` (linear) presented as
`SRGB_NONLINEAR` (`image.rs:35-36`), textures are uploaded as `R8G8B8A8_UNORM` (`mod.rs:618-620`), and
`sprite.frag` does a plain multiply. sRGB-encoded PNG texels are therefore sampled as if they were linear,
alpha-blended in non-linear space, and handed to a display that decodes them as sRGB. The net effect is
that images pass through roughly correct while **blending is wrong** — transparent edges composite too
dark, which is exactly the artefact the 2048 tiles will show. Either use `_SRGB` formats for both the
swapchain and the textures, or convert explicitly. Right now the choice has not been made, it has been
defaulted into.

**M15. The 16-bit PNG path uploads byte-swapped data** — `sprite_database/mod.rs:618-621`. The `png` crate
emits 16-bit samples in **big-endian** order; `R16G16B16A16_UNORM` is interpreted in host order. On every
little-endian target each channel is byte-reversed. No shipped asset is 16-bit, so the branch has never
run.

**M16. `.gitignore`'s unanchored `config.toml` also ignores the module configs the stager requires.**

```
$ git check-ignore --no-index -v surge-modules/surge-mod-default/config.toml
.gitignore:4:config.toml    surge-modules/surge-mod-default/config.toml
```

The two existing module configs are tracked only because they predate the rule. Add a new module and its
`config.toml` is silently unstageable — `git add` refuses it without `-f` — while
`stager.py` hard-exits if the file is missing. Anchor the pattern to `/config.toml`.

**M17. Every resize re-reads twelve PNGs from disk inside the frame loop** — `main.rs:65-66` →
`surge-mod-2048/src/lib.rs:72-74`. `on_swapchain_recreate` calls `load_image_assets`, which opens,
decodes, and uploads all twelve files with a blocking fence per upload (**M12**), synchronously in
`about_to_wait`. That is 12 file reads plus 12 full GPU stalls per recreate, and by **H13** at least one
recreate happens on every launch. There is no texture cache and no way for a module to keep GPU resources
across a rebuild.

**M18. `load_from_dylib` has no branch outside Windows and Linux** — `app/mod.rs:56-65`. The
`#[cfg(target_os = ...)]` block yields `()` on macOS/BSD, producing a confusing type error instead of a
`compile_error!` naming the unsupported platform.

**M19. `shader_draw_parameters` is requested but never checked — and never used** — `device.rs:235` vs
`device.rs:17-45`. `device_has_required_features` validates nine features; this tenth one is enabled at
`vkCreateDevice` without ever being in the suitability check, so a device that passes suitability can
still fail device creation. Meanwhile the shaders only use `gl_InstanceIndex`/`gl_VertexIndex`, which need
no such feature. Drop it or check it.

**M20. `present_completed_sem` is over-allocated, and the comment explaining why is wrong** —
`ctx_new_drop.rs:128-134`, `ctx_swpc.rs:23`, `ctx_command.rs:277`. A five-line comment explains that
semaphores are sized per swapchain image "because the render_finished semaphore … is held by the
presentation engine". That is true of `render_finished_sem` and false of `present_completed_sem`, which is
indexed by `current_frame` (0..`FRAMES_IN_FLIGHT`). Both arrays get `swapchain_image_count` entries, so
the acquire semaphores beyond index 1 are created, never used, and destroyed. Harmless; actively
misleading to the next reader, on the one topic in this file where the code is otherwise right.

**M21. `Buffer` snapshots its `AllocationInfo` at construction** — `buffer.rs:28-38`. `mapped_data` is
cached once and handed out by `get_allocation_info()` forever. Any VMA remap or defragmentation
invalidates it silently, and there is no null check before `add_instance` dereferences it as `*mut
InstanceRecord`. Query it at use, or at least assert non-null at construction.

**M22 (was H7 — corrected). `depth_bounds_test_enable = VK_TRUE` without the `depthBounds` feature** —
`ctx_graphics_pipeline.rs:147`. `set_depth_test_enabled` unconditionally enables the depth bounds test;
`create_logical_device` never requests `depthBounds` and `device_has_required_features` never checks it.
VUID-VkPipelineDepthStencilStateCreateInfo-depthBoundsTestEnable-00598 is unambiguous, so this is out of
spec.

It is nevertheless **not** reported by the Khronos validation layer 1.4.321: neither this, nor a
deliberately injected `minDepthBounds = 5.0` on the same struct, produced any message, while an injected
`layerCount = 0` was reported instantly through the same messenger. So the practical impact is nil today —
the bounds are `0.0..1.0`, making the test a functional no-op — and the cost of correctness is deleting one
line. Demoted from HIGH accordingly, and flagged as a reminder that "validation is clean" does not mean
"spec-conformant".

---

## LOW / hygiene

- **Subtexture bounds off-by-one** — `sprite_database/mod.rs:522-523`: `(sx + sw) < ow` rejects a sub-rect
  ending exactly at the texture edge; should be `<=`. Not triggered by the shipped 12502×502 sheet (frame
  24 ends at 12500 — verified), but it's wrong. `sx + sw` also overflows `u32` and panics in debug.
- **`create_swapchain` ignores half of what it queries** — `image.rs:18-64`: `current_extent` (including
  the `0xFFFFFFFF` "surface size is up to you" sentinel) is never consulted, and
  `supported_usage_flags` is never checked against the unconditionally-requested `TRANSFER_DST`.
- **`Buffer::get_allocation()` hands out a `Copy` handle** — `buffer.rs:45`. `vk_mem::Allocation` derives
  `Copy`; a caller can trivially arrange a double-free. Currently unused — delete it.
- **`SpriteDatabase::new` panics inside `std::array::from_fn`** — `mod.rs:208`: `Buffer::new(...).unwrap()`
  turns an allocation failure into a panic, in a function whose whole signature is `Result<Self,
  VulkanError>`. Same shape at `mod.rs:585` (`output_buffer_size().unwrap()` on attacker-controlled input).
- **`Drop for VulkanContext` leaks everything if `device_wait_idle` fails** — `ctx_new_drop.rs:210-213`
  returns early, skipping the allocator, device, surface, messenger, and instance.
- **Config drift** — `msaa = true` in every shipped config while `set_multisampling_none()` is hardcoded;
  `allow_resizes` is an unknown key silently swallowed by serde (no `deny_unknown_fields`) against a window
  hardcoded `.with_resizable(false)`.
- **Dead code** — `uuid` is a `surge-core` dependency with **zero** references (verified); `ModuleError`
  and `VulkanError::ImageCreationError` are never constructed; `Texture::new` reports image-creation
  failure as `DepthImageCreationError` (`texture.rs:52`) and image-view failure as
  `SwapchainCreationError` (`texture.rs:77`); `create_depth_image` does the same at `image.rs:170`;
  `set_depth_test_disabled` is unused; `surge-player`'s `path` dependency on `surge-mod-default` is
  vestigial.
- **`BlendingMode::default()` / `SubTextureInfo::default()`** are inherent methods shadowing the `Default`
  trait without implementing it — they will not work in `..Default::default()` position.
- **`load_shader_module` never checks the SPIR-V magic number** (`ctx_shader.rs`), and the
  `chunks_exact`/`from_ne_bytes` round-trip is a byte-identical copy whose only real purpose — obtaining
  4-byte alignment for `p_code` — is the one thing the comment doesn't mention.
- **`debug_callback` matches a bitflag by equality** — `instance.rs:67-72`: a message tagged
  `GENERAL | VALIDATION` falls through to `"Unknown"`.
- **`README.md` is entirely wrong** — it documents the C++/CMake/vcpkg incarnation: `SURGE_*` flags,
  submodules, OpenGL, a `SCOMP` tool that does not exist. Only "Philosophy" survives. It is the first thing
  anyone reads.
- **`surge-player/hot_reloading`** is a declared feature with two `//` placeholder comments and no
  implementation, for the capability the README calls the engine's reason to exist.
- **No `old_swapchain`** passed on recreate (`ctx_swpc.rs:96`); the old one is destroyed first,
  guaranteeing a visible hitch on a path that, per **H13**, runs at least once every launch.
- **VMA `vulkan_api_version` never set** (`command.rs:77-78`), so VMA defaults to 1.0 semantics on a 1.3
  device and skips 1.1+ paths.
- **Per-frame log spam** — `surge-mod-2048/src/lib.rs:96,104` log at `info!` inside the move/resolve arms,
  i.e. every frame of every animation. `init_env_logger` also hardcodes `LevelFilter::Debug` *after*
  `from_default_env()`, overriding `RUST_LOG`.
- **2048 renders one frame stale** — `lib.rs:76-107` submits every sprite before advancing the state
  machine, so the screen always shows the previous frame's positions.
- **`surge-mod-default` keeps animation state in `thread_local!` statics inside a dylib**
  (`lib.rs:63-100`) rather than in `AppDefault`, which is an empty struct. That state survives a module
  reload and will be a hot-reloading landmine. Its frame timer also drops the leftover accumulator on each
  advance, so it drifts.
- **Typos in public identifiers** — `DPETH_FORMAT`, `FenceWaiteError`, `fragmen_shader_name`,
  `fragment_sader`, "Winnit", "databasae", "recieves", "engin", "phaase", "has_all_mambers".
- **`stager/`** — `get_module_list()` is a bare `os.listdir` with no filter for real crate directories;
  `copytree` on a missing `assets/` throws an unhandled traceback; it copies `assets/svg/` (source artwork)
  into the release; `docopt.py` is vendored (581 lines of an unmaintained library) rather than pinned.
- **Zero tests in the workspace.** `board_geometry`, `right_ops`, `reconstruct_board`,
  `make_ortho_projection` (see **C6** — a three-line test would have caught it), and the subtexture UV
  derivation are all pure functions with obvious properties and no coverage.

---

## Verified sound — worth not breaking

These were checked closely and are correct; two of them are things engines routinely get wrong.

- **Per-image `render_finished_sem` indexing** (`ctx_swpc.rs:29-30, 47-51`, `ctx_command.rs:273-283`). The
  present-wait semaphore is indexed by *image*, not by frame, which is the correct fix for the classic
  present/acquire semaphore reuse hazard. The accompanying comments are accurate. (`present_completed_sem`
  is a different story — see **M20**.)
- **Rust/GLSL struct sizes** currently agree exactly: `InstanceRecord` 112 B vs std430 stride 112,
  `FrameGlobals` 128 B vs std140 block 128, `PushConstants` 8 B vs a 64-bit buffer reference. **C4** is
  about the absence of a guarantee, not about a present mismatch.
- **`LoadedApp` field order** (`app/mod.rs:12-15`) really does need to be `app` before `_library`, and is.
- **A clean run emits zero validation errors or warnings** — 154 info lines, 16 debug, nothing else.
- **`update_descriptor_sets` for the texture array** rewrites the whole array from index 0 on each upload,
  so descriptor indices stay consistent with `uploaded_textures` order.

---

## Not bugs — missing work, for completeness

`surge-mod-2048` is a movement demo, not 2048: no merging, no post-move spawn, no score, no win/lose, no
restart. `Up`/`Down`/`Left` push states but set no targets, so `update_live_pieces_pos()` returns `true`
immediately and the state pops with nothing having happened — three of four arrow keys are silently inert.
The `Up` arm additionally pushes `Resolve` before `Move` (`lib.rs:119-120`), inverting the order used by
the other three. `Piece::value` is a texture index in `1..=2`, not a tile value, so only the 2 and 4
sprites are reachable out of the twelve loaded. The `push(Move); push(Resolve); push(Idle); pop()` idiom
(`lib.rs:159-162`) is a queue rotation that depends on an `Idle` always sitting at the front, and
`board_idle()` returns `true` for an empty queue (`app_2048.rs:56-61`), so inputs are accepted in a state
the machine does not otherwise consider valid.

---

## Where I'd start

1. **C2**, **H9**, **M16** — one `.gitignore` line each and two identifier fixes; without them the repo is
   not buildable/runnable/extensible by anyone else.
2. **C6** — fix the projection before any more modules encode a `z` convention against it. A unit test on
   `make_ortho_projection` is three lines and closes the whole class.
3. **C3** — move the fence wait above `app.update`. Small change, removes a real race.
4. **C1**, **C4**, **H14** — use ash's safe `.color_attachment_formats()` setter; add `#[repr(C)]`; null the
   swapchain handle. Three one-liners against genuine UB and a dead safety guard.
5. **H11** + **H12** — match on `color_type`, and flush mapped writes. Both are small, both are
   memory-correctness rather than style.
6. **C5** + **M8** — give `upload_texture` a real handle type instead of a bare `usize` index, and split the
   texture budget from the instance budget.
7. **H1** + **H2** — treat `OUT_OF_DATE`/`SUBOPTIMAL` as control flow, not error. This unblocks resize,
   fullscreen, and **H3**.
8. **H13** + **M10** + **M17** — decide whether `config.resolution` is an input or an output, give modules a
   `&mut self` recreate hook that reports the new extent, and stop re-reading assets from disk inside the
   frame loop. These three are one design decision wearing three hats.
9. **H8** — an asset-path resolution API in `surge-core` (executable-relative, with a dev fallback), which
   fixes both modules at once.
10. **H10** — settle the module ABI before hot reloading is built on top of it.
