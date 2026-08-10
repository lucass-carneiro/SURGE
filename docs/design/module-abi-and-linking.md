# Module ABI & Linking Model

**Status:** Resolved (pass 8) — pass 7 turned the design into a commit-by-commit sequence; pass 8 is the
harsh-critic pass that checks *that sequence itself* for soundness (does step N actually compile given
what step N-1 left behind?), rather than checking the design's prose against the codebase again. It found
one sequencing bug severe enough to break the doc's own central architectural invariant — open question 41:
step 1.6 as pass 7 wrote it asks `surge-sdk` to host `SurgeApp` with a `&mut SpriteDatabase` parameter
*after* step 1.3 has already given `surge-core` a dependency on `surge-sdk`, which is a Cargo dependency
cycle, not just an awkward intermediate state; fixed by forcing 1.6 and pass 7's 1.7 into one commit, the
same way pass 7 itself already forced Q36's fix into 1.7. It also found four more commit-sequencing gaps
of the same shape — a step assumes machinery (a Cargo dependency, a trait method, a value's source) that no
step actually adds (open questions 42–45, 47–48) — and one honesty gap: the throwaway F5 reload trigger
pass 7 specified (Q40) cannot actually exercise the race condition Q22's fix was built for, because it fires
synchronously on the same thread as the blocked read it would need to race (open question 46). All eight are
resolved below with concrete fixes folded back into "Implementation plan." Pass 7's own paragraph, below, is
left as written; pass 6's is dropped per this doc's own convention of keeping only the immediately-preceding
pass's status paragraph as history (its content lives on in "Open questions" and "Self-critique" regardless).

**Status (pass 7):** Resolved — the design itself is unchanged in substance since pass 6; pass 7 turned it
into a commit-by-commit implementation sequence (see "Implementation plan" below) and, in the process of
checking that sequence against every real call site rather than just the ones earlier passes had already
grepped, found one more concrete gap the design had no answer for (open question 36: `surge-mod-2048`
calls three `SpriteDatabase` methods — `decode_texture_file`/`upload_decoded_texture`/`DecodedTexture` —
that don't exist anywhere in `SpriteCanvas` or the Q26 type migration, and structurally can't). It also
surfaced four implementation-scoped decisions with no single objectively-correct answer, put to the user
directly rather than picked unilaterally: how a module's release `[[bin]]` target gets its `surge-player`
dependency without dragging Vulkan into dev builds (open question 37), how the dlopen→process cutover gets
staged without one all-at-once commit (open question 38), where the module learns its IPC socket name
(open question 39), and whether the reload machinery gets even one throwaway way to be manually exercised
given its real trigger is out of this doc's scope (open question 40). All four are resolved with the
user's answers recorded alongside them.

**Origin:** CODE_REVIEW.md H10 ("Two statically-linked copies of `surge-core` and `std` in one
process"). What started as "give the module entry point a versioned, `extern "C"` ABI" grew into this
doc once two additional, concrete failure modes came up in discussion (see "Problem" below). Both are
bigger than H10's original scope and change what "fixing H10" means.

## Problem

Today, `surge-core` has no `[lib] crate-type` override in `surge-core/Cargo.toml`, so it defaults to
`rlib` — a static library. `surge-player` links one copy of it into the player binary; each module in
`surge-modules/*` (`crate-type = ["rlib", "dylib"]`) links a *second, independent* copy into its own
`.so`. This is directly visible:

```
$ ldd target/debug/libsurge_mod_2048.so
    libstdc++ libgcc_s libm libc     # no Rust libraries at all — std is statically linked in too
```

`surge_core::app::load_from_dylib` (`surge-core/src/app/mod.rs:55`) then crosses the boundary between
these two independently-compiled copies using a bare `fn() -> Box<dyn SurgeApp>` symbol — implicitly
Rust ABI, unversioned, no check that the two sides agree on anything. That was H10's original,
narrower finding.

Discussion surfaced two more concrete problems with the *current* static-linking architecture, not just
the entry-point symbol:

1. **`dlclose` is unreliable on Linux for Rust `.so`s.** Every module `.so` carries its own full copy
   of `std`, and `std` itself defines internal thread-locals (panic count, thread name/id caching,
   unwind bookkeeping) — so essentially any Rust `.so` has a nontrivial `PT_TLS` segment purely from
   linking `std`, before a module writes a line of its own code. `surge-mod-default/src/lib.rs:70,87`
   also uses `thread_local!` directly. glibc's dynamic-TLS accounting for `dlopen`'d objects is a known,
   long-standing weak point: it does not reliably reclaim a module's TLS block on `dlclose`, especially
   once any thread has touched it. This is exactly the failure mode reported from prior experience with
   this class of architecture — reload silently fails to release the old `.so`, or worse.

2. **Global/singleton state is duplicated per module, not shared.** Because `surge-core` is statically
   linked into each module, every `static`/future `OnceLock`/singleton-shaped thing living inside
   `surge-core` gets an independent copy per module. This is not hypothetical — it's already visible:
   `log`'s global logger state is duplicated per module today, which is *why* every module has to call
   `surge_core::cli::init_env_logger()` itself (`surge-mod-2048/src/lib.rs:82`,
   `surge-mod-default/src/lib.rs:31`) instead of the player initializing it once. Nothing today stops a
   future module from calling a hypothetical singleton constructor (e.g. a second `VulkanContext::new()`)
   and getting its own independent instance in the same process — it's avoided purely by convention (only
   `surge-player::main` calls it today), not by anything the type system enforces.

Both of these get worse, not better, once hot reloading (the engine's stated purpose, currently an
unimplemented stub — `surge-player/hot_reloading` feature, `// Handle hot reloading` placeholder in
`main.rs:99`) is actually built.

Separately, the *release* build pays the full cost of this architecture for no benefit: end users get
a `surge-player` executable that `dlopen`s a module `.so` at startup (`stager/stager.py` ships exactly
that pair), carrying the unversioned ABI, the duplicated-`std` footprint, and `libloading` as a runtime
dependency — none of which a shipped, non-hot-reloadable game needs.

Both of these turn out to have the same fix once dev-mode modules move out of the player's process
entirely — see "Proposed architecture" below. An earlier direction (share `surge-core` as one dylib
in-process, keep modules `dlopen`'d into the same process) was explored during discussion and is
superseded; it's why some of the language below still references an FFI vtable ABI as the *release-mode*
mechanism even though dev mode no longer uses one.

### This is a regression, not new ground — with one caveat

`README.md`'s Philosophy section (still accurate per `CLAUDE.md` — it's the one part of the stale
README that is) already describes the target shape:

> The **SURGE** core contains all of the engine's facilities condensed in **a single shared object**
> (DLL on Windows) ... Each module is compiled as a dynamic library object. This allows for modules to
> be *hot reloaded* ... The *player* takes care of loading, unloading and refreshing modules.

The Rust port currently does not do this — `surge-core` is statically duplicated, not a single shared
object. The architecture below achieves the *outcome* (hot-reloadable modules, no memory-reclaim
footgun) but not the *mechanism* the README describes: instead of one shared object with modules
`dlopen`'d into the same process, dev-mode modules run as a **separate OS process**, each statically
linking its own copy of `surge-core`. See "Divergence from README Philosophy" below — flagging this
honestly rather than silently drifting from the stated design.

## Goals

- No shared, aliasable state between the player and a dev-mode module — singleton-shaped bugs (like the
  duplicated `log` state) become impossible by construction (separate processes, separate address
  spaces), not just avoided by convention.
- A module entry-point protocol that is explicit and versioned, so a mismatch between player and module
  fails loudly at connect time instead of corrupting memory silently.
- A release build that is a single, statically-linked, self-contained binary per game — no `dlopen`, no
  IPC, no child process, no `libloading` dependency compiled in, no shipped `.so` alongside the player.
  "Hot reloading" (and its IPC cost) must be a dev-time-only cost.
- A reload lifecycle that does not depend on `dlclose` actually releasing memory, since that's an
  unreliable premise on Linux for Rust binaries — achieved here by never `dlclose`-ing anything, because
  dev-mode modules are never `dlopen`'d in the first place.

## Non-goals (for this doc)

- Designing the actual hot-reload *trigger* (file watching, debouncing, deciding when to swap a
  module). That's the other half of hot reloading, still unimplemented (previously tracked under the
  `surge-player/hot_reloading` Cargo feature — see resolved open question 21, that feature is deleted by
  this doc, not the *concept* it named); this doc only settles the linking/IPC/lifecycle foundation the
  trigger would be built on.
- Sandboxing modules against malicious code. Modules are trusted, first-party code in this project.
  Process isolation here is an *incidental* hard boundary (a module genuinely cannot touch the player's
  `VkInstance`/window handle, since it's a different address space), not a deliberately hardened one —
  no attempt is made to restrict what a module process can otherwise do on the machine (filesystem,
  network, etc.).
- Windows-specific process/IPC behavior. Everything below (process spawn, local socket, TLS-on-`dlclose`
  motivation) is scoped to Linux first; Windows named-pipe/process equivalents are a separate pass (see
  open question 12).
- Networked or non-local IPC. The module process always runs on the same machine, spawned directly by
  the player; nothing here generalizes to a remote/distributed module.

## Proposed architecture

Two build modes, selected at compile time, not runtime, and — as of pass 4 — selected by *which binary
you build*, not by a feature flag on a shared one. See resolved open question 21: the pre-existing
`surge-player/hot_reloading` feature is deleted, not repurposed. `surge-player`'s own binary has exactly
one job under this design (spawn a module child, speak the socket protocol to it), so there is no second
behavior left for a flag to select between — "dev mode" *is* "you ran the `surge-player` binary," full
stop. "Release mode" was never a configuration of that binary to begin with; it's a separate `[[bin]]`
target that lives on each module crate and never touches `surge-player`'s binary at all (see "Release
mode" below).

### Dev mode (always — running the `surge-player` binary is the dev-mode entry point)

- The module is compiled as an ordinary standalone binary — the *same* `[[bin]]` shape release mode
  uses (see below), statically linking `surge-core`/`surge-sdk` plus the game code directly. No
  `crate-type = "dylib"`, no `dlopen`, no `libloading`. This is a meaningful simplification over the
  originally-sketched dylib-sharing approach: the module binary builds the same way in both modes: the
  only thing that differs is who calls its `main()` and how it talks to the player.
- Only the player process creates the window and calls `VulkanContext::new` — this is now enforced by
  construction, not convention: the module process has no access to the player's address space at all,
  so it cannot construct a second `VulkanContext`/instance/device even if it tried.
- `surge-player` **binds and listens on a Unix domain socket before spawning**, then spawns the module
  binary as a child process (`std::process::Command`), passing it the socket path to connect to and
  whatever config it needs. Binding first — not after spawn — rules out a connect-before-listen race;
  the module is always the connecting side, the player always the listening side.
  - **Socket path lifecycle** (open question 23, resolved). A fixed, well-known path doesn't work: Unix
    `bind()` fails with `EADDRINUSE` on a stale file left behind by a player that didn't exit cleanly
    (crash, `SIGKILL`), and a fixed path collides outright if two player instances run concurrently
    (plausible in dev — comparing two modules side by side). Fix: use Linux's **abstract socket
    namespace** (a leading NUL byte in the path — `SocketAddr::from_abstract_name`, stable via the
    `std::os::linux::net::SocketAddrExt` trait, confirmed against current stable `rustc`), which is
    process-lifetime-scoped by the kernel and never touches the filesystem at all, so there's no
    stale file to clean up and no collision to guard against beyond picking a per-run-unique name (e.g.
    the player's own PID). This sidesteps the whole cleanup question rather than solving it.
- **Module discovery / config schema.** Today `[startup_app] app_folder`/`app_name` resolve to
  `dlopen("lib{app_name}.so")` (`surge_core::app::load_from_dylib`). They're reinterpreted here, not
  replaced: `app_folder`/`app_name` resolve to the standalone module binary's path instead
  (`{app_folder}/{app_name}`, no `lib`/`.so` decoration) — same `[startup_app]` shape in both dev and
  release, only the resolution logic behind it changes.
- **IPC channel**, established right after spawn. Player → module carries `dt` and the frame's input
  events each turn; module → player carries the instances the module wants drawn this frame (small,
  fixed-shape — at most `max_sprites` = 32 `InstanceInfo`-shaped records) plus any texture-upload
  requests. See "Message shape" below for the exact `Request`/`Response`/`CanvasOps` shapes this
  resolves to — `KeyEvent` needs a wire-format mirror type (see `surge-sdk` section and open question
  13); `MouseButton`/`MouseScrollDelta`/`ElementState`/`TouchPhase` reuse winit's own types directly,
  since winit's `serde` feature derives `Serialize`/`Deserialize` for all four; `DeviceId` is dropped
  from the wire entirely rather than mirrored (open question 19 — it doesn't derive anything under
  winit's `serde` feature on any platform, so it can't cross `postcard` as-is, and neither existing
  module reads it for anything).
  - Texture uploads cross as **asset paths, not pixel bytes**: both processes already share the same
    filesystem/CWD contract (per `CLAUDE.md`'s CWD contract), so the module tells the player *which*
    PNG to load and the player does the actual file IO + `upload_texture` call itself, in-process. This
    avoids serializing image bytes across the boundary entirely.
  - A `RemoteApp` type, living in **`surge-player`** (settled — see open question 20; pass 3 had left
    this an open "or `surge-core`"), implements `SurgeApp`. **Correction, pass 5 (open question 31):**
    earlier passes described this as forwarding "each trait method" across the socket; only
    `on_load`/`on_swapchain_recreate`/`update` actually do — those are the three with a matching
    `Request` variant. `keyboard_event`/`mouse_button_event`/`mouse_wheel_event` have no `Request`
    counterpart at all: `RemoteApp`'s impls of them just push onto a local `Vec<InputEvent>` with no IPC,
    and that queue is what gets drained into the next `Request::Frame`'s `input_events` when `update`
    actually triggers a round trip. So "forwards each trait method, blocking for that frame's response"
    only describes half the trait; the other half is pure local buffering. This plays the role `LoadedApp`
    played in the dylib-vtable design: `surge-player`'s run loop still only ever deals in `Box<dyn
    SurgeApp>` and never knows whether it's driving a `RemoteApp` (dev, IPC-backed) or a
    directly-constructed module (release, in-process) — same run-loop code either way. `RemoteApp` is
    also what actually drives the real `&mut dyn SpriteCanvas` the run loop gave it: on receiving a
    `CanvasOps` reply, it applies `texture_uploads` then `instances` against that canvas itself — the
    module process never touches a canvas reference at all, it only ever describes what it wants done.
  - The mirror image on the module side — connect, send the unprompted `Handshake`, then loop on
    `Request`s replaying `input_events` into the concrete app's `keyboard_event`/`mouse_button_event`/
    `mouse_wheel_event` (in wire order), calling `on_load`/`update`/`on_swapchain_recreate`, and shipping
    back whatever the module's `SpriteCanvas` proxy buffered as that call's `CanvasOps` — is likewise not
    hand-rolled per module. It's `surge_sdk::run_dev_module(app: Box<dyn SurgeApp>) -> !`, symmetric to
    `surge-player`'s own `run()` (see release mode below): every module's dev entry point is `fn main()
    { surge_sdk::run_dev_module(Box::new(App2048::new())) }` and nothing else. This was implicit in past
    passes (the "IPC-proxy type" bullet under `surge-sdk` describes its buffering behavior but not the
    loop that drives it) — naming and placing it here so it isn't reverse-engineered at implementation
    time.
  - **Message shape** (open question 16, resolved). Two enums, `Request` (player → module) and
    `Response` (module → player), cover the whole lifecycle, not just the per-frame loop:

    ```rust
    enum Request {
        // create_info is pushed here rather than pulled on demand — see open question 27.
        OnLoad { create_info: surge_sdk::CreateInfo },
        OnSwapchainRecreate { create_info: surge_sdk::CreateInfo },
        Frame { dt: f32, input_events: Vec<InputEvent> },
        Shutdown,
    }

    enum Response {
        Handshake { wire_version: u16 },
        CanvasOps(CanvasOps),   // reply to OnLoad, OnSwapchainRecreate, and Frame alike
        ShutdownAck,
    }

    struct CanvasOps {
        /// Asset paths, in call order — texture_id is this call's position in the sequence,
        /// exactly as SpriteDatabase::upload_texture is positional today (see open question 17).
        texture_uploads: Vec<String>,
        /// Empty in practice for OnLoad/OnSwapchainRecreate, populated for Frame.
        instances: Vec<surge_sdk::InstanceInfo>,
    }

    enum InputEvent {
        Keyboard { event: surge_sdk::KeyEvent, is_synthetic: bool },
        MouseButton { state: ElementState, button: MouseButton },
        MouseWheel { delta: MouseScrollDelta, phase: TouchPhase },
    }
    ```

    `InstanceInfo` and `CreateInfo` are written as `surge_sdk::` here deliberately, not
    `surge_core::vulkan::sprite_database::` as today's code has it — see open question 26, they (and
    `SubTextureInfo`/`BlendingMode`/`TextureFilteringMode`/`TextureFilteringLevel`) move to `surge-sdk`
    and gain `Serialize`/`Deserialize` as part of this doc, they don't stay `surge-core`-only types
    threaded through the wire format unchanged. `create_info` on `OnLoad`/`OnSwapchainRecreate` is pushed
    rather than pulled — see open question 27, there is otherwise no mechanism for the module to ask the
    player anything mid-turn.

    Note `DeviceId` does **not** appear in this enum — see resolved open question 19. It's dropped from
    the wire format entirely, not mirrored. On the module side, `surge_sdk::run_dev_module` reconstructs
    a placeholder via `DeviceId::dummy()` when it dispatches each `InputEvent` back into the concrete
    app's `keyboard_event`/`mouse_button_event`/`mouse_wheel_event` — the same value every time, since
    neither existing module distinguishes input devices.

    `on_load` and `on_swapchain_recreate` reuse the same `CanvasOps` response shape `Frame` uses — both
    are just "the module got to mutate a `SpriteCanvas`, here's what it did" — rather than inventing a
    separate payload per lifecycle call. `on_unload` is *not* a distinct `Request`/`Response` round trip:
    it's folded into `Shutdown`/`ShutdownAck` below, since the player never needs anything back from
    `on_unload` beyond "the module is done."
  - **Protocol versioning / connect sequence:** the module speaks first. Immediately after connecting,
    before the player sends any `Request`, the module sends `Response::Handshake { wire_version }`
    unprompted. The player validates it; on a mismatch, it never sends `Request::OnLoad` at all and
    instead routes the child into the same crash-state path as open question 11 (loud log, distinct
    on-screen/log indicator, no silent retry). On a match, the player proceeds with `Request::OnLoad`,
    then the steady-state `Request::Frame` loop, then `Request::Shutdown` on exit — the player waits
    briefly for `Response::ShutdownAck` before falling back to the same `SIGTERM`-then-`SIGKILL` sequence
    reload already uses, so a hung module on shutdown can't hang player exit.
  - **`texture_id` handshake** (open question 17, resolved). Traced to the root cause: this was never
    really an IPC problem. `SpriteDatabase::upload_decoded_texture` (`surge-core/src/vulkan/
    sprite_database/mod.rs:643`) already returns `Ok(())` — not the assigned index — even in today's
    single-process code, and silently no-ops (still returning `Ok(())`, not an `Err`) once
    `uploaded_textures.len() == max_textures`. Neither existing module reads the return value for an ID
    anyway — `AppDefault`/`App2048` both hardcode `texture_id` as a literal matching upload order, i.e.
    they already trust positional prediction, not a returned value. So the dev-mode `SpriteCanvas` proxy
    doing the same (assigning `texture_id` as this call's position in `CanvasOps.texture_uploads`, no
    round trip, no ack) is not a new assumption — it's the existing one, carried across a process
    boundary unchanged. Two changes make the desync case safe instead of silent:
    1. **Fix the source bug alongside this work:** `upload_texture`/`upload_decoded_texture` change to
       `Result<usize, VulkanError>`, returning the real assigned index on success and a genuine `Err`
       (not a silently-successful `Ok(())`) once the database is full. Worth doing regardless of this
       doc — it's a plain correctness bug in `surge-core` today.
    2. **On the player side, a `CanvasOps.texture_uploads` failure is treated as a module fault, not a
       partial success to reconcile.** The player processes `texture_uploads` in order; the moment one
       fails, it stops (no attempt to patch up the module's now-wrong positional assumptions for
       everything uploaded after it) and routes the module into the exact same crash-state path open
       question 11 already defines for a dead connection: loud log, distinct on-screen/log indicator, no
       silent continuation. No new wire message needed — this reuses Q11's existing machinery, it's just
       triggered by a new cause.
- **Reload:** kill the child process (`SIGTERM`, wait, `SIGKILL` if it doesn't exit promptly), respawn a
  freshly-built binary, reconnect the socket. No `dlclose`, nothing to leak — the OS reclaims the whole
  child process's address space, TLS segment included, on exit. This resolves Problem #1 by construction
  instead of by convention or a "never `dlclose`, leak forever" workaround.
  - **Reload racing an in-flight, blocked `Request::Frame`** (open question 22, resolved). Q10's
    backpressure policy is synchronous blocking: the player's main thread can be sitting in a blocked
    socket read waiting on a `Response` at the exact moment a reload trigger fires. Killing the child at
    that moment unblocks the read with the same observable failure (EOF/broken pipe) that Q11 defines as
    an unexpected crash — nothing so far distinguishes "the player did this on purpose" from "the module
    actually died," so a deliberate reload could get misrouted into Q11's crash-state UI/logging path.
    Fix: a `reload_in_progress` flag (or equivalently, a oneshot channel the reload trigger writes to
    before signaling the child), set immediately before `SIGTERM` is sent and checked by the main thread
    the moment its blocked read returns an error, *before* it decides whether to take the Q11 crash path.
    Cleared once the respawn/reconnect/re-handshake sequence completes. This is purely a sequencing rule
    on top of machinery this doc already specifies (Q10's blocking, Q11's crash detection, the reload
    steps below) — no new wire message, no new component.
  - **The player's `SpriteDatabase` must be rebuilt on every reload, not just the socket reconnected**
    (see open question 14). `uploaded_textures` is capped at `ci.max_textures` (32, shared with
    `max_sprites`) with no clear/reset method on `SpriteDatabase` — the only way to empty it today is a
    full rebuild, the same one `recreate_swapchain` already does (`surge-player/src/main.rs`). Skip this
    and a module that uploads N textures in `on_load` (e.g. `surge-mod-2048` uploads 12) blows the
    32-slot cap after `32 / N` reloads and starts silently dropping uploads — breaking hot reload, dev
    mode's entire purpose, within a couple of iterations of normal use. Fix: reuse the
    swapchain-recreation rebuild path on every reload rather than adding a new narrower clear method.
  - **Orphaned child on player crash.** The kill/respawn path above only covers a *deliberate* reload.
    If the player panics or is killed before `exiting()` runs, the spawned module process has no
    supervisor and is orphaned. Linux-only fix, consistent with this doc's Linux-first scope: set
    `PR_SET_PDEATHSIG` on the child at spawn time so the kernel kills it automatically if the player dies
    for any reason. Implementation footnote, not a design gap: `PR_SET_PDEATHSIG` is a raw `prctl(2)`
    call with no `std` wrapper, so this needs a `libc` or `nix` dependency added to `surge-player` —
    neither is in the workspace today. Flagging so it isn't a surprise mid-implementation, not something
    that changes the design. **Pass 5 note (open question 30):** that same dependency is not optional
    background plumbing — it's also required to send the `SIGTERM` this doc's reload/shutdown sequences
    depend on. `std::process::Child::kill()` sends `SIGKILL` unconditionally on Unix; there is no `std`
    API for `SIGTERM` specifically. Built against `Child::kill()` alone, "SIGTERM, wait, SIGKILL if it
    doesn't exit promptly" silently degrades to "always hard-kill immediately" on every reload, not just
    the timeout case. `libc::kill(pid, libc::SIGTERM)`/`nix::sys::signal::kill` (the same crate this
    bullet already pulls in) covers both needs.

### Release mode (default; no *runtime-selectable* feature flag)

Caveat added in pass 7 (see open question 37): "no feature flags" here means `surge-player`'s own binary
has no mode switch (Q21 still holds exactly as resolved). It does not mean *no Cargo feature anywhere in
this design* — each module crate ends up with one, `release-bin`, but it selects which `[[bin]]` *target*
to build, not a runtime behavior of an already-built binary, which is a different thing from what Q21
killed. See Q37 for why Cargo forces this and why it's the least-bad option among the ones considered.

- `surge-core` is a plain `rlib`, statically linked, exactly as today. There is **no universal player
  binary** that loads an arbitrary module by name — a single static binary must pick its module at
  compile time. So "single binary blob" is necessarily *per game*: each module crate gains its own
  `[[bin]]` target that statically links `surge-core` + a shared, app-agnostic `run(app: Box<dyn
  SurgeApp>, config: EngineConfig)` entry point + the module itself, and calls e.g. `App2048::new()`
  directly.
- No child process, no IPC, no `dlopen`, no vtable indirection — just an ordinary `Box<dyn SurgeApp>`
  constructed in-process, which is completely normal, zero-FFI-risk Rust. The module calls the real
  `SpriteDatabase` directly — no IPC proxy in this path.
- `config.toml` and `assets/` remain external files read at runtime, unchanged from today (see resolved
  open question 5, below) — not embedded into the binary.

This means today's `surge-player::main`/`SurgeContext` (`surge-player/src/main.rs`) needs to stop being
`surge-player`-the-binary's private implementation and become a reusable library entry point. Correction
from pass 1 of this doc: `SurgeContext` is *not* already generic today — `startup_app` is a concrete
`LoadedApp` field (`surge-player/src/main.rs:16-31`), and `LoadedApp`'s entire reason for existing (the
load-bearing `app`-before-`_library` drop-order invariant) is `dlopen`-specific and has no counterpart in
either new mode, since neither dev nor release mode `dlopen`s anything anymore. `LoadedApp` and
`load_from_dylib` are deleted outright, not adapted — nobody needs to preserve that drop order in the new
design. **Pass 5 note (open question 29):** deleted means deleted all the way down, which the doc hadn't
traced through before now. `surge-core/src/errors/mod.rs`'s `AppError` exists solely to type the errors
`load_from_dylib` returns — nothing else in `surge-core` or `surge-player` references it — so it becomes
fully dead code too. More concretely, `libloading` is an unconditional, top-level dependency of
`surge-core/Cargo.toml` itself (not `surge-player`'s), used nowhere except `surge-core/src/app/mod.rs`;
left in place it directly contradicts this doc's own release-mode goal ("no ... `libloading` dependency
compiled in"), since `surge-core` links into every release binary regardless of module. (The same file
also defines a `ModuleError` enum, identically `libloading`-shaped and already entirely unreferenced
today — a preexisting wart this doc doesn't need to fix, but it underlines that `libloading` in
`surge-core` has no legitimate remaining caller once `app/mod.rs` is gone.) Fix, spelled out rather than
left implicit: delete `surge-core/src/app/mod.rs` in full, delete `AppError`, and remove `libloading`
from `surge-core/Cargo.toml`.

What *is* true, and does carry over cheaply: the render loop only ever calls through the
`SurgeApp`/`Deref` interface, never touches a concrete module type, so swapping the `startup_app` field's
type from `LoadedApp` to `Box<dyn SurgeApp>` is a small, mechanical change, not a rethink of the loop
itself. **Pass 8 note (open question 43):** "small, mechanical change" undersold *when* it happens, not
*whether* — it can't wait for the `[lib]`-extraction step below, since the dev/release cutover (Phase 3 in
"Implementation plan") needs a single field capable of holding either a `dlopen`-backed app or an IPC-backed
one well before that extraction runs. See Q43 for the temporary `StartupApp` enum this actually goes
through on the way to `Box<dyn SurgeApp>`. The plan is for `surge-player` to gain a `[lib]` target exposing
`pub fn run(app: Box<dyn SurgeApp>, config: EngineConfig)`, used by:
- `surge-player`'s own dev binary, handing it a `RemoteApp` after spawning + connecting to the module
  child process.
- Each module's release `[[bin]]` target, called directly with a statically-constructed app.

This keeps the core/player/module split the README's Philosophy section describes, rather than moving
the run-loop into `surge-core` itself.

**Drop the vestigial `surge-mod-default` dependency** (open question 25, resolved). `CLAUDE.md` already
notes `surge-player` has a direct `path` dependency on `surge-mod-default` that's vestigial today, since
loading is entirely dynamic. This design gives it even less reason to exist: `surge-player`'s binary
never statically knows about any specific module in either mode (dev connects to whatever
`app_name`/`app_folder` in `config.toml` names; release's static module knowledge lives on the *module's*
`[[bin]]` target, not on `surge-player`). Delete the dependency as part of this work rather than carrying
it forward unexamined.

### `surge-sdk` — API surface, not a safety boundary

Process isolation already gives a *hard* guarantee that a dev-mode module can't reach the player's
`VulkanContext`/window (different address space, not just different visibility scope) — stronger than
anything a crate split could offer, and it costs nothing extra to get. So `surge-sdk`'s job changes from
"the only real enforcement mechanism" to "a curated, ergonomic API surface for module authors," and,
more importantly, it becomes **structurally necessary** rather than optional: a dev-mode module
physically cannot hold `&mut SpriteDatabase` (it's a type living in a different process), so *something*
has to stand in for it regardless of whether `surge-sdk` gets built as a separate crate.

Concretely, `surge-sdk` should define:

- The `SurgeApp` trait — **not** unchanged from today (pass 1 of this doc claimed it was; that's wrong).
  Its signatures have to move off types a dev-mode module process cannot produce, checked against the
  actual code (see open question 13):
  - `on_load`/`on_swapchain_recreate`/`update` take `&mut dyn SpriteCanvas` (below), not
    `&mut SpriteDatabase`. `SpriteDatabase::new` requires a live `Arc<RefCell<VulkanContext>>`
    (`surge-core/src/vulkan/sprite_database/mod.rs`), and a dev-mode module has no `VulkanContext` by
    construction — it can never hold or construct a real `SpriteDatabase`.
  - `keyboard_event` takes a `surge_sdk::KeyEvent` mirror struct, not `winit::event::KeyEvent`. Checked
    against winit 0.30.12: `KeyEvent::platform_specific` is `pub(crate)` with no public constructor and
    no `Default` impl, so it's not constructible outside the `winit` crate itself — a module-side proxy
    dispatching a deserialized wire message into this trait method could never produce one. **Pass 6
    (open question 35):** every pass through pass 5 referenced this mirror struct without ever writing
    down its fields. Pinned down and compile-spiked in pass 6 (see resolved open question 33):
    ```rust
    pub struct KeyEvent {
        pub physical_key: winit::keyboard::PhysicalKey,
        pub logical_key: winit::keyboard::Key,
        pub text: Option<String>,
        pub location: winit::keyboard::KeyLocation,
        pub state: winit::event::ElementState,
        pub repeat: bool,
    }
    ```
    This is winit's own `KeyEvent`'s six public fields, unchanged in type except `text`: winit uses
    `Option<SmolStr>`, mirrored here as `Option<String>` to avoid pulling in `smol_str` as a direct
    `surge-sdk` dependency just for this one field — a deliberate simplification, not a forced one
    (`smol_str`'s own `serde` feature is reachable transitively through winit's `serde` feature and was
    spiked working fine too, so `Option<SmolStr>` would also have worked). `physical_key`/`logical_key`/
    `location`/`state` reuse winit's own `PhysicalKey`/`Key`/`KeyLocation`/`ElementState` types directly
    (all confirmed `Serialize`/`Deserialize` under winit's `serde` feature, including exercising `Key`'s
    `Character(SmolStr)` variant specifically, not just the simpler `Named` one) rather than mirroring
    those too — consistent with this doc's existing "wrap only what's unconstructible" principle for
    `mouse_button_event`/`mouse_wheel_event`'s types.
  - `mouse_button_event`/`mouse_wheel_event` keep winit's own `MouseButton`, `MouseScrollDelta`,
    `ElementState`, and `TouchPhase` as-is — all public, `Copy`, `Serialize`/`Deserialize` under winit's
    `serde` feature (checked against 0.30.12's source: all four carry
    `#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]`), so there's no forcing reason to
    wrap them too. `DeviceId` is the one exception — see open question 19, it's dropped from the wire
    entirely rather than kept as-is. Deliberately minimal surface change otherwise, not a blanket
    SDK-owned mirror of every event type.
  - In release mode, `impl SpriteCanvas for SpriteDatabase` is the one extra hop needed to keep calling
    the real thing directly — still zero IPC, just one vtable indirection instead of a concrete-type
    call. This impl lives in `surge-core` (see open question 20 for why, and for the crate-dependency
    consequence it carries).
- **The plain-data types every module already builds by hand today** — `InstanceInfo`, `SubTextureInfo`,
  `CreateInfo`, `BlendingMode`, `TextureFilteringMode`, `TextureFilteringLevel` — move here from
  `surge-core::vulkan::sprite_database` (open question 26, resolved). None of the six touch `ash`/
  `vk-mem`; they're plain data, so the move costs `surge-sdk` no Vulkan knowledge. This isn't optional
  tidying: both real modules construct `InstanceInfo`/`SubTextureInfo` values directly
  (`surge-mod-2048/src/piece.rs:34`, `surge-mod-default/src/lib.rs:43`) and read `CreateInfo` fields
  (`surge-mod-2048/src/lib.rs:64-65`), and modules can no longer depend on `surge-core` to get these
  types from. All six gain `#[derive(Serialize, Deserialize)]` — `InstanceInfo`/`SubTextureInfo` are
  `Debug`-only today — since `CanvasOps.instances: Vec<InstanceInfo>` (see "Message shape") has to cross
  `postcard`, and their `nalgebra::Vector2`/`Vector4` fields need nalgebra's `serde-serialize` Cargo
  feature enabled wherever `surge-sdk` depends on nalgebra (checked: not enabled anywhere in the
  workspace today). `surge-core` depends on `surge-sdk` for these — consistent with the dependency
  direction open question 20 already established, just extended past the one `SpriteCanvas` impl to
  cover the types the wire format actually carries. One more consequence worth flagging: every module
  (`surge-mod-2048/Cargo.toml`, `surge-mod-default/Cargo.toml`) currently declares its own separate
  `nalgebra = "0.34.1"` dependency, not sourced through `surge-sdk`. That only type-checks today because
  the version string happens to match everywhere; a module's hand-written `InstanceInfo { position:
  Vector2::new(...), .. }` literal only compiles against `surge_sdk::InstanceInfo` if the module's own
  `Vector2` is the *same* monomorphized type as `surge-sdk`'s. Modules should drop their own `nalgebra`
  dependency and use `surge_sdk`'s re-exported math types exclusively, so a future version bump can't
  silently produce two incompatible `Vector2` types at the `InstanceInfo` boundary.
- **A `CanvasError` type, owned by `surge-sdk`** (open question 28, resolved) — an opaque,
  message-carrying error, not a reuse of `surge-core`'s `VulkanError`. `SpriteCanvas::upload_texture`
  has to be fallible (see open question 17's fix to the underlying `SpriteDatabase` method), but
  `VulkanError` is a `surge-core` type and `SpriteCanvas` lives in `surge-sdk`, which stays Vulkan-free —
  naming `VulkanError` in the trait signature would force `surge-sdk` to depend on `surge-core`,
  contradicting the acyclic graph below. **Correction, pass 6 (open question 34):** pass 5 additionally
  claimed `surge-core`'s conversion from `VulkanError` to `CanvasError` had to be a hand-written
  `.map_err(...)` closure because `impl From<VulkanError> for CanvasError` "would itself be an
  orphan-rule violation, since neither `From` nor `CanvasError` is local to that crate." That's wrong,
  and demonstrably so — compile-spiked in a real two-crate workspace (resolved open question 33) where
  `impl From<VulkanError> for CanvasError` builds cleanly. The orphan rule doesn't require the `for` type
  to be local; it requires *at least one of the trait's own type parameters* to be local, scanning
  left-to-right, with no uncovered impl-generic type parameter appearing before it. Here the impl has no
  generic parameters at all, and `From`'s one type parameter — `VulkanError`, the `T` in `From<T>` — *is*
  local to `surge-core`. That alone satisfies the rule; `CanvasError` being foreign (from `surge-sdk`) is
  irrelevant to it. (The classic, more commonly-seen shape of this same rule is `impl From<ForeignError>
  for MyLocalError`, e.g. wrapping `std::io::Error`; this is the same rule applied in the other type
  slot.) Fix: the design reverts to the simpler, more idiomatic form — `surge-core` writes `impl
  From<VulkanError> for CanvasError`, and `impl SpriteCanvas for SpriteDatabase`'s `upload_texture` uses
  plain `?` at the call site instead of a hand-rolled closure. No behavior change, just less code, now
  that the reason given for avoiding it turned out not to hold. Worth stating plainly rather than
  leaving it implicit: the dev-mode `SpriteCanvas` proxy can *never* actually produce this `Err`.
  Uploads are fire-and-forget until the end-of-turn `CanvasOps` (see "Message shape"); a real failure is
  only discoverable player-side, after the module process has already returned control, and is routed
  through open question 11's crash path instead of back through this `Result`. The same trait method is
  genuinely fallible under the release backend and unconditionally `Ok` under the dev backend — callers
  shouldn't treat the two as equivalent, and that's worth a doc comment on the trait method itself.
- A `SpriteCanvas`-shaped trait covering the module-facing instance API (`add_instance`,
  `upload_texture`, `get_create_info`, ...), with two backends:
  - `SpriteDatabase` directly, for release-mode in-process modules.
  - An IPC-proxy type, for dev-mode child-process modules, that buffers `add_instance`/`upload_texture`
    calls locally through the callback and flushes them as one `CanvasOps` response at the end (see
    "Message shape" above), and answers `get_create_info()` from a `CreateInfo` cached out of the most
    recent `Request::OnLoad`/`Request::OnSwapchainRecreate` rather than any round trip (open question 27,
    resolved — the `Request`/`Response` pair as drafted through pass 4 had no mechanism for the module to
    *ask* the player anything mid-turn, and `get_create_info()` is exactly that: `surge-mod-2048/src/
    lib.rs:62-66` calls it from `add_board_sprite`, which `update()` calls every frame, not just once at
    `on_load`. Since `CreateInfo` is fully known by the player ahead of time — from `EngineConfig`/the
    hardcoded `ci` literal in `surge-player/src/main.rs`, never from anything the module computes — the
    fix is to push it rather than have the module pull it, at the two points the player is about to hand
    the module a freshly-(re)built `SpriteDatabase` anyway. See the updated `Request` enum under
    "Message shape").
- `run_dev_module(app: Box<dyn SurgeApp>) -> !` — the module-side connect/handshake/dispatch loop (see
  open question 24), the mirror image of `surge-player`'s `run()`. Every module's dev-mode `main()` is a
  one-line call into this.
- **Asset-path helpers, logger init, math re-exports** — as originally scoped, but named explicitly here
  (open question 32, resolved): `resolve_asset_path` (`surge-core/src/assets/mod.rs`) and
  `init_env_logger` (`surge-core/src/cli/mod.rs`) are both called directly by every module today
  (`surge-mod-2048/src/lib.rs:50,82`, `surge-mod-default/src/lib.rs:24,31`), both currently live in
  `surge-core`, and neither has any Vulkan coupling, so both move to `surge-sdk` alongside the plain-data
  types above. One behavior change worth confirming as intentional, not a regression: `resolve_asset_path`
  keys its staged-path fast path off `std::env::current_exe()`, which today (module `dlopen`'d into the
  player) always resolves to the *player's* binary. Under this redesign a dev-mode module is its own
  process running its own `[[bin]]`, so `current_exe()` now resolves to *that module's* binary instead —
  which is exactly what makes the release `[[bin]]`-per-module + `stager.py` "copy binary + its own
  `assets/`" story (open question 7) work unmodified, so it's the right behavior, just a real change in
  what the function observes. The CWD-relative dev fallback (`surge-modules/<name>/assets/`) still
  resolves correctly without any explicit fix: `std::process::Command` inherits the parent's CWD by
  default, this doc never overrides it, so a spawned module process's CWD is the player's CWD (the repo
  root, per the existing CWD contract) — worth flagging so that assumption doesn't get silently broken by
  a future `.current_dir(...)` addition to the spawn call.

Modules depend on `surge-sdk`, not `surge-core`, in both modes; `surge-core` keeps everything
(`VulkanContext`, window setup, the real `SpriteDatabase`) and only `surge-player` depends on it
directly. The socket/process-spawn machinery (`RemoteApp`, listening, spawning, killing) lives in
`surge-player` itself, not `surge-core` — see open question 20.

**New dependency edge (open question 20):** `surge-core` gains a dependency on `surge-sdk` that doesn't
exist today. This is required by Rust's orphan rule, not a stylistic choice: `impl SpriteCanvas for
SpriteDatabase` needs to live in a crate that owns either the trait (`surge-sdk`) or the type
(`surge-core`) — `surge-player` owns neither, so it's the one crate in this graph that structurally
*cannot* write that impl, no matter how convenient it would be to keep it next to `RemoteApp`. Landing
it in `surge-core` (rather than `surge-sdk`, the other legal option) keeps `surge-sdk` free of any
Vulkan/`SpriteDatabase` knowledge, matching its stated job as a thin, dependency-light API surface. **Pass
5 note:** this edge carries more than "this one impl" — per open question 26, `surge-core` also now
*consumes* `surge-sdk`'s `InstanceInfo`/`CreateInfo`/`SubTextureInfo`/`BlendingMode`/
`TextureFilteringMode`/`TextureFilteringLevel` in `SpriteDatabase`'s own public signatures (`add_instance`,
`upload_texture`, `get_create_info`, `SpriteDatabase::new`), not just in the `SpriteCanvas` impl body —
same edge, same direction, just wider than pass 4 scoped it. The resulting graph, bottom to top, is:
`surge-sdk` (no internal deps) → `surge-core` (depends on `surge-sdk`, for the shared data types and the
`SpriteCanvas` impl) → `surge-player` (depends on both) → each module crate (depends on `surge-sdk` only
in dev mode; its release `[[bin]]` target additionally depends on `surge-player` for `run()`, pulling
`surge-core` in transitively). No cycle: `surge-sdk` never depends back on `surge-core` or
`surge-player` — which is exactly why `SpriteCanvas::upload_texture`'s error type can't be `surge-core`'s
`VulkanError` (see the `CanvasError` bullet above, open question 28).

## Divergence from README Philosophy

Flagging honestly: the README's Philosophy section describes *one shared object* with modules `dlopen`'d
into the *same* process. This doc's chosen mechanism is different — dev-mode modules are a **separate
process**, each statically linking its own copy of `surge-core`. The observable goal (modules can be
hot-reloaded without restarting the player, without a slow accumulating leak, without corrupting shared
state) is preserved; the "single shared object" mechanism is not. This trade was made because the
single-shared-object approach depends on `dlclose` reliably reclaiming a module's TLS on Linux, which
Problem #1 argues is not a sound premise to build on. Worth being explicit about since `CLAUDE.md` calls
the Philosophy section still-accurate — this doc now knowingly departs from one part of it, for a
concrete technical reason, not by accident.

## Open questions

The original set (1–12, from pass 1) is resolved; item 12 is an explicit non-goal, not a pending
decision. Pass 2 (13–18) found two of the original "resolved" items didn't hold up against the actual
code and fixed them, and surfaced two genuinely new decisions (16, 17) that it left open. Pass 3 closes
both: 16 (wire protocol message shape) with a concrete `Request`/`Response`/`CanvasOps` enum design, and
17 (`texture_id` handshake) by tracing it to a pre-existing `surge-core` bug and fixing it at the source
rather than adding IPC-side reconciliation logic. Pass 4 was a harsh-critic pass checked directly against
the actual source (winit's serde derives, Rust's orphan rule, the workspace `Cargo.toml`s) rather than
just re-reading the doc for internal consistency; it found one item that flatly wouldn't compile as
written (19) and one crate-dependency consequence the doc had left unstated because it never pinned down
*which* crate hosts a given piece (20), plus four smaller coordination/lifecycle/cleanup gaps (21–25).
Pass 5 turned the same method on `surge-sdk` specifically — the crate that doesn't exist yet, checked
against what the two real modules actually import/call and what `nalgebra`/`Cargo.toml` actually enable
— and found one more wire-format type that wouldn't compile plus a crate-ownership question the doc
hadn't asked (26), one IPC round trip the design was simply missing (27), one more orphan-rule-shaped
error-type problem (28), and four smaller cleanup/precision gaps (29–32). Pass 5's own self-critique
predicted that a further read-through pass would have a nonzero false-negative rate against this class of
bug, and that pass 6 would be better spent actually compiling `surge-sdk`'s proposed types than reading
the doc again. Pass 6 did that (resolved open question 33) and it paid off exactly as predicted: it found
one place where pass 5's *own* orphan-rule reasoning was wrong, not just unchecked (34), and it pinned
down a type this doc had referenced by name for three passes without ever defining (35). Pass 7 turned the
doc into a commit sequence (see "Implementation plan") rather than reading it again, and the same class of
bug turned up a fourth time anyway (36) — not because the method stopped working, but because grepping for
`InstanceInfo`/`get_create_info` call sites (pass 5's search) and grepping for every method a module calls
on its `SpriteDatabase` parameter (pass 7's search, needed to write commit-by-commit steps) are different
searches with different blind spots. It also produced four decisions that don't have a single correct
answer independent of what the user wants (37–40), resolved by asking directly instead of guessing. Pass 8
turned the same scrutiny on the commit sequence itself rather than the design prose — not "is this claim
true against the code" (pass 4–7's question) but "does step N compile given exactly what steps 1 through
N-1 actually left behind" — and found the same lesson land a third time: a plan can be internally
inconsistent in ways no single-step read catches. It found one sequencing bug that breaks the doc's own
acyclic-dependency invariant (41: step 1.6 as pass 7 wrote it can't compile), a cluster of three
"step N assumes machinery no step actually adds" gaps in the Phase 3 cutover (42: a dependency; 43: a
field's type; 44: a value's source), a missing trait hook the throwaway F5 keybind (Q40) needs to work at
all (45), an honesty gap about what that same keybind can and can't prove once 45 lets it run (46), and two
more "assumes machinery no step adds" gaps in the Phase 2 IPC plumbing (47: wire framing; 48: two Cargo
dependency/feature nits). All 48 items are now resolved.

### Resolved

1. **`surge-sdk` API-surface split** — build it. Its role shifted from "safety boundary" to "structurally
   necessary abstraction" (see above): dev-mode modules need *some* stand-in for `SpriteDatabase`
   regardless, so the trait split turns that necessity into a clean, shared abstraction rather than a
   one-off.
2. **Event/`SpriteDatabase` crossing** — `repr(C)`/wire-format mirror types, not pointer-passing.
   Pointer-passing was only ever viable for a shared-address-space FFI boundary; it doesn't mean
   anything across a process boundary, so this is now the only option, not just the more-thorough one.
3. **Build-mechanics spike (dylib sharing)** — superseded. The mechanism it was scoped to validate
   (Bevy-style `dynamic_linking` trick for sharing `surge-core` as one dylib) is no longer part of the
   design, since dev-mode modules no longer share the player's process at all. The validation need shifts
   to the IPC round-trip instead — see new open question 8.
4. **Reload lifecycle** — process-per-module: kill + respawn the child, reconnect the socket. No
   `dlclose` anywhere in the design, so the earlier "never `dlclose`, leak forever" fallback isn't needed.
5. **"Single binary blob" scope** — `config.toml` and `assets/` stay external files read from CWD/disk in
   both modes, matching today's contract. Not embedded into the binary.
6. **`declare_surge_app!`-style macro** — defer. Hand-write the (now-different, IPC-oriented) boilerplate
   for `surge-mod-2048` and `surge-mod-default`; revisit once a 3rd module makes the duplication real
   pain.
7. **`stager/stager.py` rewrite** — not a design decision, just a flagged follow-up. Unaffected by the
   process-isolation pivot beyond confirming release staging is still "copy one per-module binary +
   config + assets" — `stager/stager.py:80-131` needs a real rewrite, not a patch, to stop staging a
   player-exe-plus-`.so` pair.
8. **IPC transport** — Unix domain socket, **spiked and validated**. A throwaway two-binary harness
   (player spawns module as a real child process, length-prefixed `postcard` framing over a
   `UnixListener`/`UnixStream` pair, 32 `InstanceInfo`-shaped instances + 2 input events per frame,
   matching `max_sprites` in `surge-player/src/main.rs`) was built and measured, 20,000 warmed-up
   round trips per run:

   | build   | machine state        | p50    | p95    | p99.9   | max     |
   |---------|-----------------------|--------|--------|---------|---------|
   | release | idle                  | 6 us   | ~8 us  | ~24 us  | ~250 us |
   | debug   | idle                  | 46 us  | 140 us | 286 us  | 912 us  |
   | release | 32 cores saturated    | 12 us  | 19 us  | 2820 us | 2834 us |
   | debug   | 32 cores saturated    | 96 us  | 103 us | 122 us  | 259 us  |

   At a 144 Hz frame budget (~6944 us), every one of these is a single-digit-percent tax at p95 or
   better — the socket is not the bottleneck. The one caveat worth keeping honest: under heavy CPU
   contention the *release* build showed a rare (~1-in-1000) multi-millisecond tail (scheduler
   preemption between the two processes, not the transport itself), big enough to matter at higher
   refresh rates/tighter budgets (e.g. ~40-70% of a single frame at 240 Hz) even though it barely
   registers at 144 Hz. This is exactly the case the chosen backpressure policy (open question 10:
   block, with loud logging) is built to make visible rather than silently eat, so no design change
   needed — just noting it's a real, occasionally-observed effect, not a hypothetical one. Shared memory
   is not worth the added complexity given these numbers.
9. **Wire serialization format** — [`postcard`](https://docs.rs/postcard): a small binary
   `serde`-based crate built for exactly this shape of payload (small, fixed-size, no dynamic
   allocation needed), lower overhead than `bincode` and far less manual/error-prone than hand-rolled
   `repr(C)` framing.
10. **Backpressure/latency policy** — block, same as today's synchronous in-process `app.update()` call
    (no behavior change from what exists now), but pair it with clear, loud debug logging when the
    player is blocked waiting on the module (e.g. "waiting on module response — Nms and counting") so a
    stalled module reads as an obvious, diagnosable state during a dev session rather than a silent
    freeze indistinguishable from a player-side hang.
11. **Process supervision** — surface a distinct crash state. If the child's connection dies outside a
    deliberate reload trigger, the player stops driving that module, logs it loudly, and makes the crash
    visibly different from normal reload churn (e.g. freeze last frame + on-screen/log indicator) rather
    than silently auto-respawning and potentially masking a real crash loop as routine hot-reload
    activity.
12. **Windows equivalent** — out of scope per the Non-goals above (this doc is Linux-first). Flagging
    explicitly, not deciding: "process spawn + local IPC" is a concrete mechanism with real OS-specific
    API differences (named pipes vs. Unix domain sockets, process spawn/kill semantics) once it's
    actually built for Windows, not just an abstract TLS claim scoped to Linux/glibc. No action needed
    now.

### Added in pass 2

13. **`SurgeApp` trait signature change** — resolved. Minimal scope: `&mut dyn SpriteCanvas` replaces
    `&mut SpriteDatabase` (structurally required — see "Blocking issues" discussion folded into the
    `surge-sdk` section above); a `surge_sdk::KeyEvent` mirror struct replaces `winit::event::KeyEvent`
    (structurally required — winit's `KeyEvent` is not constructible outside the `winit` crate);
    `MouseButton`/`MouseScrollDelta`/`ElementState`/`TouchPhase` stay as winit's own types since they're
    safely constructible and there's no forcing reason to wrap them. **Correction, pass 4:** this item
    originally also listed `DeviceId` among the types kept as-is; that's wrong — see resolved open
    question 19, `DeviceId` doesn't derive `Serialize`/`Deserialize` under winit's `serde` feature on any
    platform, so it's dropped from the wire entirely rather than kept.
14. **Reload GPU-state reset** — resolved. Every reload rebuilds the player's `SpriteDatabase` from
    scratch, reusing the existing swapchain-recreation code path, before replaying the fresh module's
    `on_load`. Without this, `uploaded_textures`' 32-slot cap (no clear/reset method exists today) gets
    blown after a handful of reloads, breaking hot reload — dev mode's entire purpose — almost
    immediately.
15. **Dev-mode module discovery / config schema** — resolved. `[startup_app] app_folder`/`app_name` are
    reinterpreted (not replaced) to resolve to the standalone module binary's path instead of a `dlopen`
    target. Same config shape in both modes.
16. **Wire protocol message shape** — resolved in pass 3 (see "Message shape" under Dev mode above): an
    explicit `Request { OnLoad, OnSwapchainRecreate, Frame, Shutdown }` / `Response { Handshake,
    CanvasOps, ShutdownAck }` enum pair, with `on_load`/`on_swapchain_recreate`/`Frame` all replying with
    the same `CanvasOps` payload shape rather than three bespoke ones, and `on_unload` folded into
    `Shutdown`/`ShutdownAck` rather than getting its own round trip.
17. **`texture_id` upload-failure handshake** — resolved in pass 3 (see "`texture_id` handshake" under
    Dev mode above). Turned out not to be a new IPC problem: `SpriteDatabase::upload_decoded_texture`
    already returns `Ok(())` instead of the assigned index and silently no-ops past `max_textures` in
    today's single-process code, and neither existing module reads a returned ID — both already hardcode
    `texture_id` from upload order. Fix at the source instead of adding wire-protocol reconciliation:
    (a) `upload_texture` changes to `Result<usize, VulkanError>` with a real `Err` on overflow instead of
    a silently-successful `Ok(())`; (b) the player treats any `CanvasOps.texture_uploads` failure as a
    module fault, routed through the same crash-state path open question 11 already defines for a dead
    connection — no new wire message, no partial-desync reconciliation logic.
18. **Orphaned child process on player crash** — resolved. Deliberate reload (kill+respawn) was already
    covered; a player panic or external kill before `exiting()` runs was not. Fix: `PR_SET_PDEATHSIG` on
    the child at spawn time (Linux-only, consistent with this doc's Linux-first scope) so the kernel
    reclaims it if the player dies for any reason.

### Added in pass 4

19. **`DeviceId` can't cross the wire as originally proposed** — resolved. Pass 2's open question 13
    concluded `DeviceId` needed no SDK-owned mirror because it's "safely constructible outside winit,"
    which is true but isn't the same claim as "serializable." Checked directly against winit 0.30.12's
    source: `ElementState`, `MouseButton`, `MouseScrollDelta`, and `TouchPhase` all derive
    `Serialize`/`Deserialize` under winit's `serde` feature; `DeviceId` — neither the public wrapper nor
    either Linux backend's inner type (`x11`'s `xinput::DeviceId`, `wayland`'s unit struct) — does not,
    on any platform. `InputEvent` as drafted in pass 3 would not compile. Fix: drop `DeviceId` from the
    wire format entirely rather than mirroring it — neither existing module reads it for anything, so
    `surge_sdk::run_dev_module` just reconstructs `DeviceId::dummy()` locally when replaying an
    `InputEvent` back into the concrete app's event-handler methods.
20. **Which crate hosts `impl SpriteCanvas for SpriteDatabase`, and where does `RemoteApp` live** —
    resolved. Left as an unpinned "`surge-player` or `surge-core`" in pass 3; that phrasing hid a Rust
    orphan-rule constraint (`impl SpriteCanvas for SpriteDatabase` needs to live in the crate owning the
    trait or the type — `surge-player` owns neither, so it structurally cannot host that impl, no
    ambiguity possible) and a release-mode goal violation risk (if `RemoteApp`/the socket code landed in
    `surge-core`, every release binary — which always links `surge-core` as a plain `rlib` — would
    statically compile in `postcard` and the socket/spawn machinery it never uses, contradicting this
    doc's own "no IPC ... compiled in" release goal). Resolved: `impl SpriteCanvas for SpriteDatabase`
    lives in `surge-core` (the only legal option that keeps `surge-sdk` free of Vulkan knowledge);
    `RemoteApp` and all socket/spawn/kill code live in `surge-player` (no orphan-rule issue — it's a new
    type, not a foreign-trait-for-foreign-type case — and it keeps that code out of every release
    binary). This gives `surge-core` a new dependency on `surge-sdk` that doesn't exist today; see the
    "New dependency edge" note under `surge-sdk` above for the full acyclic graph.
21. **Fate of the `hot_reloading` Cargo feature** — resolved: **deleted**, not repurposed. Pass 3 still
    framed dev mode as "the `hot_reloading` feature" as if it selected between two behaviors of one
    `surge-player` binary, but release mode was simultaneously defined as a wholly separate binary target
    living on each *module* crate that never touches `surge-player`'s own `[[bin]]`. Under that shape
    `surge-player`'s binary has exactly one behavior left, so there's nothing left for a feature flag to
    select between. Running the `surge-player` binary *is* dev mode now, unconditionally.
22. **Reload racing an in-flight, blocked `Request::Frame`** — resolved. Q10 (block, synchronously) and
    Q11 (an unexpected connection death is a crash) were each individually sound but never checked against
    each other: a deliberate reload's `SIGTERM`/`SIGKILL` can land while the main thread is blocked inside
    exactly the socket read Q10 describes, producing the same EOF/broken-pipe failure Q11 treats as a
    crash. Fix: a `reload_in_progress` flag, set before the child is signaled and consulted by the main
    thread before it decides whether a read failure means "crash" or "expected, I did this" — pure
    sequencing on top of already-specified machinery, no new component.
23. **Unix socket path lifecycle** — resolved. Not specified in earlier passes. A fixed path fails to
    `bind()` on a stale file from a prior crashed run and collides across concurrent player instances.
    Fix: use Linux's abstract socket namespace (`SocketAddr::from_abstract_name`, stable via
    `std::os::linux::net::SocketAddrExt`) keyed on the player's PID — kernel-lifetime-scoped, never
    touches the filesystem, no stale-file cleanup or collision handling needed.
24. **The module-side dispatch loop was never named** — resolved. Pass 3 fully specified the player-side
    `RemoteApp` and the *shape* of the module-side buffering proxy, but not the loop that actually drives
    it: connect, send the unprompted `Handshake`, then per `Request` replay `input_events` into the
    concrete app's event-handler methods, call the matching lifecycle method, and ship back whatever the
    `SpriteCanvas` proxy buffered. Named and placed: `surge_sdk::run_dev_module(app: Box<dyn SurgeApp>)`,
    symmetric to `surge-player`'s `run()`, so every module's dev `main()` is one line.
25. **Vestigial `surge-player` → `surge-mod-default` dependency** — resolved: delete it as part of this
    work. Already flagged as vestigial in `CLAUDE.md` under today's dynamic-loading design; this design
    removes the last excuse for it, since `surge-player`'s binary never statically knows about a specific
    module in either mode.

### Added in pass 5

26. **`CanvasOps`'s payload types don't compile as wire types, and are homed in the wrong crate** —
    resolved. `InstanceInfo` (`surge-core/src/vulkan/sprite_database/mod.rs:93`, `#[derive(Debug)]` only)
    and `SubTextureInfo` (same file, also `Debug`-only) are what `CanvasOps.instances: Vec<InstanceInfo>`
    has to carry across `postcard`, but neither derives `Serialize`/`Deserialize`, and their fields
    (`nalgebra::Vector2<f32>`, `Vector4<f32>`, `Vector2<u32>`) don't either — nalgebra only implements
    those under its `serde-serialize` Cargo feature (checked against nalgebra 0.34.1's `Cargo.toml`),
    which isn't enabled anywhere in this workspace today (`surge-core/Cargo.toml`, and each module's own
    separate `nalgebra` dependency). Pass 3's `CanvasOps` draft would not compile as written, for the same
    class of reason Q19 caught for `DeviceId` — a payload type checked against real derives, not assumed.
    Worse: even fixed, these types can't stay where they are. They're defined in
    `surge-core::vulkan::sprite_database`, and dev-mode modules are explicitly barred from depending on
    `surge-core` at all (it pulls in `ash`/`vk-mem`) — yet both real modules construct these types
    directly today (`surge-mod-2048/src/piece.rs:34`, `surge-mod-default/src/lib.rs:43`), so a
    `SpriteCanvas`-based module still has to be able to name and build an `InstanceInfo` value, `surge-core`
    dependency or not. Fix: `InstanceInfo`, `SubTextureInfo`, `CreateInfo`, `BlendingMode`,
    `TextureFilteringMode`, and `TextureFilteringLevel` move to `surge-sdk` outright (see the `surge-sdk`
    section above for the full resolution, including the nalgebra-version-skew consequence for modules'
    own `Cargo.toml`s).
27. **`get_create_info()` has no wire mechanism at all, and it's called every frame, not just at
    startup** — resolved. `SpriteCanvas`'s sketched surface includes `get_create_info`, but the
    `Request`/`Response` enum pair only supports the player pushing a `Request` and the module replying
    once with a `CanvasOps` — there is no direction for the module to *ask* the player something mid-turn
    and block on an answer before it can finish computing that turn's `CanvasOps`. Not a rare corner:
    `surge-mod-2048/src/lib.rs:62-66` calls `spdb.get_create_info().window_width/height` from
    `add_board_sprite`, which `update()` calls (`lib.rs:93`) — every `Frame`, not just `on_load`. Fix:
    since `CreateInfo` is fully determined by the player ahead of time, push it instead of pulling it —
    `Request::OnLoad`/`Request::OnSwapchainRecreate` gain a `create_info: surge_sdk::CreateInfo` field
    (see the updated `Request` enum under "Message shape"), and the dev-mode `SpriteCanvas` proxy caches
    it from whichever it saw most recently, answering `get_create_info()` as a pure local read from then
    on with no round trip, ever.
28. **`SpriteCanvas::upload_texture`'s error type can't be `VulkanError`** — resolved. Q17's fix changes
    `SpriteDatabase::upload_texture` (in `surge-core`) to `Result<usize, VulkanError>` — sound on its own
    — but `SpriteCanvas` is the trait module code actually calls, and it lives in `surge-sdk`, which this
    doc insists stays free of Vulkan knowledge. Naming `VulkanError` in the trait signature would force
    `surge-sdk` to depend on `surge-core`, contradicting the acyclic graph Q20 established. Fix: a
    dedicated `surge_sdk::CanvasError` (see the `surge-sdk` section above for the full resolution) and
    the dev/release asymmetry it's worth documenting on the trait method itself: the dev-mode proxy can
    never actually produce this `Err`, since upload failures there are only discoverable after the fact,
    player-side, and routed through Q11's crash path instead. **Correction, pass 6 (Q34):** this item
    originally also required the `VulkanError` → `CanvasError` conversion to be a hand-written
    `.map_err(...)` mapping rather than a `From` impl, reasoning the latter would violate the orphan
    rule; that reasoning was wrong — see Q34, a plain `From` impl compiles fine and is what the design
    now uses.
29. **Deleting `load_from_dylib` leaves dead code and a contradicted release goal behind** — resolved.
    "Release mode" says `LoadedApp`/`load_from_dylib` are deleted outright but doesn't trace the
    consequence through: `AppError` (`surge-core/src/errors/mod.rs`) exists solely to type
    `load_from_dylib`'s errors and becomes fully dead code, and `libloading` — an unconditional, top-level
    dependency of `surge-core/Cargo.toml` itself, not `surge-player`'s — becomes fully unused, directly
    contradicting this doc's own release-mode goal ("no `libloading` dependency compiled in") since
    `surge-core` links into every release binary. Fix: delete `surge-core/src/app/mod.rs` in full, delete
    `AppError`, remove `libloading` from `surge-core/Cargo.toml` (see "Release mode" above for the full
    note, including the already-dead `ModuleError` enum this incidentally explains).
30. **`Child::kill()` is `SIGKILL`, not `SIGTERM`** — resolved. Both the reload and shutdown sequences
    assume `SIGTERM` is sendable, but `std::process::Child::kill()` on Unix always sends `SIGKILL` — there
    is no `std` API for `SIGTERM` specifically. Built against `Child::kill()` naively, the "graceful,
    wait-then-escalate" story silently degrades to "always hard-kill immediately" on every reload, not
    just the timeout case. Fix: no new dependency — the `libc`/`nix` crate already flagged for
    `PR_SET_PDEATHSIG` (Q18) also provides `kill(pid, SIGTERM)`; worth stating both uses together (see
    "Orphaned child on player crash" above) so an implementer doesn't reach for `Child::kill()` for the
    graceful half and get a working-looking but silently-wrong result.
31. **"`RemoteApp` ... forwards each trait method" overstates what the wire protocol supports** —
    resolved. Only `on_load`/`on_swapchain_recreate`/`update` have a matching `Request` variant and
    actually round-trip; `keyboard_event`/`mouse_button_event`/`mouse_wheel_event` have none, and instead
    buffer into a local `Vec<InputEvent>` with no IPC, flushed into the next `Request::Frame` when
    `update` triggers a real round trip. Fix: reworded under "Message shape" above so an implementer
    doesn't read "each trait method" literally and try to add wire variants for the three event methods
    (which would be far chattier for no benefit `Frame.input_events` doesn't already provide).
32. **`init_env_logger`/`resolve_asset_path` are called by every module today but live in `surge-core`,
    which modules can no longer depend on** — resolved. Both existing modules call
    `surge_core::cli::init_env_logger()` and `surge_core::assets::resolve_asset_path()` directly
    (`surge-mod-2048/src/lib.rs:50,82`, `surge-mod-default/src/lib.rs:24,31`); both currently live in
    `surge-core`, which the "modules depend on `surge-sdk`, not `surge-core`" rule now forbids. Fix: both
    move to `surge-sdk` (see the `surge-sdk` section above for the full resolution, including the
    `resolve_asset_path`/`current_exe()` behavior change this move is entangled with, and why it's
    correct rather than a regression).

### Added in pass 6

33. **`surge-sdk`'s proposed types, actually compiled and round-tripped** — resolved (this is the spike
    pass 5's own self-critique asked for). A throwaway two-member Cargo workspace was built and thrown
    away after: `sdk-spike` (standing in for `surge-sdk` — zero dependency on anything `surge-core`-shaped,
    depends only on `nalgebra` with `serde-serialize`, `serde`, `postcard`, and `winit` with its `serde`
    feature) and `core-spike` (standing in for `surge-core`, with a `path` dependency on `sdk-spike` — the
    same edge Q20 established), both built against this workspace's actual pinned `winit 0.30`/`nalgebra
    0.34` versions (Cargo resolved `winit 0.30.13`/`nalgebra 0.34.2` — compatible patch releases of what
    `surge-core/Cargo.toml` pins today). What it covered and confirmed, all by actually running `cargo
    test`, not by reasoning about it:
    - `BlendingMode`, `TextureFilteringMode`, `TextureFilteringLevel` (including its non-contiguous
      explicit discriminants, `X0 = 0` through `X16 = 16`), `CreateInfo`, `SubTextureInfo`, `InstanceInfo`
      all derive `Serialize`/`Deserialize` and round-trip through `postcard::to_stdvec`/`from_bytes`
      cleanly with `nalgebra`'s `serde-serialize` feature on, including the `Option<SubTextureInfo>` field
      and nested `Vec<InstanceInfo>` inside `CanvasOps` (Q26).
    - The full `Request`/`Response`/`CanvasOps`/`InputEvent` enum graph from "Message shape" compiles as
      drafted and round-trips a `Request::Frame` carrying all three `InputEvent` variants, a
      `Request::OnLoad` carrying `CreateInfo` (Q27), and `Response::Handshake`/`ShutdownAck`/`CanvasOps`.
    - `winit::event::{ElementState, MouseButton, MouseScrollDelta, TouchPhase}` and
      `winit::keyboard::{PhysicalKey, Key, KeyLocation}` all derive `Serialize`/`Deserialize` under
      winit's `serde` feature as claimed, including specifically exercising `Key::Character(SmolStr)`
      (not just the simpler `Key::Named` variant used in earlier examples) to confirm `smol_str`'s own
      `serde` feature is actually reachable transitively, not just assumed reachable.
    - `winit::event::KeyEvent` is confirmed not publicly constructible outside `winit` (its
      `platform_specific` field is `pub(crate)`, verified directly in winit 0.30.12's source) — the
      structural reason `surge_sdk::KeyEvent` has to exist at all (Q13, Q19).
    - `DeviceId::dummy()` is a public `const fn`, confirmed callable (the mechanism Q19's fix depends on).
    - `&mut dyn SpriteCanvas` is object-safe as drafted — `core-spike`'s `FakeSpriteDatabase` was used
      through a `&mut dyn SpriteCanvas` reference, not just a concrete type, matching how `SurgeApp`'s
      methods actually take it.
    - `impl SpriteCanvas for FakeSpriteDatabase`, living in the `surge-core`-shaped crate, returns a
      genuine `Err(CanvasError)` once a fixed `max_textures` cap is hit and `Ok(usize)` (the assigned,
      positional index) otherwise — the actual fallible/positional contract Q17 and Q28 describe, checked
      against a real two-crate boundary rather than asserted.
    - `std::os::linux::net::SocketAddrExt::from_abstract_name` + `UnixListener::bind_addr`, the mechanism
      Q23's fix depends on, compiles and successfully binds on this machine's stable `rustc` (1.96.1).
    - `libc::prctl(PR_SET_PDEATHSIG, ...)` (Q18) and `nix::sys::signal::kill(pid, Signal::SIGTERM)` (Q30)
      both compile and run against `nix 0.29`.
    One thing this spike deliberately did *not* attempt: proving a negative (that some particular `impl`
    fails to compile) via `trybuild` or similar — it's a throwaway spike, not a test suite meant to ship.
    Where a "this shouldn't compile" claim mattered, it was checked by literally trying it, once, by hand
    (see Q34, where doing exactly that is what caught the bug).
34. **The orphan-rule reasoning behind hand-writing the `CanvasError` conversion was wrong** — resolved.
    Pass 5 (Q28) asserted `impl From<VulkanError> for CanvasError` inside `surge-core` "would itself be an
    orphan-rule violation, since neither `From` nor `CanvasError` is local to that crate," and had
    `surge-core` do the conversion with a hand-written `.map_err(...)` closure instead. Compile-spiked in
    `core-spike` (Q33) by literally writing that `impl From<...> for ...` block and building it: **it
    compiles.** The reasoning was incomplete, not just unverified — Rust's orphan rule for `impl<P...>
    ForeignTrait<T1..Tn> for T0` requires *at least one* of `T0..Tn` (scanning left to right) to be local,
    with no uncovered impl-generic parameter appearing before that first local type; it does not require
    `T0` (the `for` type) specifically to be local. `From<VulkanError>`'s one type parameter — `T1 =
    VulkanError` — *is* local to `surge-core`, and the impl has no generic parameters of its own to worry
    about being "uncovered," so the rule is satisfied regardless of `CanvasError` (`T0`) being foreign.
    This is the same rule as the far more commonly-seen `impl From<std::io::Error> for MyLocalError`
    pattern, just with the local/foreign types in the trait's argument position instead of the `for`
    position. Fix: the design is simplified back to the obvious form —
    `impl From<VulkanError> for CanvasError` in `surge-core`, with `?` at the `upload_texture` call site
    instead of a hand-rolled `.map_err(...)` closure (see the `CanvasError` bullet under `surge-sdk`
    above, and Q28's correction note). No behavior change; this is strictly less code once the reason
    given for avoiding it turned out not to hold. Worth flagging as a process point, not just a content
    fix: this is the second time (after Q19) a "this can't work, here's the Rust rule why" claim in this
    doc turned out to be the thing that was actually wrong, not the design — a reminder that "checked
    against the actual language/library semantics" has to mean *actually checking*, including re-deriving
    rules from their real definition rather than restating a remembered version of them, even inside a
    pass whose whole premise is being the harsh critic.
35. **`surge_sdk::KeyEvent`'s field list was never written down** — resolved. Every pass since pass 2
    (open question 13) referenced "a `surge_sdk::KeyEvent` mirror struct" by name and cited the structural
    reason it has to exist, but none ever specified its fields — an implementer would have had to
    reverse-engineer them from winit's `KeyEvent` at implementation time. Pinned down and compile-spiked
    (Q33): winit's own six public fields (`physical_key`, `logical_key`, `text`, `location`, `state`,
    `repeat`), with `text: Option<SmolStr>` mirrored as `text: Option<String>` — a deliberate
    simplification to avoid a direct `smol_str` dependency in `surge-sdk` for one field, not a forced
    change (`smol_str`'s `serde` feature was also confirmed reachable transitively through winit's, so
    `Option<SmolStr>` would have worked too). See the `keyboard_event` bullet under `surge-sdk` above for
    the full struct definition.

### Added in pass 7

36. **`DecodedTexture`/`decode_texture_file`/`upload_decoded_texture` have no home in `SpriteCanvas`** —
    resolved. `surge-mod-2048` is the one real caller neither pass 5 nor pass 6 traced: `decode_image_assets`
    (`surge-mod-2048/src/lib.rs:31-54`) calls `SpriteDatabase::decode_texture_file` once in `App2048::new()`,
    caches the result in `cached_textures: Vec<DecodedTexture>` (`app_2048.rs:31`), and `upload_image_assets`
    (`lib.rs:56-60`) replays it through `upload_decoded_texture` on both `on_load` and every
    `on_swapchain_recreate` — an optimization to avoid re-reading/re-decoding 12 PNGs from disk on every
    swapchain rebuild. None of `DecodedTexture`, `decode_texture_file`, or `upload_decoded_texture` appear
    anywhere in this doc's `SpriteCanvas` sketch or the Q26 type-migration list, and they structurally
    can't: dev-mode modules never see decoded pixel bytes at all under the "asset paths, not pixel bytes"
    wire design ("Message shape" above), so a module-side decode cache isn't just unlisted, it's impossible
    to keep in dev mode regardless of what `SpriteCanvas` exposes. Fix: retire the caching path.
    `surge-mod-2048` drops `cached_textures`/`decode_image_assets`/the `DecodedTexture` import and calls
    `SpriteCanvas::upload_texture(path)` per asset name directly in both `on_load` and
    `on_swapchain_recreate`, matching `surge-mod-default`'s existing pattern (`surge-mod-default/src/
    lib.rs:22-27`) exactly. `decode_texture_file`/`upload_decoded_texture`/`DecodedTexture` stay as
    `surge-core`-internal implementation details of `SpriteDatabase::upload_texture`, which already calls
    them internally (`sprite_database/mod.rs:634-636`) — they're simply not part of `SpriteCanvas`'s surface
    and not exported from `surge-sdk`. Net effect: `surge-mod-2048` re-reads and re-decodes 12 small PNGs
    from disk on every swapchain recreate instead of caching decoded bytes — a real behavior change, but
    swapchain recreation is resize-triggered and rare, and the *player*-side `SpriteCanvas` backend was
    always going to re-decode from an asset-path string regardless of what the module does, in both modes.
    Worth flagging as a data point in its own right: this is the same class of bug pass 4–6 kept finding
    (an assumption checked against prose, not every real call site) turning up again in a pass whose
    explicit job was checking against real call sites — grep *breadth*, not methodology, was the gap this
    time (see the "Added in pass 7" note under "Open questions" above).
37. **Release `[[bin]]` target dependency shape, and the "no feature flags" framing it costs** — resolved,
    user-confirmed (see "Implementation plan," Phase 4). Cargo has no way to scope a dependency to one
    `[[bin]]` target only; `surge-player` (needed for `run()`) in a module crate's plain `[dependencies]`
    would make even `cargo build -p surge-mod-2048`'s *dev* target compile `surge-player` → `surge-core` →
    `ash`/`vk-mem`/`winit` transitively, reintroducing exactly the Vulkan-in-dev-build coupling this doc's
    dev/release split exists to avoid. Fix: each module crate declares `surge-player` as `optional = true`,
    gated by a `release-bin` feature (`[features] release-bin = ["dep:surge-player"]`), and the release
    `[[bin]]` target sets `required-features = ["release-bin"]`. Considered and rejected: a separate sibling
    crate per module (e.g. `surge-mod-2048-release`) keeps the game-logic crate's `Cargo.toml` flag-free at
    the cost of doubling the crate count; accepting the compile-time cost outright was rejected as a
    permanent iteration-speed regression, not a one-time one. This does put a Cargo feature flag back into
    the design — see the retitled "Release mode" heading above for what it does and doesn't contradict (Q21
    is unaffected: that flag lives on module crates, not `surge-player`, and selects a build *target*, not a
    runtime mode of an already-built binary).
38. **Cutover scaffolding: dlopen → process migration switch** — resolved, user-confirmed (see
    "Implementation plan," Phase 3). Flipping `surge-player::main()`'s `startup_app` construction from
    `load_from_dylib` to spawn+IPC, and porting both modules' entry points, has no single-crate-at-a-time
    path: `main()` picks exactly one construction strategy, so testing "`surge-mod-2048` ported,
    `surge-mod-default` not yet" requires *some* way to select the strategy per run. Fix: a temporary
    `SURGE_DEV_LOADER` environment variable (`process` selects the new path; anything else, including
    unset, keeps today's `load_from_dylib`), added in one commit and deleted in the commit that completes
    the cutover, once both modules have a working dev `[[bin]]` target and `load_from_dylib`/`LoadedApp`
    are deleted outright. Never appears in `config.toml`, never documented as a supported mode, never
    shipped. Does not reopen Q21: Q21 killed a *permanent*, *shipped* mode-selecting feature; this is
    throwaway migration scaffolding with its own deletion commit already scheduled.
39. **Module-side socket handoff — how does the spawned child learn its socket name** — resolved. Q23 fixed
    the abstract-namespace *naming* scheme (PID-keyed) but never specified how that name reaches the child
    process, since the player picks it before spawn (per the bind-before-spawn ordering already
    established) and the module is a separate process that can't read the player's memory. Fix: pass it as
    `argv[1]` to the spawned module binary (`Command::arg`), not an environment variable — argv is directly
    visible in `ps`/`/proc/<pid>/cmdline` during debugging, and avoids namespacing a `SURGE_`-prefixed env
    var against whatever a module's own process environment might already use. `run_dev_module`'s connect
    step reads `std::env::args().nth(1)`.
40. **The reload mechanism this doc specifies has no trigger, and therefore no way to be exercised at all
    within this doc's own scope** — resolved, user-confirmed (see "Implementation plan," Phase 3).
    "Designing the actual hot-reload trigger" is an explicit non-goal (file watching, debouncing), the
    right scope call for a *design* doc, but taken literally it means the kill/respawn/reconnect sequence,
    the `reload_in_progress` flag, and the forced `SpriteDatabase` rebuild would all ship as dead code with
    zero manual or automated way to run even once. Fix, scoped to implementation only, not the design: a
    throwaway dev-only keybind (F5) in `surge-player`'s dev binary that calls the same `RemoteApp::reload()`
    a real file-watching trigger would eventually call. Explicitly not a feature — no config entry, not
    documented as user-facing, deleted or replaced outright whenever a real trigger doc lands.

### Added in pass 8

41. **Step 1.6, as pass 7 wrote it, is a Cargo dependency cycle** — resolved. Pass 7's 1.6 moves the
    `SurgeApp` trait to `surge-sdk` with its `on_load`/`on_swapchain_recreate`/`update` signatures
    "unchanged" — still `&mut SpriteDatabase` — and defers the `&mut dyn SpriteCanvas` retyping to a
    separate step, 1.7. But `SpriteDatabase` is a `surge-core` type
    (`surge-core/src/vulkan/sprite_database/mod.rs:150`), and step 1.3, three steps earlier in the same
    phase, already gives `surge-core` a `path` dependency on `surge-sdk` for the six plain-data types. A
    trait defined in `surge-sdk` that names `SpriteDatabase` in a method signature would need `surge-sdk`
    to depend on `surge-core` right back — Cargo does not allow cyclic package dependencies in a
    workspace, so `cargo check -p surge-sdk` fails the moment 1.6 is written this way, not just "reads
    awkwardly." This directly contradicts the acyclic graph open question 20 spent real effort
    establishing (`surge-sdk` → nothing internal; `surge-core` → `surge-sdk`; never the reverse) — pass 7,
    while checking every call site a module makes, didn't check the dependency-graph consequence of its
    own step ordering, a different blind spot again from Q19/Q26/Q36's (those were "does this type/call
    exist," this is "does this *sequence* of otherwise-individually-fine steps stay acyclic at every
    intermediate commit"). Fix: 1.6 and 1.7 are not actually separable — merge them into one commit. The
    moment the trait relocates to `surge-sdk`, its `SpriteDatabase`-shaped parameters are *forced* to
    become `&mut dyn SpriteCanvas` in the same commit (which is itself already defined in `surge-sdk` as of
    1.4, so no cycle there). This does **not** force `winit::event::KeyEvent`/`DeviceId`/`ElementState`/
    `MouseButton`/`MouseScrollDelta`/`TouchPhase` to move in the same commit — `winit` is an ordinary
    external-registry dependency, not a workspace member, so `surge-sdk` naming those types directly
    carries no cycle risk regardless of what `surge-core` depends on. So the split pass 7 actually wanted —
    isolate "which crate owns the trait" from "what the trait's methods look like" — still holds for the
    `KeyEvent` half (that stays its own step, unchanged from pass 7's 1.8); it just doesn't hold for the
    `SpriteDatabase`/`SpriteCanvas` half, which was never actually splittable once 1.3 landed. See
    "Implementation plan" below, where 1.6/1.7 are merged and pass 7's 1.8 is renumbered 1.7.
42. **`surge-player`'s own `init_env_logger` call site is orphaned by the Q32 move** — resolved. Step 3.2
    moves `init_env_logger`/`resolve_asset_path` from `surge-core::cli`/`surge-core::assets` to `surge-sdk`
    because modules need them without a `surge-core` dependency (Q32) — but `surge-player/src/main.rs:332`
    calls `sc::cli::init_env_logger()` too, for the player's own logging, and `surge-player` has no
    `surge-sdk` dependency anywhere in the plan (1.6's `pub use surge_sdk::SurgeApp;` re-export in
    `surge-core::app` covers the *trait*, but `init_env_logger` isn't re-exported by anything once it
    physically leaves `surge-core`). Left as pass 7 wrote it, 3.2's commit deletes the function
    `surge-player/src/main.rs:332` calls, breaking the compile gate 3.2 itself requires. Fix: 3.2 adds a
    direct `surge-sdk` path dependency to `surge-player/Cargo.toml` in the same commit, and changes
    `main.rs:332`'s call site to `surge_sdk::init_env_logger()`. `sc::cli::print_logo()`
    (`main.rs:327`) is unaffected — it stays in `surge-core` per 3.2's existing text, since only
    `surge-player::main` calls it.
43. **`SurgeContext.startup_app`'s `LoadedApp` → `Box<dyn SurgeApp>` retyping has no commit of its own, and
    3.1 needs it immediately, not at 4.1** — resolved. The "Release mode" section describes "swapping the
    `startup_app` field's type from `LoadedApp` to `Box<dyn SurgeApp>`" as a small mechanical change,
    prose that (along with 4.1's step text) reads as if that retyping happens during the Phase 4 `[lib]`
    extraction. But step 3.1's `SURGE_DEV_LOADER` branch needs it four phases earlier: `main()` must
    construct `ctx.startup_app` from *either* `load_from_dylib(...)` (returns `LoadedApp`) *or*
    `spawn_and_connect(...)` (returns something `RemoteApp`-shaped) depending on an env var read at
    runtime, and a single struct field cannot hold two different concrete types — only a common type
    (`Box<dyn SurgeApp>`, or an enum wrapping both) can. Written the way pass 7 sequenced it, 3.1 doesn't
    actually compile: `SurgeContext { startup_app: sc::app::LoadedApp, .. }`
    (`surge-player/src/main.rs:21`) has no slot for a `RemoteApp`. Fix: 3.1 introduces a temporary
    `StartupApp` enum in `surge-player` — `enum StartupApp { Dylib(sc::app::LoadedApp), Remote(Box<dyn
    SurgeApp>) }`, with hand-written `Deref`/`DerefMut` to `dyn SurgeApp` (a two-arm `match`) so every
    existing `self.startup_app.<method>(...)` call site keeps working unchanged via deref coercion, exactly
    as it does today through `LoadedApp`'s own `Deref` impl. `SurgeContext.startup_app`'s type becomes
    `StartupApp` at 3.1, not `Box<dyn SurgeApp>` yet — that final collapse happens at 3.4 (see below),
    once the `Dylib` arm has nothing left to hold. This is scaffolding in the same spirit as Q38's env var:
    introduced for the transition window, deleted outright once the cutover completes, never touching
    `LoadedApp`'s own field-order invariant (it stays nested, untouched, inside the `Dylib` arm until 3.4
    deletes it wholesale). 4.1's text and the "Release mode" section are corrected to point here instead of
    implying the retyping is still pending at Phase 4 — by Phase 4, it's long done.
44. **`RemoteApp`'s outgoing `create_info` has no specified source** — resolved. `Request::OnLoad`/
    `Request::OnSwapchainRecreate` carry a `create_info: surge_sdk::CreateInfo` field (Q27's push-not-pull
    fix), but 2.5's `RemoteApp` description never says where the value it sends comes from. It doesn't need
    a new mechanism: `RemoteApp::on_load`/`on_swapchain_recreate` are `SurgeApp` methods, which per this
    doc's own signature already receive `&mut dyn SpriteCanvas` — the *real* backend, already constructed
    with the correct `CreateInfo` by `SurgeContext` before `on_load` is ever called
    (`surge-player/src/main.rs`'s `resumed`/`about_to_wait` both build the real `SpriteDatabase` first, then
    call `self.startup_app.on_load(...)`/`on_swapchain_recreate(...)`) — and `get_create_info()` is already
    part of `SpriteCanvas`'s surface. Fix: `RemoteApp` calls `canvas.get_create_info()` at the top of its
    `on_load`/`on_swapchain_recreate` impls and forwards the result as the outgoing `Request`'s
    `create_info` field. No new field, no new round trip, no module-side change — this is purely a gap in
    stating where an already-available value comes from. Folded into 2.5 below.
45. **The F5 reload keybind (Q40) has no way to reach `RemoteApp` through `Box<dyn SurgeApp>`, and no way
    to make `SurgeContext` rebuild the `SpriteDatabase`** — resolved. Q40 approved a throwaway F5 keybind
    calling `RemoteApp::reload()`, and 3.5 wires it up, but neither says *how* a keypress handled in
    `SurgeContext::window_event` (`surge-player/src/main.rs:295-300`, which only ever sees
    `self.startup_app` as `Box<dyn SurgeApp>`/`StartupApp` — see Q43 — never the concrete `RemoteApp`) is
    supposed to call a method that exists only on the concrete type. Worse, even with a way to call it,
    `RemoteApp::reload()` alone can't satisfy Q14's requirement ("the player's `SpriteDatabase` must be
    rebuilt on every reload") — `SpriteDatabase` is owned by `SurgeContext`, not by whatever implements
    `SurgeApp`, so nothing inside `RemoteApp` can set `SurgeContext`'s own `recreate_swapchain` flag no
    matter what it's called through. Fix: add one default trait method to `SurgeApp` (in `surge-sdk`):
    ```rust
    /// Dev-only hot-reload hook. Default no-op — release-mode, in-process apps have nothing to reload.
    /// Returns true if the caller must rebuild its canvas backend from scratch (the same rebuild a
    /// swapchain recreate already triggers), because the reload invalidated whatever GPU-side state the
    /// backend had accumulated for this app.
    fn reload(&mut self) -> bool { false }
    ```
    `RemoteApp::reload` overrides it: does the kill/respawn/reconnect/re-handshake sequence (Q4, Q22, Q23,
    Q30, Q38 — unchanged), then unconditionally returns `true`. `SurgeContext`'s F5 arm in `window_event`
    becomes `self.recreate_swapchain |= self.startup_app.reload();` — an ordinary trait-dispatch call
    through the same interface every other lifecycle method already goes through, keeping "the run loop
    only ever deals in `Box<dyn SurgeApp>`" true for this method too, not a special case. This is additive
    to `SurgeApp` (a new *default* method), so it doesn't reopen anything 1.6/1.7 already settled about that
    trait's other signatures.
46. **The F5 trigger, once 45 makes it work, still can't exercise the race Q22's fix is for** — resolved
    (an honesty fix, not a mechanism fix). Q22 added a `reload_in_progress` flag because a reload's
    `SIGTERM` could land *while the main thread is blocked* inside the socket read `Request::Frame`
    performs, and 3.5's smoke test description claims this is "the first and only point in this plan that
    actually exercises kill/respawn/reconnect/`reload_in_progress` at all." That overstates it: winit's
    `ApplicationHandler` calls `window_event` (where F5 is handled) and `about_to_wait` (where the blocking
    `Request::Frame` round trip happens) sequentially on one thread — never concurrently. A key press
    physically cannot be *handled* while `about_to_wait` is blocked inside a read; at best it's queued by
    the OS/winit and delivered on the next loop iteration, after the block has already returned. So F5, as
    specified, can only ever trigger a reload from a quiescent state, never from mid-`Frame`-round-trip —
    exactly the one case Q22 was written for is the one case this trigger structurally cannot produce.
    Nothing here is a design flaw: Q22's flag is still correct and still needed once a *real* trigger
    (necessarily async — a file-watcher thread, the actual future scope this doc excludes) exists. Fix:
    3.5's smoke-test claim is corrected to say what F5 actually proves (kill/respawn/reconnect/rebuild work
    in the non-racing case) and what it doesn't (`reload_in_progress` stays functionally unverified by
    anything in this plan, pending the real trigger this doc scopes out).
47. **Wire framing has no assigned step** — resolved. Open question 8's own spike used "length-prefixed
    `postcard` framing over a `UnixListener`/`UnixStream` pair," because a `UnixStream` is a byte stream
    with no built-in message boundaries — two back-to-back `postcard`-serialized values written to the
    socket can be coalesced or split arbitrarily on the read side without an explicit frame delimiter. Step
    2.3's `drive(app: &mut dyn SurgeApp, socket: impl Read + Write)` presupposes this framing (it has to
    read exactly one `Request` at a time off an arbitrary `Read`), but no step in Phase 2 assigns "write the
    length-prefix framing helpers" to a commit — 2.1 only defines the message *types*. Fix: 2.1 gains a
    `write_message`/`read_message` pair (a `u32` little-endian length prefix, matching Q8's spike) as
    `surge-sdk`-internal helpers, generic over `impl Write`/`impl Read`, with a `#[test]` round-tripping
    several back-to-back frames over a real `UnixStream::pair()` (not just a `Vec<u8>` buffer, so the test
    actually exercises stream-boundary behavior, not just serialization).
48. **Two small Cargo dependency/feature sequencing nits in Phase 1–2** — resolved. (a) Step 1.2's `#[test]`
    uses `postcard::to_stdvec`/`from_bytes`, but 1.2's own Cargo.toml-additions sentence lists only `serde`
    and `nalgebra`; step 2.1 then says "add postcard" as if introducing it for the first time. Fix: `postcard`
    moves into 1.2's dependency list (it's needed there regardless, for that step's own test), and 2.1's text
    changes to "postcard, already a dependency since 1.2." (b) Step 1.8 (renumbered 1.7 per Q41) defines
    `surge_sdk::KeyEvent` with `Debug` only — no `Serialize`/`Deserialize`, since Phase 1 is still `dlopen`,
    pre-IPC. Step 2.1's `InputEvent` enum embeds this same `KeyEvent`, which now needs those derives, and
    winit's own `ElementState`/`MouseButton`/`MouseScrollDelta`/`TouchPhase`/`PhysicalKey`/`Key`/
    `KeyLocation` need winit's `serde` Cargo feature turned on to have theirs — neither the retroactive
    derive nor the feature flip is mentioned anywhere in 2.1's text as pass 7 wrote it. Fix: 2.1 explicitly
    adds `#[derive(Serialize, Deserialize)]` to the `KeyEvent` struct from 1.7 and turns on `winit`'s
    `serde` feature in `surge-sdk/Cargo.toml`, both in the same commit that first needs them. (Folded in:
    Phase 0.2's new `VulkanError` variant for the "database full" case — implied by "a real `Err` once
    `max_textures` is hit" but never named — is called `VulkanError::TextureDatabaseFull` below, so the fix
    has an unambiguous shape rather than being left for an implementer to invent.)

## Implementation plan (pass 7, sequencing corrected pass 8)

The workspace has no test suite (per `CLAUDE.md`) and no CI gate beyond the compiler, so "tested" below
means one of two things, named once here and referenced by name after: **the compile gate**
(`cargo check --workspace`, then `cargo build --workspace` — every commit must pass both; a commit that
doesn't is not a valid stopping point, full stop) and the **standard smoke test** (`cargo run -p
surge-player` from the repo root; confirm the window opens, the active module's sprites draw as expected,
a keypress/mouse click still logs or acts as before, a resize triggers `on_swapchain_recreate` without a
visible glitch, and closing the window exits cleanly with no orphaned process left in `ps`). Individual
steps note only how they *differ* from these two defaults.

**The one finding that reshapes the whole sequence:** the design bundles two independent risks —
retyping `SurgeApp` (`&mut dyn SpriteCanvas`, the `KeyEvent` mirror) and moving modules into a separate
OS process (spawn, socket, IPC protocol) — under one architecture, but nothing requires doing them at the
same time. `SpriteCanvas`/`CanvasError`/the `KeyEvent` mirror can be built and cut over to *while modules
are still `dlopen`'d exactly as today* — `SpriteDatabase` just implements `SpriteCanvas` directly, in the
same process, and `surge-player`'s call sites pass `&mut SpriteDatabase` where `&mut dyn SpriteCanvas` is
now expected (an ordinary unsized coercion, no cast needed). Splitting these means the highest-risk trait
signature change is fully built, tested, and proven working end-to-end (real window, real sprites, real
input) *before* the second high-risk change (process isolation) is even started, instead of the two risks
compounding in one cutover. Phases 1 and 2 below are that split; Phase 3 is the (separately staged, per
Q38) process-isolation cutover.

### Phase 0 — Preliminaries (zero design risk, unblocked today)

- **0.1.** Delete the vestigial `surge-player` → `surge-mod-default` path dependency (Q25).
  `surge-player/Cargo.toml` only; `main.rs` never references the crate, so this is a one-line removal.
- **0.2.** Fix `SpriteDatabase::upload_texture`/`upload_decoded_texture`'s return type: `Result<(),
  VulkanError>` → `Result<usize, VulkanError>`, a real `Err` once `max_textures` is hit instead of a
  silently-successful `Ok(())` (Q17's source-level fix). The new failure needs a `VulkanError` variant of
  its own (Q48) — `TextureDatabaseFull` (no payload; `self.ci.max_textures` is already known at the call
  site if a message wants it). `surge-core` only — no module call-site changes needed, since both modules
  currently `.unwrap()` the result as a bare statement and discarding a `usize` compiles identically to
  discarding a `()`. This is explicitly flagged in "Self-critique / risks" above as worth landing as its
  own fix regardless of the rest of this doc; Phase 0 is where that happens.

*(Milestone: workspace still behaves exactly as today; both changes are invisible to a user running the
player.)*

### Phase 1 — `surge-sdk` exists; `SpriteCanvas`/`KeyEvent` land; still single-process, still `dlopen`

- **1.1.** Create the `surge-sdk` crate: `Cargo.toml`, an empty `lib.rs`. Add `"surge-sdk"` to
  `[workspace] members` explicitly — it is a root-level crate like `surge-core`/`surge-player`, not under
  `surge-modules/*`, so the existing glob does not pick it up.
- **1.2.** Move the six plain-data types (`BlendingMode`, `TextureFilteringMode`, `TextureFilteringLevel`,
  `CreateInfo`, `SubTextureInfo`, `InstanceInfo`) from `surge-core::vulkan::sprite_database` into
  `surge-sdk`, adding `#[derive(Serialize, Deserialize)]`; add `serde`, `nalgebra` (with `serde-serialize`),
  and `postcard` to `surge-sdk/Cargo.toml` (Q48 — `postcard` is used by this step's own test below, so it
  belongs here, not re-introduced at 2.1 as pass 7 had it). Add one `#[test]` round-tripping an `InstanceInfo` (with a
  `Some(SubTextureInfo)`) through `postcard::to_stdvec`/`from_bytes` — this is the first test anywhere in
  the workspace; `CLAUDE.md`'s "There are currently no tests anywhere in the workspace" line goes stale
  starting here and should be updated in the same commit. Test: `cargo test -p surge-sdk`.
- **1.3.** `surge-core` takes a `path` dependency on `surge-sdk`; the six local definitions in
  `sprite_database/mod.rs` become `pub use surge_sdk::{BlendingMode, TextureFilteringMode,
  TextureFilteringLevel, CreateInfo, SubTextureInfo, InstanceInfo};`. This is a pure compatibility-shim
  commit: `surge_core::vulkan::sprite_database::InstanceInfo` still resolves to the same path for existing
  module code, so **neither module's source changes in this step**. (nalgebra's `serde-serialize` feature
  needs no explicit enabling in `surge-core/Cargo.toml` — Cargo's feature unification turns it on
  workspace-wide once anything enables it, which `surge-sdk` now does.) Test: compile gate + smoke test on
  both modules, unchanged.
- **1.4.** Define `SpriteCanvas` and `CanvasError` in `surge-sdk` (additive; nothing implements or calls
  either yet). Include the doc-comment on `upload_texture` about the dev/release fallibility asymmetry (Q28)
  now, while the reasoning is fresh — it costs nothing to write before the dev backend exists. Test: compile
  gate.
- **1.5.** `impl SpriteCanvas for SpriteDatabase` in `surge-core`, plus `impl From<VulkanError> for
  CanvasError` (Q34's resolved simple form — plain `From`, `?` at the call site, no hand-rolled
  `.map_err`). Additive; nothing calls through the trait yet. Test: compile gate.
- **1.6.** Move the `SurgeApp` trait definition to `surge-sdk`, **and** retype `on_load`/
  `on_swapchain_recreate`/`update` to take `&mut dyn SpriteCanvas` instead of `&mut SpriteDatabase`, in the
  same commit — pass 7 originally split these into 1.6 (relocation, "signatures unchanged") and 1.7
  (retyping), but that split doesn't compile: `surge-core` already depends on `surge-sdk` as of 1.3, so a
  `surge-sdk`-hosted trait can never name `SpriteDatabase` (a `surge-core` type) without creating a
  dependency cycle, not even for one intermediate commit (see resolved open question 41). `winit::event::
  KeyEvent`/`DeviceId`/`ElementState`/`MouseButton`/`MouseScrollDelta`/`TouchPhase` **do** stay unchanged in
  this commit, unaffected by Q41 — `winit` is an ordinary external dependency, not a workspace member, so
  naming those types in `surge-sdk` carries no cycle risk; only the `SpriteDatabase`-shaped parameters were
  ever actually forced to move together. `surge-core::app::mod.rs` does `pub use surge_sdk::SurgeApp;`;
  `LoadedApp`/`load_from_dylib` are otherwise untouched (their `Box<dyn SurgeApp>` field doesn't care what
  the trait's methods take). Both modules' `Cargo.toml`s gain a `surge-sdk` path dependency (alongside
  their existing `surge-core` one — not yet removed); their `impl SurgeApp for ...` blocks change the `use`
  line and the three method signatures. `surge-player/main.rs`'s call sites need no cast — `&mut
  SpriteDatabase` coerces to `&mut dyn SpriteCanvas` at the call site (an ordinary unsized coercion).
  **Bundled into this same commit** (see resolved open question 36): `surge-mod-2048` drops its
  `cached_textures` field, `decode_image_assets`, and the `DecodedTexture` import, and calls
  `SpriteCanvas::upload_texture(path)` directly per asset name from both `on_load` and
  `on_swapchain_recreate` — this is a forced consequence of the signature change (there is no `SpriteCanvas`
  method that does what `upload_decoded_texture` did), not optional cleanup, so splitting it into its own
  commit would leave this one not actually compiling in between. `surge-mod-default`'s body needs no
  equivalent change (it already calls `upload_texture(path)` directly). Test: compile gate + smoke test,
  **both modules explicitly** — this is the first commit that changes what either module actually does at
  runtime; watch 2048's textures reload correctly on a resize.
- **1.7.** Add the `surge_sdk::KeyEvent` mirror struct (Q35's field list, plus `Debug` — both modules'
  `keyboard_event` bodies currently `log::info!("{:?}", event)` or pattern-match fields by name, and the
  mirror's field names match winit's exactly, so pattern matches like `surge-mod-2048/src/lib.rs:126-132`
  need no change beyond the `use` line). Retype `keyboard_event` to take it; `surge-player/main.rs`
  constructs one from the real `winit::event::KeyEvent` in the `WindowEvent::KeyboardInput` arm before
  calling through. Test: compile gate + smoke test, explicitly press a key on each module and confirm the
  log line / arrow-key handling still fires correctly.

*(**Milestone A**: `surge-sdk` exists and is load-bearing; `SpriteCanvas`/`CanvasError`/the `KeyEvent`
mirror are fully live and proven by the smoke test; modules are still `dlopen`'d exactly as today, still
depend on `surge-core` directly, still build as `crate-type = ["rlib", "dylib"]`. Zero process-isolation
risk taken yet — if something in Phase 2/3 goes sideways later, the codebase can sit at this milestone
indefinitely without being in a half-migrated state.)*

### Phase 2 — Dev-mode IPC machinery, built additively (nothing wired into `main()` yet)

- **2.1.** Define `Request`/`Response`/`CanvasOps`/`InputEvent` in `surge-sdk` per "Message shape" above
  (no `DeviceId` field, per Q19); `postcard` is already a dependency since 1.2 (Q48). Two prerequisites this
  step covers that pass 7 left unstated (Q48): add `#[derive(Serialize, Deserialize)]` to the
  `surge_sdk::KeyEvent` struct from 1.7 (only `Debug` until now — this is the first point anything needs it
  to cross `postcard`), and turn on `winit`'s `serde` Cargo feature in `surge-sdk/Cargo.toml` (needed for
  `KeyEvent`'s own `PhysicalKey`/`Key`/`KeyLocation`/`ElementState` fields and for `InputEvent`'s
  `MouseButton`/`MouseScrollDelta`/`TouchPhase` to derive `Serialize`/`Deserialize` at all). Also add
  `write_message`/`read_message` helpers — a `u32`-little-endian length prefix around each `postcard`-encoded
  value, generic over `impl Write`/`impl Read` (Q47: a `UnixStream` has no built-in message boundaries, and
  2.3's `drive()` needs to read exactly one `Request` at a time off an arbitrary stream; this is the framing
  open question 8's spike actually used, never assigned to a step before now). A `#[test]` round-trips one
  instance of every `Request`/`Response` variant through `postcard` directly, plus a second `#[test]` that
  writes several back-to-back frames through `write_message` over a real `UnixStream::pair()` and reads them
  back through `read_message`, confirming the length-prefix framing — not just serialization — actually
  holds a stream boundary. Test: `cargo test -p surge-sdk`.
- **2.2.** Add the dev-mode `SpriteCanvas` implementor to `surge-sdk`: buffers `add_instance`/
  `upload_texture` calls, drains them into a `CanvasOps`, and answers `get_create_info()` from a cached
  `CreateInfo` (Q27's push-not-pull fix) rather than any round trip. Pure data structure — no socket
  I/O — so it's fully unit-testable without a process or even a `UnixStream`: push N calls, drain, assert
  the resulting `CanvasOps`. Test: `cargo test -p surge-sdk`.
- **2.3.** Implement `run_dev_module`'s dispatch loop as two layers, not one — a deliberate testability fix
  over what earlier passes implied. A private, non-diverging `fn drive(app: &mut dyn SurgeApp, socket: impl
  Read + Write) -> io::Result<()>` does the actual handshake/dispatch/reply loop and returns `Ok(())` after
  `Shutdown`/`ShutdownAck`; the public `pub fn run_dev_module(app: Box<dyn SurgeApp>) -> !` is a thin
  wrapper that resolves the socket path from `std::env::args().nth(1)` (Q39), connects, calls `drive`, then
  `std::process::exit(0)`. A `-> !` function that calls `process::exit` cannot be safely called from inside
  a normal `#[test]` (it would kill the test process), so without this split the whole dispatch loop would
  be untestable short of spawning a real subprocess per test. With the split, a `#[test]` uses
  `UnixStream::pair()` (an in-process, kernel-backed socket pair — no subprocess, no real spawn) to drive
  `drive()` against a scripted sequence of `Request`s from a second thread standing in for the player, and
  asserts the `Response`s it gets back. Test: `cargo test -p surge-sdk`.
- **2.4.** Add `surge-player/src/proc.rs`: `set_pdeathsig()` (`libc::prctl(PR_SET_PDEATHSIG, ...)`, Q18) and
  `terminate(pid)` (`libc::kill(pid, libc::SIGTERM)`/`nix::sys::signal::kill`, Q30) as free functions, not
  yet called from anywhere. Add the `libc`/`nix` dependency to `surge-player/Cargo.toml`. Test: compile
  gate only — there is no meaningful unit test for `PR_SET_PDEATHSIG` short of a real process pair;
  functional verification is deferred to 2.6/3.x once it's wired in.
- **2.5.** Build `RemoteApp` in `surge-player`: binds the abstract-namespace `UnixListener` (Q23) before
  spawning, spawns the module binary via `Command` (socket name as `argv[1]`, `pre_exec` calling
  `set_pdeathsig`), accepts the connection, validates the unprompted `Response::Handshake`, and implements
  `SurgeApp` — `on_load`/`on_swapchain_recreate`/`update` round-trip a `Request` and apply the returned
  `CanvasOps` against the real `&mut dyn SpriteCanvas` it was given (texture uploads first, in order, then
  instances — a `texture_uploads` failure mid-sequence stops processing and routes to the Q11 crash path,
  per Q17's part 2); `keyboard_event`/`mouse_button_event`/`mouse_wheel_event` buffer into a local
  `Vec<InputEvent>`, no IPC, drained into the next `Request::Frame`. The outgoing `create_info` on
  `Request::OnLoad`/`Request::OnSwapchainRecreate` (Q44) comes from `canvas.get_create_info()` — called on
  the same real `&mut dyn SpriteCanvas` argument `on_load`/`on_swapchain_recreate` were just handed, before
  sending the `Request` — not a new value `RemoteApp` computes or caches itself; `SurgeContext` already
  constructed that canvas with the right `CreateInfo` before calling through. Not yet constructed from
  `main()`. Test: compile gate only — there is deliberately no throwaway fixture binary built just to
  integration-test this in isolation; the first real end-to-end exercise of `RemoteApp` is 3.2, against a
  real module. This is an accepted, named risk, not an oversight: Phase 2 proves the protocol's *types* and
  the dispatch loop's *logic* (2.1–2.3) in isolation, but proves nothing about the actual spawn/socket/kill
  mechanics until a real process is on the other end.
- **2.6.** Wire `set_pdeathsig`/`terminate` and the `reload_in_progress` flag (Q22) into `RemoteApp`'s
  spawn/kill/reload methods (the inherent `RemoteApp::reload()`, not yet called from anywhere). Test:
  compile gate. (The `SurgeApp::reload() -> bool` trait method that lets generic code — `SurgeContext`,
  operating only on `Box<dyn SurgeApp>`/`StartupApp` — actually reach this is added at 3.5, per Q45, once
  the F5 keybind is what first needs it; adding it here would be premature; noted now so it isn't missed
  later.)

*(**Milestone B**: the entire dev-mode machinery exists, is additive, and compiles — but it is unproven
end-to-end. Phase 3 is where it either works against a real module or doesn't.)*

### Phase 3 — Cutover: `dlopen` → spawn+IPC, module by module (Q38's scaffolding)

- **3.1.** Add the temporary `SURGE_DEV_LOADER` branch to `surge-player::main()` (Q38): unset/anything but
  `process` keeps `load_from_dylib`; `process` selects a new `spawn_and_connect(&engine_config.startup_app)`
  path constructing a `RemoteApp`. Since `main()` must build `ctx.startup_app` from *either* branch and a
  struct field can't hold two different concrete types, this step also introduces the temporary
  `StartupApp` enum (Q43): `enum StartupApp { Dylib(sc::app::LoadedApp), Remote(Box<dyn SurgeApp>) }`, with
  hand-written `Deref`/`DerefMut` to `dyn SurgeApp` (a two-arm `match`), and `SurgeContext.startup_app`'s
  type changes from `sc::app::LoadedApp` (`surge-player/src/main.rs:21`) to `StartupApp` — every existing
  `self.startup_app.<method>(...)` call site is unaffected, since deref coercion covers the enum exactly as
  it already covered `LoadedApp`. Nothing can select `process` successfully yet. Test: compile gate + smoke
  test with the variable unset (unchanged behavior, still `dlopen`, now routed through
  `StartupApp::Dylib(...)`).
- **3.2.** Move `init_env_logger`/`resolve_asset_path` from `surge-core::cli`/`surge-core::assets` to
  `surge-sdk` (Q32) — this becomes load-bearing exactly here, not before: it's the first point a module
  needs them *without* a `surge-core` dependency. `surge-core::cli::print_logo` stays in `surge-core` (only
  `surge-player::main` calls it; not a module concern). This move also breaks `surge-player`'s *own* logger
  init (Q42) — `surge-player/src/main.rs:332` calls `sc::cli::init_env_logger()` for the player itself, and
  that function no longer exists in `surge-core` once this step lands — so this same commit adds a direct
  `surge-sdk` path dependency to `surge-player/Cargo.toml` and changes that call site to
  `surge_sdk::init_env_logger()`. Port `surge-mod-2048`: add `src/bin/dev.rs` (`surge_sdk::init_env_logger();
  surge_sdk::run_dev_module(Box::new(App2048::new()));`), drop the `dylib`
  crate-type (keep `rlib`, needed by the eventual release `[[bin]]` target), swap the `surge-core`
  dependency for `surge-sdk` only. Test: compile gate + `SURGE_DEV_LOADER=process cargo run -p surge-player`
  with `config.toml` pointed at `surge_mod_2048` — full smoke test, this is the first real end-to-end proof
  of the IPC path (this is also where 2.5's deferred integration risk actually gets resolved).
- **3.3.** Port `surge-mod-default` the same way. Test: compile gate + `SURGE_DEV_LOADER=process` smoke test
  with `config.toml` pointed at `surge_mod_default`.
- **3.4.** Cutover complete: delete the `SURGE_DEV_LOADER` branch and default `main()` to the `process` path
  unconditionally; delete `surge-core/src/app/mod.rs` (`LoadedApp`, `load_from_dylib`), `AppError` and
  `ModuleError` from `surge-core/src/errors/mod.rs` (Q29 — `ModuleError` was already fully dead before this
  doc), and the `libloading` dependency from `surge-core/Cargo.toml`. The `StartupApp` enum from 3.1 (Q43)
  has nothing left to hold in its `Dylib` arm, so it collapses too: `SurgeContext.startup_app` becomes plain
  `Box<dyn SurgeApp>`, `StartupApp` itself is deleted. This is where the "Release mode" section's "swapping
  the `startup_app` field's type... is a small, mechanical change" claim actually finishes landing — not at
  4.1, which is pure code motion by the time it runs. Test: compile gate + smoke test both
  modules (swap `config.toml`'s `app_name` between them), confirm `ldd`/`nm` show no `libloading` symbol
  left in either `surge-core` or `surge-player`.
- **3.5.** Add `fn reload(&mut self) -> bool { false }` to `SurgeApp` (default no-op; Q45), overridden by
  `RemoteApp::reload` to run the inherent `RemoteApp::reload()` sequence from 2.6 and unconditionally return
  `true`. Add the throwaway F5 reload keybind (Q40) in `surge-player`'s dev binary: `SurgeContext`'s
  `WindowEvent::KeyboardInput` arm (`surge-player/src/main.rs:295-300`) special-cases F5 before forwarding
  to the module — `self.recreate_swapchain |= self.startup_app.reload();` instead of calling
  `keyboard_event`, so F5 never reaches the module's own key handler and the Q14 rebuild-on-reload path
  (which only `SurgeContext` can trigger, via that same flag) actually fires. Test: smoke test — press F5
  mid-session against `surge-mod-2048`, confirm the board re-renders with no leftover child process (`ps`
  before/after) and no texture-cap warning after several presses (proves the Q14 `SpriteDatabase`
  rebuild-on-reload fix). This proves kill/respawn/reconnect/rebuild work in the ordinary, non-racing case —
  it is **not** proof that `reload_in_progress` (Q22) is correct: F5 is handled in `window_event`, the
  blocked `Request::Frame` read happens in `about_to_wait`, and winit calls these sequentially on one
  thread, so a keypress can never actually be delivered *while* the main thread is blocked inside that read
  (Q46). Q22's flag is still needed — it's just that nothing in this plan, including this step, exercises
  the race it exists for; that only happens once a real, necessarily-async trigger (file watcher or
  similar) replaces F5, which is explicitly out of this doc's scope.

*(**Milestone C**: dev mode is exactly what this doc specifies, fully live, fully proven by hand, with zero
permanent scaffolding left over. `surge-player`'s binary has exactly one behavior, per Q21.)*

### Phase 4 — Release mode

- **4.1.** Extract `surge-player`'s run loop into a `[lib]` target: move `SurgeContext`/the
  `ApplicationHandler` impl into `surge-player/src/lib.rs`, expose `pub fn run(app: Box<dyn SurgeApp>,
  config: EngineConfig)`; `surge-player`'s own `main.rs` (the dev binary) becomes `RemoteApp` setup +
  `run(Box::new(remote_app), config)`. `SurgeContext.startup_app` is already plain `Box<dyn SurgeApp>` by
  this point (the `StartupApp` scaffolding collapsed at 3.4, per Q43) — this step is pure code motion (move
  the struct/impl, wrap the existing logic in a function signature), not where that retyping happens; earlier
  drafts of this doc described it that way, which is corrected in "Release mode" above. Doing this *after*
  Phase 3 rather than alongside it means the extraction is a mechanical move of already-proven code, not
  extraction-and-protocol-swap at once. Test: compile gate + smoke test (dev mode must behave identically
  after a pure code-motion refactor).
- **4.2.** Per module, add the release `[[bin]]` target per Q37: `surge-player` as an `optional = true`
  dependency gated by a `release-bin` feature, a `src/bin/release.rs` (`surge_player::run(Box::new(...),
  config)`) with `required-features = ["release-bin"]`. Test: compile gate on the dev target (must **not**
  pull in `surge-player`/`ash`/`vk-mem` — check with `cargo tree -p surge-mod-2048` with no features
  enabled) + `cargo build -p surge-mod-2048 --features release-bin --release` + running the produced binary
  directly (no player process, no `config.toml` env-var games) against the module's own `config.toml`.

*(**Milestone D**: release mode exists — a single self-contained binary per module, no `dlopen`, no IPC, no
`libloading` — and dev mode is unaffected.)*

### Phase 5 — Shippability cleanup

- **5.1.** Rewrite `stager/stager.py` (flagged, not designed, by resolved open question 7): stage a single
  per-module release binary + its `config.toml` + `assets/`, not a player-exe-plus-`.so` pair. Test:
  `python3 stager/stager.py stage surge-mod-2048`, run the staged binary from a directory other than the
  repo root, confirm assets resolve (exercises `resolve_asset_path`'s staged-path branch, now keyed off the
  *module's* `current_exe()` per the behavior change noted under Q32).
- **5.2.** Delete the now-fully-superseded `hot_reloading` feature flag from any leftover
  `surge-player/Cargo.toml` reference if 3.1–3.4 didn't already remove it (Q21). Sweep `CLAUDE.md`'s "Build
  & run" and "Frame lifecycle" sections for now-inaccurate statements (`load_from_dylib`, the `dlopen` ABI
  description, the CWD contract's `libsurge_mod_*.so` mention, the stale "no tests" line 1.2 already
  touched) — a documentation-only commit, not a design change, closing the loop this doc opened by citing
  `CLAUDE.md` as ground truth throughout.

## Self-critique / risks

- The IPC round-trip (Unix domain socket + `postcard`) was the new load-bearing piece replacing the old
  dylib-sharing risk, and it's now measured (see open question 8): negligible tax at p95 in both debug
  and release, idle or under heavy contention. `run()` extraction and per-module release binaries are
  fairly ordinary Rust and low-risk. The one residual data point: release-build round trips showed a rare
  multi-millisecond tail under 32-core contention — small at 144 Hz, worth re-checking if the engine ever
  targets a much tighter frame budget.
- Corrected in pass 2: the `RemoteApp`/`SpriteCanvas` split is *not* as low-risk as pass 1 characterized
  it — it requires an actual breaking change to `SurgeApp`'s signature (`&mut dyn SpriteCanvas`, an
  SDK-owned `KeyEvent`), not just a new trait bolted on alongside the old one (see open question 13), and
  reload silently blowing the 32-texture cap (open question 14) was a correctness bug in pass 1's design,
  not just a risk to monitor.
- Pass 3 closed the two remaining gaps (open questions 16, 17). Worth flagging that 17's fix reaches
  outside this doc's own scope: `upload_texture`'s return type changing to `Result<usize, VulkanError>`
  is a `surge-core` change that benefits *today's* single-process code too (the silent-`Ok(())`-on-overflow
  behavior is a pre-existing bug, not something this redesign introduced), so it should land as its own
  small fix rather than getting buried as a side effect of the IPC work whenever this doc is implemented.
- Pass 4 was the first pass checked against the actual source rather than re-read for internal
  consistency, and it found real teeth: open question 19 (`DeviceId` not serializable) would have been a
  compile error discovered only once someone actually wrote the `InputEvent` enum, and open question 20
  (orphan rule forcing `surge-core` → `surge-sdk`, and ruling `surge-player` out as `SpriteCanvas`'s home
  entirely) is exactly the kind of thing that's invisible from reading the doc's prose — "`RemoteApp`
  living in `surge-player` or `surge-core`" reads like a preference, not a question with only one legal
  answer. It predicted its own follow-up: a pass should spend time on the crates that don't exist yet
  (`surge-sdk` in particular) at the level of "what does its actual `Cargo.toml` look like, what does
  each of its types derive." Pass 5 did exactly that and found the same shape of bug again, one level
  deeper — `InstanceInfo`/`CreateInfo` and friends (open question 26) are `CanvasOps`'s payload, not
  `InputEvent`'s, but the failure mode is identical: an unchecked assumption that a type "obviously"
  crosses `postcard` fine, falsified by the actual derives once someone checks. Two passes in a row
  finding a genuine compile blocker by checking one more type against real `Cargo.toml`s/derives is a
  signal in itself: prose review has a real, nonzero false-negative rate against this class of bug, and a
  pass 6 (if the doc needs one) would be better spent as a small throwaway crate — a `surge-sdk` stub with
  the actual proposed types, actually compiled and actually round-tripped through `postcard` once — than a
  third read-through, the same way open question 8 stopped being a prose question once an actual
  two-binary harness was built and measured.
- Pass 6 did exactly that (open question 33) and it changed the outcome, not just added confidence: every
  serde-derive/object-safety/API-existence claim checked out as described, but the spike also caught pass
  5's own orphan-rule justification for the `CanvasError` conversion being outright wrong (open question
  34) — a `From` impl compiles fine; the doc had been giving implementers a reason to write *more* code
  than necessary, based on a rule that was mis-stated, not just unchecked. That a harsh-critic pass can
  itself introduce a wrong "here's why you can't do the simple thing" claim — and that it took actually
  compiling the alternative to catch it, not another close reading — is worth remembering the next time
  this doc (or one like it) leans on a language-semantics argument: state the rule, then check it against
  the compiler, don't just check it against memory of the rule. Three read-through-only passes (2, 3, and
  the parts of 4 that didn't touch real source) produced zero compile-blocking corrections; three passes
  that ran something (4's source-grep, 5's derive/`Cargo.toml` check, 6's actual `cargo build`) found five
  between them (19, 20, 26, 28-corrected-to-34, and the near-miss in 27). That ratio is itself a data point
  for how this doc should be reviewed going forward, on this or any future design doc in this repo.
- The message-shape design (open question 16) deliberately keeps every `Request`/`Response` exchange
  flat — one request, one reply, no nested sub-RPCs — by making `CanvasOps` carry both texture uploads
  and instances together and having the player process uploads synchronously on its own side rather than
  acking each one individually. This was a real design choice, not the only option: a per-upload ack
  round trip was considered and rejected because neither existing module needs the returned ID
  synchronously (see question 17's resolution) and it would have added a second, structurally different
  message-passing mode alongside the simple per-turn loop for no behavioral gain.
- Blocking on a slow module (open question 10) means a stalled dev-mode module makes the *player* appear
  to hang too, distinguishable only by the debug logging called out there. That's an accepted trade for
  matching today's synchronous behavior, but worth remembering if it ever reads as a player bug during a
  session instead of a module one.
- This doc knowingly departs from the README Philosophy's "single shared object" mechanism (see
  "Divergence from README Philosophy" above) even though it preserves the same observable hot-reload
  goal. Worth re-confirming that trade explicitly with anyone relying on the README description, not just
  burying it in this doc.
- Per-module release `[[bin]]` targets mean N modules → N full static builds of the engine. Today that's
  2 modules; it's fine. It won't scale indefinitely, but that's a build-time problem, not a correctness
  one, and out of scope to solve preemptively.
- Texture uploads crossing as asset paths (module tells player *what* to load, player does the IO) is a
  simplification that assumes both processes always agree on the same asset root. That's true today
  (repo-root-relative paths, per `CLAUDE.md`) but worth restating as an assumption, not a given, if asset
  loading conventions ever change.
- The `surge-sdk` split now adds a fourth crate to a workspace that currently has three logical layers
  (per the README Philosophy section: core/player/modules), but unlike the original framing, it's no
  longer "a guardrail this doc admits isn't a hard boundary" — the boundary is now hard (process
  isolation), and `surge-sdk` is pulling real weight as the `SpriteCanvas` abstraction dev mode actually
  needs to function at all.
- This doc still treats "release build" as "one binary per game." If the actual goal is ever "one player
  binary that can run any of several statically-known games chosen via a build-time feature flag," that's
  a different, not-yet-designed shape (feature-flag-selected module rather than per-module `[[bin]]`
  targets) — flagging in case that's actually closer to what's wanted than what's written above.
- Pass 7 turned the doc into a commit sequence instead of reading it again, and that process itself found a
  fourth instance of pass 4–6's recurring bug class (open question 36, `surge-mod-2048`'s
  `decode_texture_file`/`upload_decoded_texture` calls) — worth being honest that this means passes 4
  through 6, despite explicitly checking real call sites, still missed a real one, and pass 7 only caught it
  because writing "which trait method does this call site need" for *every* line forces a completeness a
  prose read-through doesn't. It also produced four decisions (37–40) that pass 6's rigor couldn't have
  resolved on its own, because they aren't compile-or-doesn't-compile questions — they're "which acceptable
  trade-off do you want," and the fix for that class of gap isn't a harsher pass, it's asking. Both are
  worth remembering for whatever this doc's implementation actually looks like: verification passes reduce
  the false-negative rate on checkable claims, they don't drive it to zero, and not every open question in
  a doc like this has a single right answer waiting to be found by reading more carefully.
- Pass 8 checked the commit sequence pass 7 produced against itself — does step N compile given exactly
  what steps 1..N-1 left behind — rather than checking prose against source again, and that turned up a
  fourth failure mode distinct from passes 4–7's (open question 41: a step that's individually fine but
  creates a Cargo dependency cycle *in combination with* an earlier step, only visible by tracking the
  crate graph's state across the sequence, not by reading either step in isolation), plus a cluster of
  "step N quietly assumes machinery — a dependency, a field's type, a value's source, a trait method — that
  no step actually adds" gaps (42–45, 47, 48) that a pure prose read keeps missing for the same structural
  reason pass 7's own self-critique named for open question 36: completeness comes from forcing yourself to
  trace *every* consequence of a change, not from reading carefully. It's worth naming the pattern across
  all eight passes plainly, since it's the same lesson wearing four different costumes: 2–3 found gaps by
  re-reading prose; 4–6 found gaps by checking prose against compiled reality; 7 found a gap by checking
  prose against every real call site instead of the ones already grepped; 8 found gaps by checking the
  *plan's own steps* against each other instead of against the design prose one more time. Each of those is
  a different question, catches a different class of bug, and none of them is "the harsh-critic pass," full
  stop — there may well be a ninth class of gap a pass 9 would need a fifth kind of check to find. Whoever
  implements this doc should not read "all N items resolved" as "no more gaps of any kind," only as "no
  more gaps of the kinds passes 1 through 8 knew to look for."
