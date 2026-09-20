# Module ABI & Linking Model

**Status:** Resolved (pass 14) — pass 13 asked whether every behavior this design specifies has a commit
that builds it. Pass 14 asks the mirror question it named as the next one worth asking: **does every commit
have a test that could actually fail, and does that test observe the thing the commit was for?** Not "is a
test named" (several steps name one) but "if the implementer got this step wrong in the most likely way,
which test goes red." It found eight gaps, none overlapping items 1–73. **Open question 74** is the
sharpest: `PR_SET_PDEATHSIG` (Q18) is exercised by nothing — the only scenario that triggers it is the
player dying *without* running `exiting()`, and no test in the plan ever kills the player, so the orphan
guard could be a no-op (a wrong `prctl` argument, a `pre_exec` that never runs) and every step would still
pass. **Open question 75** is the same shape for the escalation branches: F5 exercises `SIGTERM` on a module
that exits promptly, and nothing ever exercises the `SIGKILL`-after-timeout or `ShutdownAck`-timeout paths
those sequences exist for. **Open question 77** covers two more never-run paths — the handshake-version
mismatch and Q67's stall-vs-fault distinction — both of which turn out to be *cheaply* testable by a trick
this doc hadn't noticed: the module is a separate file on disk, so rebuilding only one side of the socket
(`cargo build -p surge-player` after bumping `WIRE_VERSION`) produces a genuinely mismatched pair, which is
the real-world failure the handshake exists for rather than artificial injection. **Open question 76** fixes
the one place the plan admits its own test can't distinguish right from wrong (Q60's `pending_on_load`,
which 3.5 concedes both real modules hide) with one `log::debug!` line. **Open question 78** is
cross-cutting: the compile gate is `check` + `build` and never `test`, so from 1.2 onward every suite this
plan creates can silently regress in any later commit. **Open questions 79–81** cover four steps with no
stated test at all, five "compile gate only" steps that never say where their behavior is first proven, and
three test-precision gaps. All are resolved below with fixes folded back into the steps they affect. Pass
13's own paragraph, below, is left as written; pass 12's is dropped per this doc's convention of keeping
only the immediately-preceding pass's status paragraph as history.

**Status (pass 13):** Resolved — pass 12 asked what the build and the OS actually *produce and name* at each
step. Pass 13 asks the complementary question, aimed at the implementation plan rather than the prose:
**does every behavior this doc specifies have a commit that builds it, and can that commit's stated test
actually observe it?** Not "does step N compile given steps 1..N-1" (pass 8's question) but "if an
implementer executed all 20 steps faithfully, which sentences of this design would still be unimplemented
at the end." It found nine gaps, none overlapping items 1–64. **Open question 66** is the most serious:
Q11's crash-state path is *referenced* by three separate steps (2.5's `texture_uploads` failure routing,
2.6's `reload_in_progress` check, Q17's part 2) and *implemented* by none — a design requirement with no
commit, invisible to every smoke test in the plan. **Open question 65** is the same shape one lifecycle
call over: no step on either side of the socket implements `Request::Shutdown`/`ShutdownAck`, so
`on_unload` — which `surge-player`'s `exiting()` calls today and `surge-mod-default` actually logs from —
is silently never delivered to a module process after the Phase 3 cutover. **Open questions 68 and 70** are
compile-blocking: `InstanceInfo`/`SubTextureInfo` derive `Debug` only, so 2.2's proxy cannot buffer one
(verified by compiling the fix — `Clone, Copy` derive cleanly over their `nalgebra` fields), and 3.4
deletes the file whose `pub use` two crates are importing `SurgeApp` through without repointing either.
**Open question 71** turns Q20's own release-mode argument on the crate Q20 exempted: release binaries
depend on `surge-player`, which is where 2.4/2.5 put `RemoteApp` and the socket/spawn/signal code — the
identical "IPC machinery compiled into a release binary" objection Q20 used to keep that code *out* of
`surge-core`. **Open questions 67, 69, 72, 73** cover Q10's never-assigned stall logging (and its collision
with crash detection), the `SpriteCanvas` method list this doc has named for eleven passes without ever
writing down, module-binary path resolution and the dev `config.toml` value it needs, and three smaller
precision gaps. All are resolved below with fixes folded back into the sections and steps they affect.
(Pass 12's own paragraph was retained here through pass 13 and dropped by pass 14's trim, per this doc's
convention of keeping only the immediately-preceding pass's status paragraph as history.)

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
  (`{app_folder}/{app_name}`, no `lib`/`.so` decoration) — same `[startup_app]` shape in dev as today, only
  the resolution logic behind it changes. (In *release* mode nothing resolves `[startup_app]` at all: the
  module is the binary, statically linked. The keys stay in the shipped `config.toml` as inert leftovers
  rather than being removed, since `EngineConfig`'s serde struct requires the table to be present — see
  open question 64c.)
  - **This only works if the module's dev binary is actually named `{app_name}`** (open question 59,
    resolved). Cargo names a binary target after its *source file*, not its package, so an unadorned
    `src/bin/dev.rs` in both module crates produces `target/debug/dev` twice — an output-filename collision
    (verified by running it: `cargo build --workspace` warns, builds both, and leaves whichever linked last
    at that path), and a path no `app_name` in either checked-in `config.toml` (`surge_mod_2048`,
    `surge_mod_default`) names. Every module crate therefore declares its binaries explicitly, with the dev
    one's name matching its `app_name` exactly:
    ```toml
    [[bin]]
    name = "surge_mod_2048"          # == [startup_app] app_name; file stays src/bin/dev.rs
    path = "src/bin/dev.rs"

    [[bin]]
    name = "surge_mod_2048_release"  # release mode; see "Release mode" below
    path = "src/bin/release.rs"
    required-features = ["release-bin"]
    ```
    A `[[bin]]` whose name equals the package's `[lib]` name is legal — **verified by building it** rather
    than by analogy (open question 73d): a throwaway package named `surge-mod-2048`, with
    `crate-type = ["rlib"]`, a lib target named `surge_mod_2048`, and the two `[[bin]]` tables above, builds
    clean and emits `target/debug/surge_mod_2048` alongside `libsurge_mod_2048.rlib` — no name-collision
    diagnostic, since the two targets are different kinds with different output filenames. (The analogy
    passes 1–12 used for this — "it's exactly what `src/main.rs` + `src/lib.rs` produces by default" — is
    actually false for a hyphenated package: the default bin is named after the *package*
    (`surge-mod-2048`), the lib after its underscored form (`surge_mod_2048`), so the default arrangement
    never produces the same-name case at all. The conclusion holds; the reasoning offered for it did not.)
    The same spike confirmed `required-features = ["release-bin"]` behaves as Phase 4 assumes: a plain
    `cargo build` emits only the dev binary, and the release one appears only under
    `cargo build -p <pkg> --features release-bin`.
  - **Who resolves the path, and what the dev `config.toml` has to say** (open question 72, resolved).
    Resolution moves from `surge_core::app::load_from_dylib` (deleted at step 3.4) to `RemoteApp`'s
    constructor in `surge-player`, which takes the parsed `StartupAppConfig` and builds
    `Path::new(app_folder).join(app_name)` — no `lib` prefix, no extension, canonicalized so a missing or
    non-executable file fails loudly at spawn time with the resolved path in the message, rather than as an
    opaque `Command::spawn` `ENOENT`. Both checked-in module configs currently say `app_folder = "."`
    (checked: `surge-modules/*/config.toml`), which was already only meaningful for the staged layout and
    is inert in release mode regardless (Q64c). Dev runs therefore need the root working copy of
    `config.toml` — gitignored, per-developer, per `CLAUDE.md`'s CWD contract — to carry
    `app_folder = "target/debug"`, exactly as it already must today to find `libsurge_mod_2048.so`. The
    contract that changes is the *filename* under that folder (`surge_mod_2048`, not
    `libsurge_mod_2048.so`), not the folder itself; the checked-in copies need no edit.
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
        /// Asset paths, in call order. texture_id is the proxy's *cumulative* upload counter, not
        /// this vec's index — the two coincide only when the canvas was freshly built at the start
        /// of the turn (see open questions 17 and 63).
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
  - **What `Shutdown` actually carries on each side** (open question 65, resolved, added pass 13). Q16
    folded `on_unload` into `Shutdown`/`ShutdownAck` rather than giving it its own round trip, but no
    later pass ever said which code calls `SurgeApp::on_unload` afterwards, and neither side's step in
    "Implementation plan" built it. Pinned down: `RemoteApp`'s own `SurgeApp::on_unload` impl is what
    sends `Request::Shutdown` (it is the one trait method `surge-player`'s `exiting()` already calls, per
    `surge-player/src/main.rs:237`), and `surge_sdk::run_dev_module`'s `drive()` loop calls the concrete
    app's `on_unload()` on receiving `Shutdown`, *then* replies `ShutdownAck`, then returns. Both halves
    are needed for the hook to survive the cutover at all: `surge-mod-default::on_unload` logs
    "Default app shutdown" today (`lib.rs:158-160`), so its disappearance from a dev-mode run is the
    cheap, concrete observable that this path works. The player's wait for `ShutdownAck` is a bounded
    `UnixStream::set_read_timeout` (no new dependency; `std` covers it), after which it escalates
    `SIGTERM` → wait → `SIGKILL` and reaps with `Child::wait` so `exiting()` leaves no zombie. A
    `RemoteApp` already in the fault state (open question 66) skips the wire entirely and goes straight
    to the reap.
  - **`texture_id` handshake** (open question 17, resolved). Traced to the root cause: this was never
    really an IPC problem. `SpriteDatabase::upload_decoded_texture` (`surge-core/src/vulkan/
    sprite_database/mod.rs:643`) already returns `Ok(())` — not the assigned index — even in today's
    single-process code, and silently no-ops (still returning `Ok(())`, not an `Err`) once
    `uploaded_textures.len() == max_textures`. Neither existing module reads the return value for an ID
    anyway — `AppDefault`/`App2048` both hardcode `texture_id` as a literal matching upload order, i.e.
    they already trust positional prediction, not a returned value. So the dev-mode `SpriteCanvas` proxy
    doing the same (predicting `texture_id` locally, no round trip, no ack) is not a new assumption — it's
    the existing one, carried across a process boundary unchanged. **What the proxy counts, precisely**
    (open question 63, added pass 12): a *cumulative* upload counter for the life of the connection, reset
    only when it caches a new `CreateInfo` off a `Request::OnLoad`/`OnSwapchainRecreate` — which are exactly
    the two points the player has just rebuilt its `SpriteDatabase` from scratch, and therefore the only
    two points the real database's own index restarts at zero. Counting per-call instead (this vec's index)
    silently diverges from the release backend's return value for any upload made from `update()` on a
    canvas that already holds textures — legal under the trait, done by neither module today, and
    undetectable at the boundary. Two more changes make the desync case safe instead of silent:
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
  - **The respawned child must still receive `Request::OnLoad` first, and the reload path as specified
    never sends one** (open question 60, resolved, added pass 12). `RemoteApp::reload()`'s signature
    (Q45) is `fn reload(&mut self) -> bool` — no canvas argument — so it cannot itself send an `OnLoad`,
    whose `create_info` field must come from `canvas.get_create_info()` per Q44. All it can do is return
    `true`, which sets `SurgeContext.recreate_swapchain`, which one turn later rebuilds the
    `SpriteDatabase` (Q14) and calls `on_swapchain_recreate` — so the fresh process's *first ever*
    request would be `OnSwapchainRecreate`, and it would never see `OnLoad` at all. Fix, no new wire
    message and no signature change: `RemoteApp::reload()` sets an internal `pending_on_load` flag after
    the re-handshake succeeds, and `RemoteApp::on_swapchain_recreate` checks it — if set, it sends
    `Request::OnLoad { create_info }` (built from the canvas it was just handed, exactly as
    `RemoteApp::on_load` does) instead of `Request::OnSwapchainRecreate`, and clears the flag. The
    delivery is guaranteed: `reload()` returning `true` is what forces that very call. This keeps
    "`OnLoad` is always the first `Request` a module process receives" true for respawned children as
    well as the first one, which is what both the connect sequence below and Q14's own wording assume.
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
- **Crash state, concretely** (open question 66, resolved, added pass 13). Q11 settled the *policy* — a
  connection that dies outside a deliberate reload stops the player driving that module, logs loudly, and
  looks visibly different from reload churn — and three separate steps then routed failures into it
  ("the Q11 crash path") as though it were existing machinery. It is not: nothing in this doc says what
  holds the state, how `SurgeContext` learns about it, or what "visibly different" means in an engine with
  no text rendering. Pinned down, reusing Q45's own pattern (a default `SurgeApp` method) rather than
  inventing a second mechanism:
  - `RemoteApp` gains a `faulted: bool`. It is set — and cleared only by a successful `reload()` — when a
    socket read or write fails while `reload_in_progress` is false (Q22's flag is what distinguishes the
    two), when the handshake's `wire_version` mismatches, or when applying a `CanvasOps.texture_uploads`
    entry returns `Err` (Q17's part 2). Every `SurgeApp` method on a faulted `RemoteApp` becomes a no-op
    that touches neither the socket nor the canvas.
  - `SurgeApp` gains a second default method, `fn is_faulted(&self) -> bool { false }`, overridden only by
    `RemoteApp`. This is how `SurgeContext` — which only ever holds a `Box<dyn SurgeApp>` — sees the state
    at all, exactly as `reload()` is how it reaches the reload sequence. Release-mode in-process apps
    inherit the default and are never faulted, so the release path pays one never-taken branch per frame.
  - `SurgeContext` checks it at the top of `about_to_wait` and, on the transition into it, logs one loud
    `log::error!`, appends a `— MODULE CRASHED` suffix to the window title (`Window::set_title`, the only
    on-screen channel this engine has without a text renderer — deliberately not a new rendering feature),
    and from then on returns early from the frame without acquiring, drawing, or presenting. The last
    presented image stays on screen: that *is* "freeze last frame," and it costs no new machinery.
  - Recovery is F5 (Q40) and only F5 — never an automatic respawn, per Q11. `reload()` on a faulted
    `RemoteApp` runs the ordinary kill/respawn/reconnect sequence, clears `faulted` and the title suffix on
    a successful re-handshake, and returns `true` so the `SpriteDatabase` rebuild (Q14) happens as usual.
- **Making a blocked player visible** (open question 67, resolved, added pass 13). Q10 chose "block, but log
  loudly when blocked" and no step ever built the logging half. It is not free-standing: the natural
  implementation — a bounded `set_read_timeout` on the socket, logging `waiting on module response — Nms and
  counting` each time it expires and then reading again — is the same read path Q66's fault detection
  watches, so the two have to agree on one rule: `ErrorKind::WouldBlock`/`TimedOut` is a *stall* (log, retry,
  stay connected), and only `Ok(0)`/EOF or a genuine transport error is a *fault*. Conflating them turns
  every module paused under a debugger into a spurious crash state. Both rules live in the single
  `read_response`/`write_request` helper pair `RemoteApp` uses for every round trip, not scattered per call
  site.

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
- **`run()` owns process-wide startup, not just the loop** (open question 62, resolved, added pass 12).
  `init_env_logger()` and `print_logo()` are called from `surge-player`'s `main()` today; a release
  binary never runs that `main()`, so as earlier passes scoped Phase 4 a shipped game would come up with
  no logger installed at all (every `log::` call in `surge-core` and in the module silently discarded)
  and no logo. Both calls move *into* `run()`, which is the one entry point both modes share, and come
  *out* of `surge-player`'s dev `main()` in the same commit — duplicating rather than moving them would
  reproduce Q51's double-init panic exactly, one crate over (`env_logger::Builder::init()` can only
  succeed once per process). Dev-mode module *processes* keep their own `init_env_logger()` call in
  `src/bin/dev.rs` — separate process, separate global logger state, no conflict.
- **The IPC machinery must not follow `run()` into the `[lib]` target** (open question 71, resolved, added
  pass 13). Q20 kept `RemoteApp` and the socket/spawn code out of `surge-core` with an explicit argument:
  `surge-core` links into every release binary, so hosting them there would statically compile `postcard`
  and the spawn/socket machinery into games that never use it, contradicting this section's own "no IPC ...
  compiled in" goal. Under the final shape, release binaries depend on **`surge-player`** — the crate those
  types actually landed in — so the identical objection applies one crate over, and no pass had noticed.
  The fix needs no crate split and no feature flag, only a rule about *which target* owns them:
  `RemoteApp`, `proc.rs`, and the wire helpers stay modules of `surge-player`'s **`[[bin]]`** target,
  declared from `main.rs`, and never move into `src/lib.rs` during the Phase 4 extraction. A `mod` declared
  in `main.rs` compiles only into the binary crate, so `surge_player::run` — the whole of what a release
  binary links — carries none of it. What remains is honest but small, and worth stating rather than
  claiming a cleanliness the design does not have: `libc` and `postcard` stay in `surge-player`'s
  `[dependencies]` (Cargo has no per-target dependency scoping — the same limitation Q37 already hit), so a
  release build still *compiles* those crates even though no code path in the release binary reaches them.
  "No IPC compiled in" is therefore true of the linked binary's reachable code, not of the dependency
  graph's build cost.

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
  workspace today). **`InstanceInfo` and `SubTextureInfo` also gain `Clone, Copy`** (open question 68,
  resolved, added pass 13) — not tidiness, a compile requirement the wire design creates:
  `add_instance(&mut self, aii: &InstanceInfo)` takes a *reference* (unchanged, that's today's signature),
  so the dev-mode proxy has no way to push the value into its own `Vec<InstanceInfo>` buffer without
  duplicating it, and `Serialize` grants nothing of the kind. Verified by compiling the fix against this
  workspace's pinned nalgebra rather than assuming it: `#[derive(Debug, Clone, Copy)]` builds on both
  structs, since every field — `Vector2<f32>`, `Vector4<f32>`, `Vector2<u32>`, `f32`, `usize`,
  `Option<SubTextureInfo>` — is itself `Copy`. The other four types are already `Clone, Copy` today and
  need no change. `surge-core` depends on `surge-sdk` for these — consistent with the dependency
  direction open question 20 already established, just extended past the one `SpriteCanvas` impl to
  cover the types the wire format actually carries. One more consequence worth flagging: every module
  (`surge-mod-2048/Cargo.toml`, `surge-mod-default/Cargo.toml`) currently declares its own separate
  `nalgebra = "0.34.1"` dependency, not sourced through `surge-sdk`. That only type-checks today because
  the version string happens to match everywhere; a module's hand-written `InstanceInfo { position:
  Vector2::new(...), .. }` literal only compiles against `surge_sdk::InstanceInfo` if the module's own
  `Vector2` is the *same* monomorphized type as `surge-sdk`'s. Modules should drop their own `nalgebra`
  dependency and use `surge_sdk`'s re-exported math types exclusively, so a future version bump can't
  silently produce two incompatible `Vector2` types at the `InstanceInfo` boundary. **Pass 9 note (open
  question 52):** this paragraph named the bug and the fix but the fix had no concrete shape and no assigned
  commit, so it didn't actually happen anywhere in "Implementation plan" below. Resolved: `surge-sdk` does
  `pub use nalgebra;` (the whole crate, not a curated re-export list — nothing to maintain as `nalgebra`'s own
  surface grows), so module code becomes `use surge_sdk::nalgebra::{Vector2, Vector4};` and both modules'
  `Cargo.toml`s lose their own direct `nalgebra` line. See step 1.6, the point each module first gains a
  `surge-sdk` dependency and is already touching every `InstanceInfo`/`SubTextureInfo` call site regardless.
  **Pass 12 note (open question 61):** `winit` needs the identical treatment, and pass 9–11 all missed it
  while looking straight at the `nalgebra` version-skew argument. Both modules declare their own
  `winit = "0.30.12"` (checked, both `Cargo.toml`s) and name winit types in their `SurgeApp` impls'
  signatures (`DeviceId`, `ElementState`, `MouseButton`, `MouseScrollDelta`, `TouchPhase`) and inside their
  bodies (`surge-mod-2048` pattern-matches `PhysicalKey::Code(KeyCode::ArrowUp)`), against a trait that
  lives in `surge-sdk` and names winit types in those same positions — the exact "only type-checks because
  the version strings happen to match" shape Q52 named. Exposure *grows* at 1.7, not shrinks: the
  `surge_sdk::KeyEvent` mirror's own fields are `winit::keyboard::{PhysicalKey, Key, KeyLocation}` /
  `winit::event::ElementState`, so a module destructuring a `surge_sdk::KeyEvent` matches its own `winit`'s
  types against `surge-sdk`'s. Fix, symmetric to the `nalgebra` one: `surge-sdk` also does `pub use winit;`,
  modules drop their own `winit` line and import `use surge_sdk::winit::{...}`, in the same step (1.6) for
  the same reason. `surge-player` keeps its own direct `winit` dependency — it needs the event loop and
  window types, which are no part of `surge-sdk`'s surface, and it is not the crate exchanging these types
  across a boundary the SDK owns.
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
- A `SpriteCanvas` trait covering the module-facing instance API. Passes 1–12 wrote this as
  "`add_instance`, `upload_texture`, `get_create_info`, ..." and never resolved the ellipsis — the same
  gap Q35 closed for `KeyEvent`. Pinned down here (open question 69, resolved, added pass 13), taken
  from `SpriteDatabase`'s actual current signatures rather than invented:
  ```rust
  pub trait SpriteCanvas {
      /// Queue one sprite for this frame. Occupancy resets after every draw, exactly as
      /// `SpriteDatabase::add_instance` behaves today — a sprite not re-added disappears.
      fn add_instance(&mut self, aii: &InstanceInfo);

      /// Upload a PNG by path and return its assigned `texture_id`.
      ///
      /// Fallibility is asymmetric between backends: `SpriteDatabase` genuinely fails once
      /// `max_textures` is reached, while the dev-mode proxy is fire-and-forget and can only ever
      /// return `Ok` — a real dev-mode failure surfaces player-side, after this call returned, and
      /// is routed through the crash path (Q11/Q17) instead of back through this `Result`.
      fn upload_texture(&mut self, texture_file: &str) -> Result<usize, CanvasError>;

      /// The `CreateInfo` the canvas was built with. By value, matching
      /// `SpriteDatabase::get_create_info` (`CreateInfo` is `Copy`); the dev proxy answers from its
      /// cached copy with no round trip (Q27).
      fn get_create_info(&self) -> CreateInfo;
  }
  ```
  That is the complete surface: it is exactly the three methods the two real modules call on their
  `SpriteDatabase` parameter (checked across both modules' sources), and nothing more. `draw()` is
  deliberately absent — `SurgeContext` owns the concrete `SpriteDatabase` and calls `draw()` on it
  directly; no module has ever called it. `decode_texture_file`/`upload_decoded_texture`/`DecodedTexture`
  are absent for the reason Q36 gives. Object safety was compile-checked in pass 6's spike (Q33). Two
  backends:
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
  a future `.current_dir(...)` addition to the spawn call. **Pass 9 note (open questions 49, 51):**
  `init_env_logger`'s body depends on `env_logger`/`log` directly, so its move needs those two added to
  `surge-sdk/Cargo.toml` (not previously assigned to any step), and both real modules already call it once
  themselves from inside `on_load` today — that call has to come *out* once `src/bin/dev.rs` starts calling
  it once per process at startup instead, or the module's own logger init runs twice and panics. See step 3.2.
- **Config parsing** (open question 54, resolved, added pass 10): `surge-core/src/config/mod.rs`
  (`EngineConfig` and friends, `parse_config`) and `ConfigError` move here too, on the same "no Vulkan
  coupling, module-shaped consumer needs it" grounds as the two items above — except the consumer that
  needs it isn't a dev-mode module (no module ever sees `EngineConfig` in this design, in either mode; only
  the player interprets `config.toml`) but the *release* `[[bin]]` target's own `main()`, which has to
  produce the `EngineConfig` value `surge_player::run` takes as its second argument and, unlike
  `surge-player`'s dev binary, has no `surge-core` dependency to get it from. `surge-core` re-exports both
  (`pub use surge_sdk::config;` in place of today's `pub mod config;`, plus a `pub use` for `ConfigError`)
  so nothing inside `surge-core` itself or `surge-player`'s existing `sc::config::parse_config` call needs
  to change. See step 4.1, where this is assigned — it has no reason to move earlier than Phase 4, since
  nothing before it needs a module to see `EngineConfig` at all.

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
dependency/feature nits). Pass 9 pushed the same "assumes machinery no step adds" check one level further —
against the actual current source of the files each step names, not just the steps around it — and found two
more instances of it in Cargo dependency wiring the not-yet-created `surge-sdk` crate needs (49) and in the
free helper functions both real modules route their retyped trait methods through (50); one instance of a
step's own stated action ("swap the dependency") leaving old call sites behind unconverted, one of which is a
latent panic, not just a compile error (51); one instance of the design prescribing its own fix for a bug it
found and then never assigning that fix to a step (52); and one visibility mismatch between the plan's literal
example code and `surge-mod-2048`'s actual current privacy modifiers that `surge-mod-default` happens not to
share (53). Pass 10 checked two more things no earlier pass had checked: whether Phase 4's release binary can
actually produce every value the code it calls requires, not just whether the values it names are spelled
correctly (54 — it can't, `EngineConfig` is unreachable from a module crate as this doc currently scopes each
crate's dependencies), and whether a factual claim the plan makes about the *current* state of the repository
(as opposed to a claim about what a step needs to add to it) is actually true against that repository (55 —
it isn't; the workspace already has tests, contrary to both this plan's step 1.2 and `CLAUDE.md`'s own current
text). Pass 11 checked two things neither of those: whether a step's own "add crate X" instruction actually
resolves the specific functions/derives that step's own code calls, verified against X's real `[features]`
table rather than assumed from its name (56, 57 — `postcard`'s `use-std` and `serde`'s `derive` are both
off by default and neither step 1.2 nor step 4.1's `thiserror` omission was ever checked against a real
`Cargo.toml`), and whether two fixes bundled into one commit actually agree with each other once combined,
not just each individually sound (58 — Q50 and Q36, both folded into step 1.6, contradict each other for
`upload_image_assets`). Pass 12 asked what the build and the OS actually *produce and name* at each step,
rather than what source each step compiles, and found the first item since Q54 that would derail an
implementer mid-phase (59 — Cargo names binaries after files, so both modules' dev binaries collide at
`target/debug/dev` and neither matches the `app_name` the player resolves; verified by running Cargo, not by
reading it), one lifecycle call the reload path never delivers (60), one crate the SDK re-export fix
skipped even though its exposure is identical and growing (61 — `winit`, next to Q52's `nalgebra`), one
process-wide startup call no release binary ever makes (62), one trait method whose two backends return
different values for the same call sequence (63), and three smaller precision gaps (64). Pass 13 turned
the question on the plan as a whole rather than on any step: not "does step N compile given 1..N-1" (pass
8) or "does step N name real things" (passes 9–12), but "if all of it were executed faithfully, which
sentences of this design would still be unimplemented at the end." That found two requirements with no
commit at all — the shutdown round trip (65) and Q11's crash path (66), the latter referenced by three
steps as though it existed — plus Q10's unassigned stall logging and its collision with crash detection
(67), two compile-blocking omissions (68: missing `Clone`/`Copy` on the wire's own payload types; 70: a
deleted re-export two crates import through), the trait this doc has named for eleven passes without ever
defining (69), the release goal violated one crate over from where Q20 defended it (71), unowned binary-path
resolution and the dev config value the smoke tests silently depend on (72), and five smaller precision
gaps (73). Pass 14 asked the mirror of pass 13's question — not "does every behavior have a commit" but
"does every commit have a test that could actually fail" — and found four code paths no test in the plan
ever executes (74: `PR_SET_PDEATHSIG`; 75: the `SIGKILL`/`ShutdownAck` timeout escalations; 77: handshake
mismatch and the stall-vs-fault rule), one fix the plan itself admitted was unobservable and left that way
(76), a gate that never ran the suites the plan authors (78), four steps with no stated test at all (79),
five steps that deferred verification without naming where it lands (80), and three test-precision gaps
(81). All 81 items are now resolved.

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
    `texture_id` from upload order. **Pass 12 correction (Q63):** the dev proxy's prediction is a
    *cumulative* counter reset at each `CreateInfo` push, not this turn's buffer index — as originally
    worded it disagreed with the release backend's return value for any upload outside
    `on_load`/`on_swapchain_recreate`. Fix at the source instead of adding wire-protocol reconciliation:
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
    trait's other signatures. **Pass 12 note (Q60):** this signature is kept, but it has a consequence this
    item didn't trace — with no canvas argument, `reload()` cannot send the `Request::OnLoad` the connect
    sequence requires of a freshly-connected child, so the respawned module would only ever see
    `OnSwapchainRecreate`. See Q60 for the `pending_on_load` fix, which leaves `reload()`'s signature alone.
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

### Added in pass 9

49. **`surge-sdk`'s own `Cargo.toml` never gains the dependencies the plan's later steps assume it already
    has** — resolved. Two separate omissions, same shape as Q48's Cargo-nit pair, caught by actually reading
    `surge-core/Cargo.toml` and the real module `Cargo.toml`s rather than assuming a crate that doesn't exist
    yet will pick up whatever it needs. (a) Step 1.6 relocates the `SurgeApp` trait to `surge-sdk` and keeps
    `keyboard_event`/`mouse_button_event`/`mouse_wheel_event` naming `winit::event::{DeviceId, ElementState,
    MouseButton, MouseScrollDelta, TouchPhase}` (and, until 1.7 lands, `winit::event::KeyEvent` itself)
    directly in the trait signature — but no step adds a plain `winit` dependency to `surge-sdk/Cargo.toml`
    before that; 1.1 only creates an empty crate and 1.2 only adds `serde`/`nalgebra`/`postcard`. `cargo check
    -p surge-sdk` fails at 1.6 on an unresolved `winit` path, not because the design is wrong (Q33's spike
    already proved `sdk-spike` needs, and has, exactly this dependency) but because no commit was ever
    assigned to add it. (b) Step 3.2 relocates `init_env_logger` (`surge-core::cli`) to `surge-sdk`, but that
    function's body calls `env_logger::Builder`/`log::LevelFilter`/`log::Level` directly (checked against
    `surge-core/src/cli/mod.rs`), and neither `env_logger` nor `log` is ever added to `surge-sdk/Cargo.toml`
    by any step — same failure mode, one phase later. Fix: 1.6 adds `winit = "0.30.12"` (matching the version
    every other crate in the workspace pins) to `surge-sdk/Cargo.toml` in the same commit it first names a
    `winit` type; 3.2 adds `env_logger = "0.11.8"`/`log = "0.4.28"` (the same versions `surge-core/Cargo.toml`
    already pins) to `surge-sdk/Cargo.toml` in the same commit it moves `init_env_logger`'s body over.
50. **Step 1.6 retypes the three `SurgeApp` methods but not the free helper functions both real modules
    route those methods' `&mut SpriteDatabase` argument through** — resolved. Checked directly against the
    actual source, not the trait definition alone: `surge-mod-2048/src/lib.rs`'s
    `upload_image_assets(spdb: &mut SpriteDatabase, textures: &[DecodedTexture])` and
    `add_board_sprite(spdb: &mut SpriteDatabase, bg: &BoardGeometry)`, and `surge-mod-default/src/lib.rs`'s
    `upload_assets(spd: &mut SpriteDatabase)`, are ordinary free functions — not trait methods — called from
    inside `on_load`/`on_swapchain_recreate`/`update` with the trait method's own parameter passed straight
    through (e.g. `upload_image_assets(spdb, self.get_cached_textures())` inside
    `on_load(&mut self, spdb: &mut SpriteDatabase)`). Once 1.6 retypes `on_load`'s parameter to `&mut dyn
    SpriteCanvas`, every one of these call sites passes a `&mut dyn SpriteCanvas` value to a function whose
    signature still reads `&mut SpriteDatabase` — a type mismatch, not a coercion (unsized coercion only
    applies at the call site itself, concrete-to-`dyn`, not through an intermediate function boundary). 1.6's
    own text ("their `impl SurgeApp for ...` blocks change the ... three method signatures") describes exactly
    the part of each module that *doesn't* need to change and is silent on the part that does. Fix: 1.6's
    per-module change list grows to include retyping these three helper functions' parameter to `&mut dyn
    SpriteCanvas` too — mechanical, no logic change, same commit (splitting it out would leave 1.6 not
    compiling in between, the same shape of problem Q36's fold-in already established for this exact commit).
51. **Step 3.2 leaves a redundant, panic-inducing `init_env_logger()` call behind in both modules, and
    doesn't sweep their other remaining `surge_core::`-qualified call sites** — resolved. Both real modules
    already call the logger-init function directly from inside `on_load` today — `surge_core::
    cli::init_env_logger()` at `surge-mod-2048/src/lib.rs:82` and `surge-mod-default/src/lib.rs:31` (this is
    literally the behavior `CLAUDE.md`'s "Module ABI" section describes: "Modules call
    `surge_core::cli::init_env_logger()` themselves in `on_load`"). 3.2 adds a *second*, new call to the same
    (now-relocated) function in each module's `src/bin/dev.rs::main()`, ahead of `run_dev_module`, but never
    removes the original one inside `on_load` — so a normal dev-mode run calls `init_env_logger()` twice in
    the same process (once at process startup, once when the player's `Request::OnLoad` round-trips through
    to `on_load`). `env_logger::Builder::init()` calls through to `log::set_boxed_logger`, which can only
    succeed once per process; the second call panics — this would surface immediately on the very first smoke
    test 3.2 itself specifies, not as a latent bug, but it's still a real gap in what the step's text says to
    do. On top of that: once 3.2 removes the `surge-core` dependency from a module's `Cargo.toml` ("swap the
    `surge-core` dependency for `surge-sdk` only"), *every* remaining `surge_core::`-qualified path anywhere
    in that module's source stops resolving, not just the two named functions — concretely, `surge-mod-2048`'s
    post-1.6 `on_load`/`on_swapchain_recreate` bodies and `surge-mod-default`'s `upload_assets` still call
    `surge_core::assets::resolve_asset_path(...)` (Q32 doesn't move that call site until 3.2 itself), so the
    dependency swap and the call-site rewrite have to land in the same commit or the module doesn't compile in
    between. Fix: 3.2's per-module work item grows two clauses: delete the `init_env_logger()` call from
    inside `on_load` entirely (init now happens exactly once, in `src/bin/dev.rs`'s `main()`, before
    `run_dev_module` is ever called — `on_load` no longer needs to know logging exists), and rewrite every
    remaining `surge_core::`-qualified path in the module's source (`resolve_asset_path` being the only other
    one, per the grep already done for this open question) to `surge_sdk::`, in the same commit as the
    `Cargo.toml` swap.
52. **The design's own fix for the `nalgebra` version-skew risk it identified is never assigned a step, and
    `surge-sdk`'s promised "math re-exports" are never given a shape** — resolved. The `surge-sdk` section
    (see the `InstanceInfo`/`SubTextureInfo` bullet) already names the bug precisely: both modules declare
    their own `nalgebra = "0.34.1"` dependency rather than sourcing it through `surge-sdk`, which "only
    type-checks today because the version string happens to match everywhere," and a future bump could
    silently produce two incompatible `Vector2` types at the `InstanceInfo` boundary — and it prescribes the
    fix in the same breath ("Modules should drop their own `nalgebra` dependency and use `surge_sdk`'s
    re-exported math types exclusively"). It's also named as one of `surge-sdk`'s scoped responsibilities in
    the very next subsection's own header ("Asset-path helpers, logger init, math re-exports"). But nothing
    after that ever says what `surge_sdk`'s math re-export actually looks like, and no step in the
    Implementation plan removes either module's own `nalgebra` line from its `Cargo.toml` or repoints its `use
    nalgebra::...` imports — the bug this doc identified in its own review survives this doc's own
    implementation plan untouched. Fix: `surge-sdk` re-exports the whole crate (`pub use nalgebra;` — the
    simplest option, and one that needs no curation as `nalgebra`'s own API surface grows) so module code
    becomes `use surge_sdk::nalgebra::{Vector2, Vector4};` rather than a separate direct dependency; 1.6 (the
    step where each module first gains a `surge-sdk` path dependency and already has to touch every
    `InstanceInfo`/`SubTextureInfo`-constructing call site for the `SpriteCanvas` retyping) is where the
    `nalgebra` line comes out of both modules' `Cargo.toml`s and their `use nalgebra::...` imports become `use
    surge_sdk::nalgebra::...` — no later step touches these call sites again, so doing it any later would mean
    revisiting already-finished code for no reason.
53. **`surge-mod-2048`'s `App2048` is crate-private, so the plan's own example code for its dev entry point
    doesn't compile — an asymmetry the plan's shared wording hides** — resolved. Checked directly against the
    current source: `surge-mod-2048/src/lib.rs` declares `mod app_2048;` (private — no `pub`), and
    `app_2048.rs` declares `pub(crate) struct App2048` with a `pub(crate) fn new(...)`. A `src/bin/*.rs` file
    in the same Cargo package is compiled as a separate crate that depends on the package's `[lib]` target the
    way an external crate would — `pub(crate)` items are invisible to it, same as to any other crate, and a
    private `mod` compounds that. So 3.2's literal text for `surge-mod-2048/src/bin/dev.rs` —
    `surge_sdk::run_dev_module(Box::new(App2048::new()))` — doesn't compile: neither `app_2048` nor
    `App2048::new` is reachable from outside the library crate. The same problem recurs, unstated, at 4.2's
    `src/bin/release.rs`. `surge-mod-default` doesn't share this problem — `AppDefault` is declared `pub
    struct AppDefault {}` directly in `lib.rs` — which is exactly why nobody hit this reading the doc's shared
    "every module's dev `main()` is one line" framing (Q24) against only the module whose visibility happens
    to already work. Fix, and the minimal one: neither entry point needs to name the concrete app type at all.
    Both modules already export `#[unsafe(no_mangle)] pub fn surge_register_app() -> Box<dyn SurgeApp>` — a
    fully public function returning exactly the `Box<dyn SurgeApp>` both `run_dev_module` and (at 4.2)
    `surge_player::run` want. 3.2 and 4.2's example code become
    `surge_sdk::run_dev_module(surge_mod_2048::surge_register_app())` and
    `surge_player::run(surge_mod_2048::surge_register_app(), config)` respectively (substituting the crate
    name per module) — zero visibility changes to either module's internals, and it keeps the one entry point
    uniform across both real modules instead of two different constructions that happen to both compile for
    different reasons. Related cleanup, not a blocker: once 3.4 deletes `load_from_dylib` (the only caller
    that ever needed `surge_register_app` to be an `unsafe(no_mangle)`, dynamically-resolved symbol), the
    attribute itself becomes vestigial — worth a one-line mention in 3.4's own text so a future pass doesn't
    have to re-discover it, the same way Q29 traced `AppError`/`libloading` through to their actual dead end
    instead of leaving it implicit.

### Added in pass 10

54. **The release `[[bin]]` target has no way to obtain the `EngineConfig` value `run()` requires** —
    resolved. 4.2's example code is `surge_player::run(surge_mod_2048::surge_register_app(), config)`, and
    4.1 fixes `run`'s signature as `pub fn run(app: Box<dyn SurgeApp>, config: EngineConfig)` — but no step
    anywhere says where `release.rs`'s `config` local comes from, and checked against the actual crate
    graph this doc specifies, there's no legal way for it to exist. `EngineConfig`, `parse_config`, and
    `ConfigError` are defined in `surge-core/src/config/mod.rs` and `surge-core/src/errors/mod.rs` today;
    `surge-player`'s own dev binary can call `sc::config::parse_config("config.toml")` because it has always
    depended on `surge-core` directly and keeps doing so under this design. `release.rs`, however, is a
    `[[bin]]` target living on the *module's own* package, and this doc is explicit throughout ("Modules
    depend on `surge-sdk`, not `surge-core`, in both modes") that a module crate never gets a `surge-core`
    dependency in either mode — 4.2 only ever gives it `surge-sdk` (always) and `surge-player` (optional,
    behind `release-bin`, for `run()` itself). Cargo's dependency rules don't let a crate name a
    transitive dependency it hasn't listed directly, so `release.rs` can no more write
    `surge_core::config::parse_config(...)` than it could write `surge_core::vulkan::...` — and
    `surge-player` doesn't re-export `config` either, so going through it isn't an option as specified.
    This is the same "step N assumes machinery no step actually adds" shape pass 8 and 9 kept finding, just
    aimed somewhere neither pass's own method happened to check: pass 7 checked call sites a module's
    `impl SurgeApp` makes, pass 8 checked step-to-step Cargo-graph sequencing, pass 9 checked `surge-sdk`'s
    own `Cargo.toml` and two modules' visibility modifiers — none of the three asked "can the thing calling
    `run()` actually produce every argument `run()`'s own signature demands." Fix, in the same spirit as
    Q26/Q32 (both already moved Vulkan-free, module-needed pieces of `surge-core` into `surge-sdk`):
    `config`'s entire module (`WindowConfig`, `ResolutionConfig`, `RendererConfig`, `ClearColorConfig`,
    `StartupAppConfig`, `EngineConfig`, `parse_config`) and `ConfigError` (its only other caller, checked —
    `grep -rn ConfigError` turns up nothing outside `config/mod.rs` and its own definition in
    `errors/mod.rs`) move to `surge-sdk` wholesale; neither touches `ash`/`vk-mem`/anything Vulkan-shaped
    (checked directly — `config/mod.rs`'s only non-`std` imports are `serde`, `toml`, and `log`, all either
    already a `surge-sdk` dependency by this point in the plan or a one-line addition: `log` is already
    there since 3.2 (Q49b), `toml = "0.9.8"` — matching `surge-core/Cargo.toml`'s pin — is new). `surge-core`
    keeps working unchanged: `surge-core/src/lib.rs`'s `pub mod config;` becomes `pub use surge_sdk::config;`,
    the same pure compatibility-shim move step 1.3 already used for the six plain-data types, so all four
    existing `use crate::{config::EngineConfig, ...}` call sites inside `surge-core`'s Vulkan layer
    (`image.rs`, `ctx_swpc.rs`, `ctx_command.rs`, `ctx_new_drop.rs` — checked, that's the complete list) need
    no edits at all. `errors/mod.rs` loses `ConfigError`'s definition the same way (a `pub use
    surge_sdk::config::ConfigError;`, or wherever pass 10's fix lands it, standing in for the deleted local
    enum). `surge-player`'s own dev binary needs no change either — `sc::config::parse_config` still resolves,
    now through the re-export, exactly as `sc::vulkan::sprite_database::InstanceInfo` already does since 1.3.
    `release.rs` becomes `let config = surge_sdk::config::parse_config("config.toml").unwrap();
    surge_player::run(surge_mod_2048::surge_register_app(), config);` — two lines, no crate `release.rs`
    doesn't already have access to. Since nothing before Phase 4 needs a module process to see `EngineConfig`
    at all (dev-mode modules never touch it — the player alone interprets `config.toml`, per this doc
    throughout), this move has no reason to land earlier than the step that first needs it; see 4.1 below,
    which is where it's now assigned, matching this doc's own established practice (3.2's `init_env_logger`
    move, 1.4's `SpriteCanvas` definition) of landing a relocation exactly where it becomes load-bearing, not
    earlier.
55. **Step 1.2's "first test anywhere in the workspace" claim, and Phase 5.2's "the stale 'no tests' line 1.2
    already touched," are both false today, independent of this doc** — resolved. Checked directly against
    `surge-core/src/vulkan/sprite_database/mod.rs` rather than against `CLAUDE.md`'s summary of it (the same
    category of check Q49/Q53 ran, just aimed at a claim about the repository's *current* state rather than a
    claim about what a future step needs to add): that file already carries a `#[cfg(test)] mod tests` block
    with three tests — `full_z_range_lands_inside_vulkan_depth_bounds`, `smaller_world_z_draws_in_front`,
    `gpu_struct_layout_matches_glsl` — added in a commit unrelated to this doc, well before this design's
    first pass. `CLAUDE.md`'s "There are currently no tests anywhere in the workspace" line is therefore
    already stale *today*, not a claim step 1.2 would be the first to falsify, and step 1.2's own new test
    (in `surge-sdk`, round-tripping `InstanceInfo` through `postcard`) is at best the first test *outside*
    `sprite_database/mod.rs`, not the first test in the workspace. No compile-blocking consequence — the
    existing three tests exercise `make_model_matrix`/`make_view`/`make_ortho_projection` and two `#[repr]`
    struct sizes, none of which step 1.2 or 0.2 touches — this is a prose-accuracy fix, not a sequencing one.
    Fix: 1.2's text is corrected below to stop claiming a "first test in the workspace," and to fix
    `CLAUDE.md`'s line as an already-overdue correction this commit happens to be well-placed to make (it's
    already touching `surge-sdk`'s own new test), not as a side effect of newly making the line stale.

### Added in pass 11

56. **Step 1.2 adds `postcard` and `serde` to `surge-sdk/Cargo.toml` by name only, missing the feature flags
    both crates need for what 1.2's own code actually calls** — resolved. Checked directly against the real
    `[features]` tables of both crates (`postcard 1.1.3`, `serde 1.0.228`, both present in this machine's
    local registry cache — not assumed from memory or from how other crates in this workspace happen to use
    them). Two separate omissions, same shape as Q48/Q49's Cargo-dependency nits, just aimed at feature flags
    instead of whole dependencies: (a) `postcard::to_stdvec` — which 1.2's own `#[test]` calls directly to
    round-trip an `InstanceInfo` — is behind `#[cfg(feature = "use-std")]`, and postcard's `default =
    ["heapless-cas"]` does not include `use-std`; without it, `to_stdvec` simply doesn't exist as far as the
    compiler is concerned. (b) `#[derive(Serialize, Deserialize)]`, applied to every one of 1.2's own six
    moved types (`BlendingMode`, `TextureFilteringMode`, `TextureFilteringLevel`, `CreateInfo`,
    `SubTextureInfo`, `InstanceInfo`), needs serde's `derive` feature — serde's `default = ["std"]` does not
    include it either. `surge-core/Cargo.toml` already gets this right for its own `serde` line (`serde = {
    version = "1.0.228", features = ["derive"] }`), which is exactly why nobody hit this reading the doc
    against the one crate that already works — the same blind spot Q53 named for `surge-mod-2048` vs.
    `surge-mod-default`'s visibility, just recurring here for Cargo features instead of `pub`/`pub(crate)`.
    Neither gap is downstream noise: `cargo check -p surge-sdk` (and `cargo test -p surge-sdk`) fails at 1.2
    itself, the very first commit that gives `surge-sdk` any real content — not at 2.1 or later, where a
    reader might expect wire-format bugs to surface first. `postcard::from_bytes` (also used by 1.2's test)
    is *not* feature-gated — checked directly, it carries no `#[cfg]` at all — so it needs no fix; only the
    serialize half and the derive macros do. Fix: 1.2's dependency line becomes `serde = { version =
    "1.0.228", features = ["derive"] }` (verbatim what `surge-core/Cargo.toml` already uses) and `postcard =
    { version = "1.1.3", features = ["use-std"] }`. Landing the feature here, on `surge-sdk`'s single
    `postcard` dependency line, is also sufficient for every later use of `postcard` in this doc (2.1's
    `write_message`/`read_message` framing helpers, the full `Request`/`Response` wire types) — a Cargo
    feature is a property of the dependency edge, not of each individual call site, so nothing downstream
    needs its own fix once this one lands.
57. **Step 4.1 moves `ConfigError` into `surge-sdk` without adding the dependency it derives from** —
    resolved. 4.1's text adds `toml = "0.9.8"` to `surge-sdk/Cargo.toml` for the moved `config` module
    ("`log` is already there since 3.2") but never mentions `thiserror` — checked directly against
    `surge-core/src/errors/mod.rs`'s current `ConfigError` definition, it's `#[derive(Debug, Error)]` against
    `thiserror::Error`, exactly like every other error enum in that file (`AppError`, `ModuleError`,
    `VulkanError`). `thiserror` is a `surge-core`-only dependency today — checked, `grep -rn thiserror
    **/Cargo.toml` across the workspace turns up nothing else — and nothing before 4.1 ever adds it to
    `surge-sdk`. Without it, `cargo check -p surge-sdk` fails at 4.1 the moment `ConfigError`'s own derive is
    compiled, the same failure shape as Q49's `winit`/`env_logger` omissions and Q56 above, just one more
    dependency edge nobody happened to check against the type actually being moved. Fix: 4.1 also adds
    `thiserror = "2.0.17"` (matching `surge-core/Cargo.toml`'s existing pin) to `surge-sdk/Cargo.toml`, in the
    same commit that adds `toml`.
58. **Step 1.6 bundles two individually-correct fixes (Q50, Q36) that contradict each other for one specific
    function** — resolved. Q50's fix, as written, lists three helper functions that "also have to retype
    their parameter to `&mut dyn SpriteCanvas`... mechanical, no logic change, same commit":
    `upload_image_assets`/`add_board_sprite` (`surge-mod-2048`) and `upload_assets` (`surge-mod-default`). But
    the same step 1.6 also bundles in Q36's fix, which retires `surge-mod-2048`'s `DecodedTexture`-based
    decode cache outright — deletes `cached_textures` (`app_2048.rs`), deletes `decode_image_assets`
    (`lib.rs`), and has `on_load`/`on_swapchain_recreate` call `SpriteCanvas::upload_texture(path)` directly
    per asset name instead. `upload_image_assets`'s own signature is `(spdb: &mut SpriteDatabase, textures:
    &[DecodedTexture])` — checked directly against `surge-mod-2048/src/lib.rs`'s current source — and once
    Q36's half of this same commit removes `decode_image_assets`/`cached_textures`, nothing in the module
    produces a `&[DecodedTexture]` for it to receive, and `SpriteCanvas` (per its own sketched surface —
    `add_instance`, `upload_texture`, `get_create_info`) has no method that accepts one either: it only takes
    an asset-path string. "Retype the parameter, mechanical, no logic change" is simply wrong for this one
    function — it doesn't survive this commit in any form, retyped or otherwise, and an implementer following
    Q50's literal text would try to keep a signature naming a type (`DecodedTexture`) the same commit's own
    Q36 clause just stopped importing. `add_board_sprite` and `surge-mod-default`'s `upload_assets` are
    unaffected by this correction — neither touches `DecodedTexture`, so Q50's "mechanical, no logic change"
    description already holds for both exactly as written. Fix: strike `upload_image_assets` from the "just
    retype" list. In its place — pinned down here rather than left for an implementer to invent, this doc's
    own established practice (e.g. Q35's `KeyEvent` field list) — 1.6 adds a small replacement helper,
    `fn upload_board_assets(canvas: &mut dyn SpriteCanvas)`, looping over the same 12 filename literals
    `decode_image_assets` used to hardcode (`"board.png"` through `"pieces_2048.png"`), calling
    `surge_sdk::resolve_asset_path(MODULE_NAME, name)` then `canvas.upload_texture(path.to_str().unwrap())`
    per name — replacing the `upload_image_assets(spdb, self.get_cached_textures())` call in both `on_load`
    and `on_swapchain_recreate`. This is Q36's fix stated precisely enough to implement rather than a new
    decision; Q36 already established the "no `SpriteCanvas` method does what `upload_decoded_texture` did"
    reasoning, this item only traces that reasoning through to the specific helper function Q50 had
    (incorrectly) assumed would keep its shape.

### Added in pass 12

59. **Cargo names binaries after their source file, so both modules' dev binaries are `target/debug/dev` —
    and neither is the path `[startup_app] app_name` resolves to** — resolved. The only item in this doc so
    far checked by *running* the build tool rather than reading source: a throwaway two-package workspace,
    each package with a `src/bin/dev.rs` and no `[[bin]]` table, was built with `cargo build --workspace`.
    Result: `warning: output filename collision at target/debug/dev ... the bin target 'dev' in package
    'mod-b' has the same output filename as the bin target 'dev' in package 'mod-a' ... this may become a
    hard error in the future`, both compile, and the file left at that path is whichever linked last
    (confirmed by running it). Steps 3.2 and 3.3 give `surge-mod-2048` and `surge-mod-default` exactly this
    shape, and 4.2 repeats it for `src/bin/release.rs` → `target/release/release`. Two distinct
    consequences, either one enough to stop an implementer: (a) 3.3's own smoke test
    (`SURGE_DEV_LOADER=process` with `config.toml` pointed at `surge_mod_default`) would spawn whichever
    module's binary happened to link last — a *silently wrong module*, not a build failure, and 3.2's test
    passing is no protection since it runs before the collision exists; (b) the doc's own resolution rule,
    `{app_folder}/{app_name}` with "no `lib`/`.so` decoration", resolves against the real checked-in
    configs to `surge_mod_2048`/`surge_mod_default` (checked: both `surge-modules/*/config.toml`), and
    nothing in the plan ever produces a file by either name. Neither passes 7–11's step-sequencing checks
    nor their source checks could see this — it isn't a property of any source file, it's a property of
    what Cargo emits and what the player then `exec`s. Fix: every module crate declares explicit `[[bin]]`
    tables (see "Module discovery / config schema" above for the literal TOML) — the dev binary named
    exactly its `app_name` (`surge_mod_2048`, `surge_mod_default`), the release one suffixed
    `_release`, both with an explicit `path` so the source files keep their descriptive
    `src/bin/dev.rs`/`release.rs` names. A `[[bin]]` sharing the package's `[lib]` name is legal (it's the
    default `src/main.rs` + `src/lib.rs` arrangement), so nothing else has to be renamed.
60. **After a reload, the respawned module process never receives `Request::OnLoad`** — resolved. Q45 fixed
    "how does a keypress reach `RemoteApp::reload()`" with a default `fn reload(&mut self) -> bool` on
    `SurgeApp`, and Q14 requires the player's `SpriteDatabase` to be rebuilt on every reload — but tracing
    the two through the actual sequence (F5 in `window_event` → `reload()` → `recreate_swapchain = true` →
    next `about_to_wait` rebuilds the database → `on_swapchain_recreate`) shows the fresh child's first
    ever `Request` is `OnSwapchainRecreate`. It cannot be `OnLoad`: `reload()` takes no canvas, and per Q44
    a `create_info` can only come from `canvas.get_create_info()`. That contradicts the connect sequence's
    own "on a match, the player proceeds with `Request::OnLoad`" and Q14's "before replaying the fresh
    module's `on_load`" — a module doing genuine one-time init in `on_load` (the ordinary reading of that
    hook) comes back half-initialized after every reload. It is invisible to 3.5's smoke test for the same
    reason Q46 flagged there: both real modules happen to do the same asset upload in both hooks, so 2048's
    board redraws correctly and nothing looks wrong. This is the Q58 class again — Q44, Q45 and Q14 are
    each individually correct, and their *composition* is what fails. Fix: a `pending_on_load` flag inside
    `RemoteApp`, set when `reload()`'s re-handshake succeeds and consumed by the next
    `on_swapchain_recreate` (guaranteed to be called, since `reload()` returning `true` is what schedules
    it), which sends `Request::OnLoad { create_info }` in place of `Request::OnSwapchainRecreate`. No new
    wire message, no signature change, no module-side change; see the "Reload" bullet under Dev mode and
    step 3.5.
61. **Q52's version-skew fix was applied to `nalgebra` and not to `winit`, whose exposure is identical and
    grows at 1.7** — resolved. Both modules declare their own `winit = "0.30.12"` alongside the `nalgebra`
    line Q52 removes (checked, both `Cargo.toml`s), and both name winit types directly in `SurgeApp` method
    signatures (`DeviceId`, `ElementState`, `MouseButton`, `MouseScrollDelta`, `TouchPhase`) implemented
    against a trait `surge-sdk` owns — the same "compiles only because the version strings happen to match
    everywhere" situation, one crate over. It gets worse at 1.7 rather than better: `surge_sdk::KeyEvent`'s
    fields are themselves `winit::keyboard::{PhysicalKey, Key, KeyLocation}` and
    `winit::event::ElementState`, and `surge-mod-2048` destructures exactly those
    (`PhysicalKey::Code(KeyCode::ArrowUp)`, `lib.rs:126-132`). Three passes stared at the `nalgebra`
    paragraph without asking whether the argument generalizes to the other shared external crate in the same
    `Cargo.toml`. Fix: `surge-sdk` adds `pub use winit;` next to `pub use nalgebra;`, both modules drop their
    own `winit` dependency line and switch to `use surge_sdk::winit::{...}` — same commit as the `nalgebra`
    switch (1.6), which is already rewriting those `use` lines. `surge-player` keeps its own `winit`
    dependency: it needs `EventLoop`/`Window`/`ApplicationHandler`, none of which `surge-sdk` re-exports as
    part of its module-facing surface.
62. **No release binary ever initializes a logger or prints the logo** — resolved. `init_env_logger()` and
    `print_logo()` are called from `surge-player`'s `main()` (`main.rs:327,332`, verified). 4.1 turns that
    `main()` into the *dev* binary's entry point and 4.2's release `main()` is two lines — `parse_config`
    plus `surge_player::run(...)` — so a shipped game installs no logger at all: every `log::error!` in
    `surge-core`'s Vulkan layer, and every `log::info!` in the module, is silently discarded, and today's
    staged player behavior (logo + logs) regresses with nothing in the plan noting it. Q42 traced this same
    call site through the Q32 *move* and fixed where it points; nobody asked which binaries still *reach*
    it once `main.rs` stops being the only entry point. Fix: both calls move into `surge_player::run()`
    itself, the entry point both modes share, and out of `surge-player`'s dev `main()` in the same commit
    (4.1) — moved, never duplicated, since a second `env_logger::Builder::init()` in one process panics,
    which is precisely Q51's failure one crate over. `print_logo` staying in `surge-core` (3.2's decision)
    still holds: `run()` lives in `surge-player`, which depends on `surge-core` directly.
63. **The dev proxy's `texture_id` counter restarts per call while the real backend's index accumulates** —
    resolved. `SpriteCanvas::upload_texture` returns `Result<usize, CanvasError>` (Q17/Q28), documented as
    "the assigned index." Under the release backend that is `SpriteDatabase`'s cumulative
    `uploaded_textures` position; under the dev proxy, as worded through pass 11 ("this call's position in
    `CanvasOps.texture_uploads`"), it restarts at 0 on every turn. The two agree only when the canvas held
    no textures at the start of the call. Both real modules upload solely from
    `on_load`/`on_swapchain_recreate`, which the player always calls against a freshly-built database, so
    the divergence is latent today — but the trait permits uploading from `update()`, nothing warns, and
    the failure is a wrong sprite silently drawn, not an error. Fix: the proxy keeps a cumulative counter
    for the life of the connection and resets it only when it caches a new `CreateInfo` off
    `Request::OnLoad`/`OnSwapchainRecreate` — exactly the two moments the player rebuilds the database, so
    the two backends agree by construction rather than by module convention. Folded into the "`texture_id`
    handshake" bullet, the `CanvasOps` doc comment, and step 2.2.
64. **Three smaller precision gaps** — resolved. (a) **`surge-core`'s `serde` and `toml` dependencies go
    dead and no step removes them.** After 1.2 moves the six plain-data types and 4.1 moves `config`/
    `ConfigError`, `surge-core`'s only `use serde` (`config/mod.rs:3`) and only `use toml`
    (`config/mod.rs`, `errors/mod.rs`) are gone — checked by grep, there is no other user in the crate.
    Not compile-blocking, but it is exactly the trace-through Q29 did for `libloading`, left undone for the
    two dependencies Phase 4 orphans; 4.1 removes both lines. (b) **1.6's `DecodedTexture` cleanup names one
    import, and there are two files.** Q36/Q58 say `surge-mod-2048` drops "the `DecodedTexture` import" —
    the type is imported in `lib.rs`'s `use surge_core::{...}` block *and* separately at
    `app_2048.rs:4`, and `app_2048.rs` additionally imports `decode_image_assets` from the crate root
    (`use crate::{..., decode_image_assets, ...}`), initializes `cached_textures: decode_image_assets()`
    inside `App2048::new()`, and exposes a `get_cached_textures()` accessor whose only callers are the two
    `upload_image_assets` call sites Q58 deletes. All of it goes in 1.6; spelled out so an implementer
    doesn't remove one import, keep the accessor, and be left with a field that no longer exists. (c)
    **"Same `[startup_app]` shape in both dev and release" overstates what release does with it.** A
    release binary is its own module — it never resolves `app_folder`/`app_name` at all. The keys stay in
    the shipped `config.toml` regardless, because `EngineConfig` deserializes `[startup_app]` as a
    non-optional field (checked, `StartupAppConfig` is a plain required member) and `parse_config` would
    fail without it; the prose above is corrected to say inert-but-required rather than
    resolved-differently.

### Added in pass 13

65. **No step on either side implements `Request::Shutdown`, so `on_unload` is silently never delivered
    again** — resolved. Q16 folded `on_unload` into `Shutdown`/`ShutdownAck` in pass 3 and every later pass
    treated that as settled *design*, which it was — but the question this pass asks is which commit
    *builds* it, and the answer was none. 2.3's `drive()` is described as returning "after
    `Shutdown`/`ShutdownAck`" without ever calling `app.on_unload()`; 2.5's `RemoteApp` lists
    `on_load`/`on_swapchain_recreate`/`update` and the three buffered input methods and simply omits
    `on_unload` from its impl; nothing anywhere sends the `Request`. The result is not a compile error —
    `on_unload` has a default empty body on the trait, and `surge-player`'s `exiting()` keeps calling it on
    a `RemoteApp` that does nothing with it — so the cutover would silently drop a lifecycle hook that
    works today. It is observable, cheaply: `surge-mod-default::on_unload` logs "Default app shutdown"
    (`lib.rs:158-160`, checked), and that line would just stop appearing. Fix: the player half
    (`RemoteApp::on_unload` sends `Shutdown`, waits for `ShutdownAck` under a bounded read timeout, then
    escalates `SIGTERM`/`SIGKILL` and reaps) lands in 2.5; the module half (`drive()` calls
    `app.on_unload()` before replying `ShutdownAck`) lands in 2.3, with the scripted test extended to cover
    the arm; 3.3's smoke test watches for the log line, and the "standard smoke test" definition gains it
    from that point on. Same class as Q52 — a decision the design made correctly and the plan never
    assigned — but reached by a different question: not "was the fix given a shape" but "was the fix given
    a commit."
66. **Q11's crash-state path is referenced by three steps and implemented by none** — resolved, and the
    most serious item this pass found. "Routes to the Q11 crash path" appears in 2.5 (a `texture_uploads`
    failure), in Q17's part 2, and implicitly in 2.6/Q22 (the `reload_in_progress` check exists precisely
    to decide whether to *take* that path) — but Q11 itself resolved a *policy* ("stop driving the module,
    log loudly, make it visibly different from reload churn, e.g. freeze last frame + on-screen/log
    indicator"), never a mechanism, and no step builds one. Three concrete things were missing, each of
    which an implementer would otherwise have had to invent mid-phase: (a) *where the state lives* —
    `RemoteApp`, as a `faulted: bool` set on non-reload transport failure, handshake mismatch, or upload
    failure, making every subsequent trait method a no-op; (b) *how `SurgeContext` learns about it* — it
    holds only a `Box<dyn SurgeApp>` and cannot downcast, the same wall Q45 hit for `reload()`, so the
    answer is the same: a second default trait method, `fn is_faulted(&self) -> bool { false }`; (c) *what
    "visibly different" means in an engine with no text rendering* — `Window::set_title` gaining a
    `— MODULE CRASHED` suffix, plus skipping acquire/draw/present entirely so the last presented image
    stays on screen, which is what "freeze last frame" reduces to when nothing new is submitted. Recovery
    is F5 only, never an automatic respawn (Q11's own requirement). Fix: new step 3.6, placed after 3.5 so
    it can reuse the F5 keybind as its recovery test, with the flag itself introduced at 2.5 where the
    failures are actually detected. Its test is the first in this plan to deliberately break something
    (`kill -9` the child) rather than confirm a happy path.
67. **Q10's "loud logging while blocked" was never assigned either, and it collides with Q66's fault
    detection** — resolved. Q10 chose blocking backpressure "paired with clear, loud debug logging when the
    player is blocked waiting on the module," and no step mentions it. The reason it matters more than a
    missing log line: implementing it requires a read *timeout*, and a timeout produces exactly the kind of
    error return (`WouldBlock`/`TimedOut`) that Q66's fault detection watches the same call for. Built
    naively, either the stall logging never fires (blocking read, no timeout) or every slow module trips
    the crash state (timeout treated as EOF). Fix: one rule, stated once, in the single
    `write_request`/`read_response` helper pair — `WouldBlock`/`TimedOut` is a stall (log with elapsed ms,
    retry, stay connected); `Ok(0)`/EOF or a transport error is a fault. Folded into 2.5 alongside Q66's
    flag, since both are properties of that same helper.
68. **`InstanceInfo`/`SubTextureInfo` derive `Debug` only, so 2.2's proxy cannot buffer them** — resolved,
    compile-blocking. Checked against the real definitions
    (`surge-core/src/vulkan/sprite_database/mod.rs:67,93`): both are `#[derive(Debug)]`, nothing more.
    1.2 adds `Serialize`/`Deserialize` and stops there — but `SpriteCanvas::add_instance` takes
    `&InstanceInfo` (unchanged from today's signature, deliberately), and 2.2's dev proxy has to put that
    value into its own `Vec<InstanceInfo>` to drain later, which needs `Clone`/`Copy` and gets nothing of
    the kind from a serde derive. `cargo check -p surge-sdk` fails at 2.2. This is Q56's shape (a derive
    that isn't there) aimed at the standard library's traits rather than a Cargo feature table. Fix: 1.2
    adds `Clone, Copy` to both, verified by actually compiling `#[derive(Debug, Clone, Copy)]` on both
    structs against this workspace's pinned `nalgebra` (every field — `Vector2<f32>`, `Vector4<f32>`,
    `Vector2<u32>`, `f32`, `usize`, `Option<SubTextureInfo>` — is `Copy`, so the stricter derive is
    available and there is no reason to settle for `Clone` alone). The other four moved types already
    derive both today.
69. **`SpriteCanvas`'s method list was never written down** — resolved. Eleven passes wrote it as
    "`add_instance`, `upload_texture`, `get_create_info`, ...", ellipsis included, and step 1.4 says
    "Define `SpriteCanvas` and `CanvasError`" with no signatures — leaving the one trait every module
    programs against, both backends implement, and the whole dev/release symmetry rests on, to be
    reconstructed at implementation time. Exactly the gap Q35 closed for `KeyEvent`, still open for the
    more central type. Fix: the trait is now written out in full under "`surge-sdk`" above, transcribed
    from `SpriteDatabase`'s actual signatures — three methods, `get_create_info` returning `CreateInfo` by
    value (it is `Copy`, and that is what `SpriteDatabase::get_create_info` already does), `upload_texture`
    returning `Result<usize, CanvasError>` per Q17/Q28. The ellipsis resolves to *nothing*: those three are
    the complete set of methods either real module calls on its canvas parameter. `draw()` is deliberately
    excluded — `SurgeContext` owns the concrete database and calls it directly, no module ever has.
70. **3.4 deletes the file two crates import `SurgeApp` through** — resolved, compile-blocking. 1.6 leaves
    `surge-core/src/app/mod.rs` as a `pub use surge_sdk::SurgeApp;` re-export; 3.1 (checked against
    `surge-player/src/main.rs`, which today never names the trait at all, reaching it only through
    `LoadedApp`'s `Deref`) makes `surge-player` name it for the first time, necessarily through
    `sc::app::SurgeApp` since `surge-player` has no `surge-sdk` dependency until 3.2. 3.4 then says
    "delete `surge-core/src/app/mod.rs`" without repointing that import — and without dropping
    `pub mod app;` from `surge-core/src/lib.rs` (checked: it is line 1), which alone is a hard error, not a
    dead-code warning. Fix: both clauses fold into 3.4, which by then has the `surge-sdk` dependency 3.2
    added to reach `surge_sdk::SurgeApp` directly.
71. **Release binaries link the crate that hosts the IPC machinery — Q20's own objection, one crate over**
    — resolved. Q20 refused to put `RemoteApp`/the socket code in `surge-core` with an explicit argument:
    "every release binary — which always links `surge-core` as a plain `rlib` — would statically compile in
    `postcard` and the socket/spawn machinery it never uses, contradicting this doc's own 'no IPC ...
    compiled in' release goal." It then placed that code in `surge-player` — and Phase 4 makes every
    release binary depend on `surge-player` for `run()`. The argument transfers verbatim and no pass
    noticed, because Q20 was reasoning about `surge-core`'s dependency edge at a point (pass 4) when
    `surge-player` had no `[lib]` target for a module to depend on at all. Fix, needing neither a crate
    split nor a feature flag: `RemoteApp`, `proc.rs`, and the wire helpers are modules of `surge-player`'s
    **`[[bin]]`** target (declared from `main.rs`) and never move into `src/lib.rs` at 4.1 — a `mod`
    declared in `main.rs` compiles only into the binary crate, so `surge_player::run` carries none of it.
    Stated honestly rather than overclaimed: `libc` and `postcard` remain in `surge-player`'s
    `[dependencies]` and are therefore still *compiled* for a release build (Cargo has no per-target
    dependency scoping — the same limitation Q37 hit), so the goal holds for the linked binary's reachable
    code, not for the build graph. 4.1 states the rule; 4.2 checks it with `nm`.
72. **Nothing owns module-binary path resolution, and no step says what the dev `config.toml` must contain**
    — resolved. The design says `app_folder`/`app_name` resolve to `{app_folder}/{app_name}` with no
    `lib`/`.so` decoration, and Q59 made sure a file by that name exists — but the code that *performs*
    that join was `load_from_dylib`, which 3.4 deletes, and no step assigns the replacement to anything.
    2.5 says `RemoteApp` "spawns the module binary" without saying how it knows which one, and 3.1's
    `spawn_and_connect(&engine_config.startup_app)` is the only hint. Second half, checked against the
    real files: both `surge-modules/*/config.toml` say `app_folder = "."`, so a dev run following 3.2's
    test instructions verbatim would look for `./surge_mod_2048` and fail — the root working copy needs
    `app_folder = "target/debug"`, which is already true today for the `.so` and stays true for the
    binary, but is nowhere written down in a plan whose smoke tests depend on it. Fix: resolution is
    `RemoteApp`'s constructor (canonicalizing, failing loudly with the resolved path), stated in "Module
    discovery / config schema" and in 2.5; 3.2's test line now says exactly which `config.toml` to use and
    what to set `app_folder` to.
73. **Five smaller precision gaps** — resolved. (a) **The `nalgebra` re-export switch misses two
    fully-qualified call sites.** Q52/Q61 have both modules drop their own `nalgebra`/`winit` lines at 1.6
    and rewrite their `use` statements, but `surge-mod-default/src/lib.rs:94-95` writes
    `nalgebra::Vector2::new(...)` inline, bypassing its own `use` line — the module doesn't compile after
    the dependency comes out unless those two are rewritten too (the sweep-every-qualified-path shape Q51
    established for `surge_core::`, arriving a phase earlier). Folded into 1.6, along with both modules'
    `use surge_core::app::SurgeApp` → `use surge_sdk::SurgeApp`. (b) **A doc comment outlives the code it
    documents.** `surge-mod-2048/src/lib.rs:26-28` documents the decode cache but sits above
    `const MODULE_NAME`, not above `decode_image_assets`, so Q36/Q58/Q64b's deletion list leaves it in
    place describing behavior the module no longer has. Added to 1.6's list. (c) **0.2 changes a behavior,
    not just a type.** Both modules `.unwrap()` the upload result, so "a real `Err` once `max_textures` is
    hit" turns a `log::warn!` into a module panic between 0.2 and the Phase 3 cutover (after which the
    same failure lands player-side and routes through Q66). Intended — the old behavior silently drew the
    wrong texture — but worth naming in the step rather than discovering. (d) **The justification for
    naming a `[[bin]]` after the `[lib]` was wrong even though the conclusion was right.** Q59 argued it
    "is exactly what `src/main.rs` + `src/lib.rs` produces by default"; for a hyphenated package it is not
    (the default bin takes the *package* name, `surge-mod-2048`, the lib its underscored form). Checked by
    building it: a package with `crate-type = ["rlib"]` and an explicit `[[bin]] name = "surge_mod_2048"`
    compiles clean and emits both `target/debug/surge_mod_2048` and `libsurge_mod_2048.rlib`, and
    `required-features` gates the release bin exactly as Phase 4 assumes. Conclusion kept, reasoning
    replaced. (e) **The handshake constant had no home.** `Response::Handshake { wire_version }` is
    compared by the player against a value the module sends, and no step declares either — inviting two
    literals in two crates. 2.1 now declares `pub const WIRE_VERSION: u16` in `surge-sdk`, which both
    sides use.

### Added in pass 14

74. **`PR_SET_PDEATHSIG` is verified by nothing in the plan** — resolved, and the clearest instance of the
    shape this pass looked for: a step that names a test, passes it, and still proves nothing about its own
    deliverable. 2.4 builds `set_pdeathsig` and defers "functional verification ... to 2.6/3.x"; 2.6 wires
    it in with a compile gate; 3.2's and 3.5's smoke tests check `ps` for orphans — but only after a *clean*
    exit, which reaps the child through Q65's shutdown sequence and looks byte-identical whether
    `set_pdeathsig` works or is a no-op. Grep-checked across the whole plan: the only `kill` in any test
    targets the *child* (3.6 case 1), never the player. So a wrong `prctl` argument, a `pre_exec` closure
    that silently fails, or the call being dropped in a refactor would sail through every step and surface
    the first time a developer's player panicked — the exact scenario Q18 added it for. Fix: 3.6 case 2,
    `kill -9` the player and assert no surviving `surge_mod_*` in `ps`. One line, and it is the only test of
    that guarantee anywhere.
75. **Only the happy branch of the kill sequence is ever exercised** — resolved. F5 (3.5) does send
    `SIGTERM` to a live module, which is real coverage, so this is narrower than Q74 but the same species:
    both the `SIGKILL`-after-timeout escalation (Q30) and the `ShutdownAck`-timeout fallback (Q65) only run
    against a module that *doesn't* exit promptly, and every module in every test exits promptly. An
    implementation that sends `SIGTERM` and then simply waits forever — or one that never starts the timer —
    passes 3.5, 3.3, and every smoke test in the plan, and hangs the player the first time a module
    deadlocks, which is precisely when a developer most needs the player to exit. Fix: 3.6 case 3, a
    temporary `loop {}` in `drive()`'s `Shutdown` arm, module rebuilt alone. Cheap because the module is a
    separate binary: no player rebuild, no scaffolding, revert when done.
76. **Q60's `pending_on_load` fix is unobservable, and 3.5 says so without fixing it** — resolved. 3.5's own
    text concedes "both real modules do the same work in either hook, so the smoke test below cannot
    distinguish the two" — an honest statement of a test that cannot fail, left standing as if nothing could
    be done. Something can: the module process has its own stdout, and `drive()` is the one place every
    `Request` arrives. Fix: `drive()` logs each received variant at debug level (2.3), so a reload prints
    `OnLoad` from the fresh child if Q60's flag works and `OnSwapchainRecreate` if it doesn't, and 3.5's
    test reads that line. Worth generalizing beyond this instance: in a two-process design, "the other
    process's log" is the cheapest assertion surface available, and this plan had not used it once. It is
    also the reason case 4 in 3.6 can check a handshake rejection at all.
77. **Two more never-executed paths — handshake mismatch and the stall/fault distinction** — resolved.
    Neither has a test in any step: a `wire_version` mismatch (the entire justification for having a
    handshake) and Q67's rule that `WouldBlock`/`TimedOut` means "stall, keep waiting" while EOF means
    "fault." The second is the likelier bug of the two — one `if err.is_err()` in 2.5's read helper collapses
    them, and the symptom is a module that crashes the session whenever it is slow or paused in a debugger,
    which is a dev-mode-only failure that no rendering test would attribute to the socket layer. Both turn
    out to be cheap, and the first is not even injection: rebuilding *only* `surge-player` after bumping
    `WIRE_VERSION` leaves a genuinely stale module binary on disk — the actual production failure this
    handshake was designed to catch, reproduced by doing less work rather than more. The second is
    `kill -STOP` / `kill -CONT`. Fix: 3.6 cases 4 and 5.
78. **The compile gate never runs the tests the plan writes** — resolved. "Tested" was defined as
    `cargo check --workspace` + `cargo build --workspace`, with `cargo test` named only inside the four steps
    that author suites (1.2, 2.1, 2.2, 2.3). Every commit from 2.4 onward could therefore break framing,
    proxy-counter, or dispatch-loop tests and still pass its own stated gate — not hypothetical here, since
    3.2, 3.5, and 4.1 all edit `surge-sdk` again after its suites exist. Fix: the compile gate becomes
    `check` + `build` + `cargo test --workspace`, checked to be cheap and dependency-free (the three
    pre-existing `surge-core` tests are pure math/layout — no device, no window). Also worth stating once,
    which the plan never did: what the gate structurally *cannot* cover is anything needing a live
    `VkInstance` or a real child process, which is why the smoke tests and 3.6's cases exist and why five
    steps legitimately cannot do better than a compile gate (Q80).
79. **Four steps state no test at all** — resolved. 0.1, 0.2, 1.1, and 5.2 have no `Test:` clause anywhere
    in their text (checked mechanically, not by eye). Three of the four are not merely unstated but
    *unfalsifiable by the compile gate they implicitly rely on*: deleting an unused dependency (0.1)
    compiles identically whether or not it happened, a crate created but never added to `[workspace] members`
    (1.1) is skipped rather than failed by `--workspace`, and a documentation sweep (5.2) has no compile
    consequence whatsoever. Each gets an assertion that can actually go red — `cargo tree | grep` for 0.1,
    an explicit `cargo check -p surge-sdk` for 1.1, a must-be-empty `grep -rn` for 5.2. 0.2 is the different
    case: its success path is covered by the standard smoke test, but its new failure branch needs a live
    device and a full database, so it is genuinely unverifiable until 3.6 case 6 — five phases later. That
    is now stated in the step rather than left to chance.
80. **"Compile gate only" never said where the behavior is first proven** — resolved. Five steps (1.4, 1.5,
    2.4, 2.5, 2.6) build things no test at that commit can exercise, and only 2.5 acknowledged it. Silence
    made the other four read as though a compile gate were sufficient, when for 1.5 in particular it is
    conspicuously not: three forwarding methods where a swapped argument or a dropped index compiles
    perfectly and is caught only by 1.6's real-GPU smoke test one commit later. Fix: each of the five now
    names the step and the observable that first proves it (1.4 → 1.6; 1.5 → 1.6; 2.4 → 3.6 cases 2–3;
    2.5 → 3.2/3.3/3.6; 2.6 → 3.5/3.6). Deferral is fine and often unavoidable in a plan that builds
    machinery additively; unlabelled deferral is what lets a step ship unverified permanently, which is how
    Q74 happened.
81. **Three test-precision gaps** — resolved. (a) **2.2's proxy test covers half of Q63's rule.** It asserts
    the counter *continues* across drains but never that it *resets* when a new `CreateInfo` is cached — and
    a proxy that never resets passes the stated case while diverging from the real backend on every reload,
    which is the bug Q63 exists to prevent. A third case (`get_create_info` answered from cache, no I/O) is
    likewise unasserted anywhere despite being Q27's whole point. Both added. (b) **2.3's test doesn't
    assert dispatch order.** "Replay `input_events` in wire order, then call `update`" is a protocol
    property; a `drive()` that replays after `update` satisfies every stated assertion and silently delays
    every keypress by a frame. Assertion added. (c) **3.4's `ldd`/`nm` check for `libloading` is the wrong
    instrument.** `ldd` cannot show it (it is a statically linked rlib, so that half of the check has always
    passed vacuously and would keep passing with the dependency intact), and `nm` depends on linker section
    garbage collection. `cargo tree -p surge-core | grep libloading` tests the actual claim — the edge is
    gone — and fails deterministically when it isn't.

## Implementation plan (pass 7, sequencing corrected pass 8, dependency/visibility gaps closed pass 9, config-plumbing gap closed pass 10, build-output/lifecycle gaps closed pass 12, unimplemented-requirement gaps closed pass 13)

The workspace has no CI gate beyond the compiler and, `CLAUDE.md` notwithstanding, only a handful of tests
today (`surge-core/src/vulkan/sprite_database/mod.rs`'s three — see Q55), none of which any step below
depends on or perturbs, so "tested" below means one of two things, named once here and referenced by name
after: **the compile gate**
(`cargo check --workspace`, then `cargo build --workspace`, **then `cargo test --workspace`** — every
commit must pass all three; a commit that
doesn't is not a valid stopping point, full stop) and the **standard smoke test** (`cargo run -p
surge-player` from the repo root; confirm the window opens, the active module's sprites draw as expected,
a keypress/mouse click still logs or acts as before, a resize triggers `on_swapchain_recreate` without a
visible glitch, and closing the window exits cleanly with no orphaned process left in `ps`). Individual
steps note only how they *differ* from these two defaults. From step 3.3 on, the standard smoke test gains
one more line to watch for, since it is the cheapest observable proof that the shutdown round trip (Q65)
survived the cutover: `surge-mod-default`'s `on_unload` logs `Default app shutdown` on window close, and
its absence means `Request::Shutdown` never reached the module.

**Progress convention:** a step that has been implemented and passed its stated test is marked
**`[DONE]`** immediately after its number. An unmarked step is still open.

**Why `cargo test --workspace` is part of the gate and not just of the four steps that write tests**
(Q78, added pass 14): as pass 7–13 wrote it, the gate was `check` + `build`, and `cargo test` appeared only
in the four steps that author a suite (1.2, 2.1, 2.2, 2.3). Every commit after 2.3 could therefore break
one of those suites and still pass its own stated test — which is not hypothetical for this plan
specifically: 2.2's proxy and 2.1's framing helpers live in the same crate and get edited again at 3.2
(module port), 3.5 (`pending_on_load`), and 4.1 (config move). A suite nobody re-runs is documentation, not
a gate. Running the whole workspace's tests is cheap here (three `surge-core` unit tests, no GPU or window
needed — checked: they exercise matrix math and `#[repr]` sizes — plus whatever `surge-sdk` has accumulated),
so there is no reason to scope it per-crate. **What the gate still cannot cover, stated once:** anything
needing a live `VkInstance` (every `SpriteDatabase` method with a real device behind it) and anything needing
a real child process. Those are what the smoke tests and the fault-injection cases below are for, and every
step whose deliverable falls into that category now names the later step that first proves it, rather than
leaving "compile gate only" to imply nothing more was possible (Q80).

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

- **0.1.** **[DONE]** Delete the vestigial `surge-player` → `surge-mod-default` path dependency (Q25).
  `surge-player/Cargo.toml` only; `main.rs` never references the crate, so this is a one-line removal.
  Test: compile gate + `cargo tree -p surge-player | grep surge-mod-default` returns nothing (Q79 — the
  compile gate alone cannot fail here, since deleting an unused dependency compiles either way; the
  `cargo tree` assertion is the only thing that actually distinguishes "removed" from "still there but
  unused," which is precisely this step's deliverable).
- **0.2.** **[DONE]** Fix `SpriteDatabase::upload_texture`/`upload_decoded_texture`'s return type: `Result<(),
  VulkanError>` → `Result<usize, VulkanError>`, a real `Err` once `max_textures` is hit instead of a
  silently-successful `Ok(())` (Q17's source-level fix). The new failure needs a `VulkanError` variant of
  its own (Q48) — `TextureDatabaseFull` (no payload; `self.ci.max_textures` is already known at the call
  site if a message wants it). `surge-core` only — no module call-site changes needed, since both modules
  currently `.unwrap()` the result as a bare statement and discarding a `usize` compiles identically to
  discarding a `()`. This is explicitly flagged in "Self-critique / risks" above as worth landing as its
  own fix regardless of the rest of this doc; Phase 0 is where that happens. **One behavior change worth
  naming rather than discovering** (Q73c): because both modules `.unwrap()` that result, a full database
  stops being a `log::warn!` and becomes a module-side panic from this commit until the Phase 3 cutover,
  after which the same failure arrives player-side and routes through the crash path instead (Q17's part
  2, Q66). That is the intent — the old behavior was a silent wrong-texture bug — but it is a real change
  in what "32 textures uploaded" does, not a pure refactor. Neither module comes close to the cap today
  (12 and 2), so nothing in the plan's own smoke tests can hit it. Test: compile gate + standard smoke test
  on both modules (proves the *success* path still returns and that the new `usize` didn't break either
  module's `.unwrap()` site). **The new failure branch is not verifiable here and this step should not
  pretend otherwise** (Q79): reaching it needs a full `SpriteDatabase`, which needs a live device, which
  rules out a unit test — and both modules stay far under the cap. It is first exercised at **3.6**, whose
  lowered-`max_textures` case is the only place in this plan where an upload actually fails. That is a real
  gap of five phases between writing a branch and running it; naming the step that closes it is the honest
  alternative to leaving it unverified forever.

*(Milestone: workspace still behaves exactly as today; both changes are invisible to a user running the
player.)*

### Phase 1 — `surge-sdk` exists; `SpriteCanvas`/`KeyEvent` land; still single-process, still `dlopen`

- **1.1.** **[DONE]** Create the `surge-sdk` crate: `Cargo.toml`, an empty `lib.rs`. Add `"surge-sdk"` to
  `[workspace] members` explicitly — it is a root-level crate like `surge-core`/`surge-player`, not under
  `surge-modules/*`, so the existing glob does not pick it up. Test: compile gate, plus confirm
  `cargo check -p surge-sdk` resolves the package at all (Q79) — that is what catches the one mistake this
  step can actually make, a crate created on disk but never added to `[workspace] members`, which
  `cargo check --workspace` would silently skip rather than fail on.
- **1.2.** **[DONE]** Move the six plain-data types (`BlendingMode`, `TextureFilteringMode`, `TextureFilteringLevel`,
  `CreateInfo`, `SubTextureInfo`, `InstanceInfo`) from `surge-core::vulkan::sprite_database` into
  `surge-sdk`, adding `#[derive(Serialize, Deserialize)]` — **and `Clone, Copy` to `InstanceInfo` and
  `SubTextureInfo`, which have neither today** (Q68; the other four already derive both). This is not
  cosmetic and not deferrable to the step that needs it: 2.2's dev proxy buffers instances handed to it as
  `&InstanceInfo` into a `Vec<InstanceInfo>`, which no amount of `Serialize` makes possible, and 1.2 is the
  one commit that writes these `#[derive]` lines at all. Verified by compiling both structs with
  `#[derive(Debug, Clone, Copy)]` against this workspace's pinned nalgebra — every field is `Copy`, so the
  derive is accepted. Add `serde`, `nalgebra` (with `serde-serialize`),
  and `postcard` to `surge-sdk/Cargo.toml` (Q48 — `postcard` is used by this step's own test below, so it
  belongs here, not re-introduced at 2.1 as pass 7 had it). **Both new dependencies need explicit feature
  flags, not just their bare crate names** (Q56, checked against postcard 1.1.3's and serde 1.0.228's actual
  `[features]` tables): `serde = { version = "1.0.228", features = ["derive"] }` — `derive` is off by
  `serde`'s own default (`default = ["std"]`), and every one of this step's six types needs it for
  `#[derive(Serialize, Deserialize)]` to exist at all — and `postcard = { version = "1.1.3", features =
  ["use-std"] }` — `postcard::to_stdvec`, used by this step's own test below, is behind `#[cfg(feature =
  "use-std")]`, off by postcard's own default (`default = ["heapless-cas"]`). Without both, `cargo check -p
  surge-sdk` fails on this commit itself. (`postcard::from_bytes`, also used below, is not feature-gated and
  needs no fix.) Add one `#[test]` round-tripping an `InstanceInfo` (with a
  `Some(SubTextureInfo)`) through `postcard::to_stdvec`/`from_bytes` — the first test in `surge-sdk`, but
  **not**, contrary to earlier drafts of this step, the first test in the workspace (Q55):
  `surge-core/src/vulkan/sprite_database/mod.rs` already has three
  (`full_z_range_lands_inside_vulkan_depth_bounds`, `smaller_world_z_draws_in_front`,
  `gpu_struct_layout_matches_glsl`), predating this doc entirely.
  `CLAUDE.md`'s "There are currently no tests anywhere in the workspace" line is therefore already stale
  today, not about to become stale here — this commit is simply a convenient, already-nearby place to fix
  it (1.2 is already touching `surge-sdk`'s own first test), not the cause of the inaccuracy. Test: `cargo
  test -p surge-sdk`.
- **1.3.** **[DONE]** `surge-core` takes a `path` dependency on `surge-sdk`; the six local definitions in
  `sprite_database/mod.rs` become `pub use surge_sdk::{BlendingMode, TextureFilteringMode,
  TextureFilteringLevel, CreateInfo, SubTextureInfo, InstanceInfo};`. This is a pure compatibility-shim
  commit: `surge_core::vulkan::sprite_database::InstanceInfo` still resolves to the same path for existing
  module code, so **neither module's source changes in this step**. (nalgebra's `serde-serialize` feature
  needs no explicit enabling in `surge-core/Cargo.toml` — Cargo's feature unification turns it on
  workspace-wide once anything enables it, which `surge-sdk` now does.) Test: compile gate + smoke test on
  both modules, unchanged.
- **1.4.** **[DONE]** Define `SpriteCanvas` and `CanvasError` in `surge-sdk` (additive; nothing implements or calls
  either yet) — the trait verbatim as written under "`surge-sdk`" above, three methods, no more (Q69: that
  definition is now pinned in the design section rather than left as "`add_instance`, `upload_texture`,
  `get_create_info`, ...", so this step is transcription, not design). Include the doc-comment on
  `upload_texture` about the dev/release fallibility asymmetry (Q28) now, while the reasoning is fresh — it
  costs nothing to write before the dev backend exists. Test: compile gate — genuinely all there is for a
  commit that adds a trait nothing implements or calls, and the compile gate does check the one property
  that matters here (object safety: `&mut dyn SpriteCanvas` must be nameable, which 1.6 depends on and
  Q33's spike already proved). First behavioral proof: 1.6's smoke test (Q80).
- **1.5.** **[DONE]** `impl SpriteCanvas for SpriteDatabase` in `surge-core`, plus `impl From<VulkanError> for
  CanvasError` (Q34's resolved simple form — plain `From`, `?` at the call site, no hand-rolled
  `.map_err`). Additive; nothing calls through the trait yet. Test: compile gate — and this is the step in
  Phase 1 where that is weakest, worth flagging rather than glossing (Q80): the impl is three forwarding
  methods, and a forwarding mistake (wrong argument, index dropped, `get_create_info` returning a stale
  value) compiles perfectly. No unit test can cover it either — every method needs a live
  `SpriteDatabase`, which needs a device. First behavioral proof is **1.6**'s smoke test, one commit later,
  where both modules start calling every one of these three methods through the trait on a real GPU; if
  1.6's sprites draw and its textures survive a resize, this impl is correct. Keeping 1.5 separate is still
  right (it isolates the orphan-rule/`From` question), but it is a commit whose own gate cannot fail for
  the reason it exists.
- **1.6.** **[DONE]** Move the `SurgeApp` trait definition to `surge-sdk`, **and** retype `on_load`/
  `on_swapchain_recreate`/`update` to take `&mut dyn SpriteCanvas` instead of `&mut SpriteDatabase`, in the
  same commit — pass 7 originally split these into 1.6 (relocation, "signatures unchanged") and 1.7
  (retyping), but that split doesn't compile: `surge-core` already depends on `surge-sdk` as of 1.3, so a
  `surge-sdk`-hosted trait can never name `SpriteDatabase` (a `surge-core` type) without creating a
  dependency cycle, not even for one intermediate commit (see resolved open question 41). `winit::event::
  KeyEvent`/`DeviceId`/`ElementState`/`MouseButton`/`MouseScrollDelta`/`TouchPhase` **do** stay unchanged in
  this commit, unaffected by Q41 — `winit` is an ordinary external dependency, not a workspace member, so
  naming those types in `surge-sdk` carries no cycle risk; only the `SpriteDatabase`-shaped parameters were
  ever actually forced to move together. This step is also the first thing in `surge-sdk` to *name* a `winit`
  type, so it adds `winit = "0.30.12"` to `surge-sdk/Cargo.toml` in the same commit (Q49a) — pass 7/8 never
  assigned that dependency to a step. `surge-core::app::mod.rs` does `pub use surge_sdk::SurgeApp;`;
  `LoadedApp`/`load_from_dylib` are otherwise untouched (their `Box<dyn SurgeApp>` field doesn't care what
  the trait's methods take). Both modules' `Cargo.toml`s gain a `surge-sdk` path dependency (alongside
  their existing `surge-core` one — not yet removed); their `impl SurgeApp for ...` blocks change the `use`
  line and the three method signatures. **The three method signatures are not the whole story** (Q50): both
  modules route each method's `&mut SpriteDatabase` argument straight through to a free helper function
  before touching it — `surge-mod-2048/src/lib.rs`'s `upload_image_assets(spdb: &mut SpriteDatabase, ...)`
  and `add_board_sprite(spdb: &mut SpriteDatabase, ...)`, `surge-mod-default/src/lib.rs`'s
  `upload_assets(spd: &mut SpriteDatabase)` — and `add_board_sprite`/`upload_assets` have to retype their
  parameter to `&mut dyn SpriteCanvas` in this same commit, or the (now `&mut dyn SpriteCanvas`-typed) call
  sites inside `on_load`/`on_swapchain_recreate`/`update` stop type-checking against them.
  `surge-player/main.rs`'s call sites need no cast — `&mut SpriteDatabase` coerces to `&mut dyn SpriteCanvas`
  at the call site (an ordinary unsized coercion), but that coercion happens once, at the outermost call from
  `surge-player` into `on_load`/etc.; it does not propagate through a module's own further function calls,
  which is exactly why those need retyping by hand instead. **`upload_image_assets` is not in that "just
  retype" set** (Q58 — Q50's original wording put it there, contradicting the paragraph below): it doesn't
  survive this commit at all, retyped or otherwise; see the `DecodedTexture` paragraph below, where it's
  replaced outright, not retyped. **Also bundled into this same commit** (Q52): both modules drop their
  own direct `nalgebra = "0.34.1"` dependency and switch `use nalgebra::...` to `use surge_sdk::nalgebra::...`
  — `surge-sdk` re-exports the crate wholesale (`pub use nalgebra;`, added alongside the `winit` dependency
  above) — since this commit is already touching every `InstanceInfo`/`SubTextureInfo`-constructing call site
  for the `SpriteCanvas` retyping regardless, and leaving each module on its own separate `nalgebra` line is
  the version-skew risk the `surge-sdk` section already flags. **`winit` gets the identical treatment in this
  same commit** (Q61): `surge-sdk` also does `pub use winit;`, both modules drop their own `winit = "0.30.12"`
  line, and their `use winit::event::{...}`/`use winit::keyboard::{...}` become `use surge_sdk::winit::...` —
  the trait they implement now names winit types from `surge-sdk`'s copy, so a module matching them against
  its own separately-declared copy type-checks only as long as the two version strings keep agreeing. This
  matters more after 1.7, not less: `surge_sdk::KeyEvent`'s own fields are winit types. **Both re-export
  switches have to catch fully-qualified paths, not just `use` lines** (Q73a): `surge-mod-default`'s
  `update` body writes `nalgebra::Vector2::new(...)` inline twice (`lib.rs:94-95`) rather than going
  through its `use` line, so dropping the crate's own `nalgebra` dependency without rewriting those two
  call sites to `surge_sdk::nalgebra::Vector2` leaves the module not compiling — the same
  sweep-every-qualified-path shape Q51 fixed for `surge_core::` at 3.2, arriving one step and one crate
  earlier. Both modules' `use surge_core::app::SurgeApp` also becomes `use surge_sdk::SurgeApp` here (the
  `surge-core` re-export would keep working, but the modules already gain their `surge-sdk` dependency in
  this commit and 3.2 would otherwise have to touch the line again). **Bundled into this same commit** (see resolved
  open question 36): `surge-mod-2048` drops its `cached_textures` field, `decode_image_assets`, and the
  `DecodedTexture` import, and calls `SpriteCanvas::upload_texture(path)` directly per asset name from both
  `on_load` and `on_swapchain_recreate` — this is a forced consequence of the signature change (there is no
  `SpriteCanvas` method that does what `upload_decoded_texture` did), not optional cleanup, so splitting it
  into its own commit would leave this one not actually compiling in between. **Concretely** (Q58, since Q36
  named the reasoning but not the replacement's exact shape): `upload_image_assets` is deleted, not retyped —
  its own second parameter, `textures: &[DecodedTexture]`, has nothing left to produce it once
  `decode_image_assets`/`cached_textures` are gone in this same commit, and no `SpriteCanvas` method takes a
  `DecodedTexture` anyway. In its place, a new `fn upload_board_assets(canvas: &mut dyn SpriteCanvas)` loops
  over the same 12 filename literals `decode_image_assets` used to hardcode (`"board.png"` through
  `"pieces_2048.png"`), calling `surge_core::assets::resolve_asset_path(MODULE_NAME, name)` (not
  `surge_sdk::` — that function doesn't move until 3.2, per Q32) then
  `canvas.upload_texture(path.to_str().unwrap())` per name; `on_load`/`on_swapchain_recreate` call
  `upload_board_assets(spdb)` where they used to call `upload_image_assets(spdb, self.get_cached_textures())`.
  The `DecodedTexture` cleanup spans two files, not one (Q64b): `lib.rs`'s `use surge_core::{...}` block and
  `app_2048.rs:4`'s separate `use surge_core::vulkan::sprite_database::DecodedTexture;` both go, along with
  `app_2048.rs`'s `use crate::{..., decode_image_assets, ...}` import, the `cached_textures:
  decode_image_assets()` initializer inside `App2048::new()`, and the now-callerless `get_cached_textures()`
  accessor. The doc comment at `lib.rs:26-28` ("Decodes every board/piece PNG from disk once. The result is
  cached by `App2048` ...") describes the retired cache and is orphaned by this same deletion — it sits
  above `const MODULE_NAME`, not above the function it documents, so it survives the deletion silently and
  then describes behavior the module no longer has (Q73b). It goes too.
  `surge-mod-default`'s body needs no equivalent change (it already calls `upload_texture(path)` directly, and
  `upload_assets` is genuinely just a retype — it never touched `DecodedTexture`). Test: compile gate + smoke
  test, **both modules explicitly** — this is the first commit that changes what either module actually does
  at runtime; watch 2048's textures reload correctly on a resize.
- **1.7.** **[DONE]** Add the `surge_sdk::KeyEvent` mirror struct (Q35's field list, plus `Debug` — both modules'
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

- **2.1.** **[DONE]** Define `Request`/`Response`/`CanvasOps`/`InputEvent` in `surge-sdk` per "Message shape" above
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
  holds a stream boundary. This step also declares the one constant the handshake compares against —
  `pub const WIRE_VERSION: u16` in `surge-sdk`, sent by `run_dev_module` and checked by `RemoteApp`, so
  neither side hardcodes a literal and a mismatch is impossible to introduce by editing only one crate
  (Q73e). Test: `cargo test -p surge-sdk`.
- **2.2.** **[DONE]** Add the dev-mode `SpriteCanvas` implementor to `surge-sdk`: buffers `add_instance`/
  `upload_texture` calls, drains them into a `CanvasOps`, and answers `get_create_info()` from a cached
  `CreateInfo` (Q27's push-not-pull fix) rather than any round trip. `upload_texture` returns a
  **cumulative** upload index, reset only when a new `CreateInfo` is cached — not this turn's buffer
  position (Q63): the player rebuilds its `SpriteDatabase` exactly at the two points a `CreateInfo` arrives,
  so a counter reset there matches the real backend's index in every case, while a per-call index matches
  only when the module uploads nothing outside `on_load`/`on_swapchain_recreate`. Pure data structure — no
  socket I/O — so it's fully unit-testable without a process or even a `UnixStream`: push N calls, drain,
  assert the resulting `CanvasOps`; include one case that uploads, drains, then uploads again without an
  intervening `CreateInfo` and asserts the second call's returned index continues from the first, **and one
  that asserts the counter *does* reset when a new `CreateInfo` is cached** (Q81a — the stated case only
  covers half of Q63's rule, and a proxy that never resets passes it while diverging from the real backend
  on every reload, which is the exact bug Q63 was written about). Add a third asserting `get_create_info()`
  returns the cached value with no I/O of any kind — the whole point of Q27's push-not-pull fix, and
  currently asserted nowhere.
  Test: `cargo test -p surge-sdk`.
- **2.3.** **[DONE]** Implement `run_dev_module`'s dispatch loop as two layers, not one — a deliberate testability fix
  over what earlier passes implied. A private, non-diverging `fn drive(app: &mut dyn SurgeApp, socket: impl
  Read + Write) -> io::Result<()>` does the actual handshake/dispatch/reply loop and returns `Ok(())` after
  `Shutdown`/`ShutdownAck`; the public `pub fn run_dev_module(app: Box<dyn SurgeApp>) -> !` is a thin
  wrapper that resolves the socket path from `std::env::args().nth(1)` (Q39), connects, calls `drive`, then
  `std::process::exit(0)`. A `-> !` function that calls `process::exit` cannot be safely called from inside
  a normal `#[test]` (it would kill the test process), so without this split the whole dispatch loop would
  be untestable short of spawning a real subprocess per test. With the split, a `#[test]` uses
  `UnixStream::pair()` (an in-process, kernel-backed socket pair — no subprocess, no real spawn) to drive
  `drive()` against a scripted sequence of `Request`s from a second thread standing in for the player, and
  asserts the `Response`s it gets back. **What `drive()` does per variant, spelled out** — the loop is this
  step's entire deliverable and pass 7–12 only ever described three of its four arms: `OnLoad`/
  `OnSwapchainRecreate` cache the pushed `create_info` into the 2.2 proxy (resetting its upload counter,
  per Q63), call the matching `SurgeApp` method with the proxy as `&mut dyn SpriteCanvas`, and reply
  `CanvasOps`; `Frame` replays `input_events` in wire order into `keyboard_event`/`mouse_button_event`/
  `mouse_wheel_event` (reconstructing `DeviceId::dummy()`, per Q19) *before* calling `update(dt, canvas)`,
  then replies `CanvasOps`; **`Shutdown` calls `app.on_unload()`, replies `ShutdownAck`, and returns
  `Ok(())`** (Q65 — without this the trait's own shutdown hook is simply never invoked in dev mode again,
  a regression no compile error catches and no smoke test notices unless it is looking for
  `surge-mod-default`'s shutdown log line). `drive()` also emits one `log::debug!` naming each `Request`
  variant it receives, before dispatching it (Q76) — three words of code that turn the module process's
  own log into a transcript of the protocol, which is what makes 3.5's F5 test able to check that a
  respawned child receives `OnLoad` rather than `OnSwapchainRecreate`. Without it, Q60's entire fix is
  unobservable by anything in this plan, as 3.5 itself concedes: both real modules do identical work in
  both hooks. The scripted `#[test]` covers all four arms, ending with
  `Shutdown` and asserting both the `ShutdownAck` and that `drive` returned; it also asserts that a
  `Frame` carrying input events dispatches them into the app's handlers **in wire order and before**
  `update` is called (Q81b — the ordering is a stated protocol property, and a `drive()` that replays
  events after `update` passes every other assertion while making a keypress land one frame late). Test:
  `cargo test -p surge-sdk`.
- **2.4.** **[DONE]** Add `surge-player/src/proc.rs`: `set_pdeathsig()` (`libc::prctl(PR_SET_PDEATHSIG, ...)`, Q18) and
  `terminate(pid)` (`libc::kill(pid, libc::SIGTERM)`/`nix::sys::signal::kill`, Q30) as free functions, not
  yet called from anywhere. Add the `libc`/`nix` dependency to `surge-player/Cargo.toml`. Test: compile
  gate only — there is no meaningful unit test for `PR_SET_PDEATHSIG` short of a real process pair.
  **But "deferred to 2.6/3.x," as passes 7–13 left it, deferred it to nowhere** (Q74): the only situation
  `PR_SET_PDEATHSIG` exists for is the player dying *without* running `exiting()`, and no test anywhere in
  this plan ever kills the player — every smoke test closes the window, which is the clean path that
  reaps the child through Q65's shutdown sequence and would look identical if `set_pdeathsig` were a
  no-op. A wrong `prctl` argument, a `pre_exec` closure that never runs, an error swallowed at spawn: all
  invisible. Its verification is now a named case in **3.6** (`kill -9` the *player*, then confirm no
  `surge_mod_*` process survives in `ps`) — a one-line test for the one guarantee this function exists to
  provide. Same for `terminate`: SIGTERM itself is exercised by 3.5's F5 reload, but the
  `SIGKILL`-after-timeout escalation is not, and that too is named in 3.6 (Q75).
- **2.5.** **[DONE]** Build `RemoteApp` in `surge-player` — as a module of the **binary** target (`mod remote_app;`
  declared from `main.rs`), never of a library target, which is what keeps it out of every release binary
  once 4.1 creates one (Q71). It binds the abstract-namespace `UnixListener` (Q23) before
  spawning, resolves the module binary as `Path::new(app_folder).join(app_name)` from the
  `StartupAppConfig` it is handed — no `lib` prefix, no extension, canonicalized so a missing file fails
  with the resolved path in the message (Q72) — spawns it via `Command` (socket name as `argv[1]`,
  `pre_exec` calling `set_pdeathsig`), accepts the connection, validates the unprompted
  `Response::Handshake` against `surge_sdk::WIRE_VERSION`, and implements
  `SurgeApp` — `on_load`/`on_swapchain_recreate`/`update` round-trip a `Request` and apply the returned
  `CanvasOps` against the real `&mut dyn SpriteCanvas` it was given (texture uploads first, in order, then
  instances — a `texture_uploads` failure mid-sequence stops processing and routes to the Q11 crash path,
  per Q17's part 2); `keyboard_event`/`mouse_button_event`/`mouse_wheel_event` buffer into a local
  `Vec<InputEvent>`, no IPC, drained into the next `Request::Frame`. The outgoing `create_info` on
  `Request::OnLoad`/`Request::OnSwapchainRecreate` (Q44) comes from `canvas.get_create_info()` — called on
  the same real `&mut dyn SpriteCanvas` argument `on_load`/`on_swapchain_recreate` were just handed, before
  sending the `Request` — not a new value `RemoteApp` computes or caches itself; `SurgeContext` already
  constructed that canvas with the right `CreateInfo` before calling through. **Three more pieces belong
  in this commit, since all three are properties of the same round-trip helpers rather than separable
  features:** (a) `RemoteApp::on_unload` sends `Request::Shutdown`, waits for `ShutdownAck` under a bounded
  `set_read_timeout`, then escalates through 2.6's `terminate` and reaps with `Child::wait` (Q65 — this is
  the player half of the shutdown round trip; the module half landed at 2.3); (b) every round trip goes
  through one `write_request`/`read_response` pair that treats `WouldBlock`/`TimedOut` as a *stall* —
  `log::warn!("waiting on module response — {}ms and counting")`, then read again — and only EOF or a
  transport error as a *fault*, which is Q10's never-implemented logging half and the distinction Q67
  requires it to keep (conflating the two would turn a debugger-paused module into a phantom crash); (c)
  the `faulted: bool` those helpers set, which for now only makes subsequent methods no-op and logs loudly
  — surfacing it to `SurgeContext` needs a trait hook that doesn't exist until 3.6, and adding the flag
  earlier than its consumer keeps each commit compiling without a forward reference. Not yet constructed
  from `main()`. Test: compile gate only — there is deliberately no throwaway fixture binary built just to
  integration-test this in isolation; the first real end-to-end exercise of `RemoteApp` is 3.2, against a
  real module. This is an accepted, named risk, not an oversight: Phase 2 proves the protocol's *types* and
  the dispatch loop's *logic* (2.1–2.3) in isolation, but proves nothing about the actual spawn/socket/kill
  mechanics until a real process is on the other end. **Where each piece of this commit is first actually
  proven** (Q80), since "compile gate only" otherwise hides that this is the largest untested commit in the
  plan: the spawn/connect/handshake path at 3.2; the shutdown round trip at 3.3 (the `Default app shutdown`
  log line); the `create_info` push at 3.2 (2048 draws its board using `get_create_info`, so a wrong value
  is visible immediately as wrong geometry); the fault flag and the stall/fault distinction at 3.6, cases
  1 and 5. Nothing here stays unverified indefinitely — but nothing here is verified for four commits
  either, which is the price of building the machinery additively and is worth paying with eyes open.
- **2.6.** **[DONE]** Wire `set_pdeathsig`/`terminate` and the `reload_in_progress` flag (Q22) into `RemoteApp`'s
  spawn/kill/reload methods (the inherent `RemoteApp::reload()`, not yet called from anywhere). One
  `SIGTERM` → bounded wait → `SIGKILL` → `Child::wait` helper serves both callers — reload and 2.5's
  shutdown path — rather than two near-identical sequences (Q30's warning about reaching for
  `Child::kill()` applies to both equally). `reload()` also clears `faulted` on a successful re-handshake,
  which is what makes F5 the recovery path Q11 requires instead of an auto-respawn. Test:
  compile gate; first proven at 3.5 (the reload sequence end to end, via F5) and 3.6 cases 2 and 3
  (`set_pdeathsig`, and the `SIGKILL` escalation this step's helper implements) — Q80. (The `SurgeApp::reload() -> bool` trait method that lets generic code — `SurgeContext`,
  operating only on `Box<dyn SurgeApp>`/`StartupApp` — actually reach this is added at 3.5, per Q45, once
  the F5 keybind is what first needs it; adding it here would be premature; noted now so it isn't missed
  later.) *(As built: 2.5 already wired `set_pdeathsig` and the shared `stop_child` helper, so this step
added `reload_in_progress` — consulted by `fault()` — and `RemoteApp::reload() -> io::Result<()>`, which
leaves the app faulted on a failed respawn so F5 can retry. `stop_child` now returns early if the child
has already exited, so a crashed or already-reaped child is never signalled by a possibly reused pid.
`pending_on_load` stays in 3.5, per Q60.)*

*(**Milestone B**: the entire dev-mode machinery exists, is additive, and compiles — the full lifecycle
(`OnLoad` → `Frame` → `Shutdown`), the framing, the proxy's counters, the stall/fault distinction, and the
spawn/kill primitives — but it is unproven end-to-end. Phase 3 is where it either works against a real
module or doesn't.)*

### Phase 3 — Cutover: `dlopen` → spawn+IPC, module by module (Q38's scaffolding)

- **3.1.** **[DONE]** Add the temporary `SURGE_DEV_LOADER` branch to `surge-player::main()` (Q38): unset/anything but
  `process` keeps `load_from_dylib`; `process` selects a new `spawn_and_connect(&engine_config.startup_app)`
  path constructing a `RemoteApp`. Since `main()` must build `ctx.startup_app` from *either* branch and a
  struct field can't hold two different concrete types, this step also introduces the temporary
  `StartupApp` enum (Q43): `enum StartupApp { Dylib(sc::app::LoadedApp), Remote(Box<dyn SurgeApp>) }`, with
  hand-written `Deref`/`DerefMut` to `dyn SurgeApp` (a two-arm `match`). This is the first line of
  `surge-player` to *name* `SurgeApp` (today's `main.rs` only reaches the trait's methods through
  `LoadedApp`'s `Deref`, never naming it — checked), and `surge-player` has no `surge-sdk` dependency until
  3.2, so the import here is `sc::app::SurgeApp`, resolving through the re-export 1.6 left in place; 3.4
  repoints it when that re-export is deleted (Q70). `SurgeContext.startup_app`'s
  type changes from `sc::app::LoadedApp` (`surge-player/src/main.rs:21`) to `StartupApp` — every existing
  `self.startup_app.<method>(...)` call site is unaffected, since deref coercion covers the enum exactly as
  it already covered `LoadedApp`. Nothing can select `process` successfully yet. Test: compile gate + smoke
  test with the variable unset (unchanged behavior, still `dlopen`, now routed through
  `StartupApp::Dylib(...)`).
  *(As built: `spawn_and_connect` is a small `#[cfg(target_os = "linux")]` fn in `main.rs` wrapping
  `RemoteApp::new`; on other targets `SURGE_DEV_LOADER=process` panics with an explicit message, since
  `RemoteApp` is Linux-only. The `#[allow(dead_code)]` on `mod proc` is gone; the one on `mod remote_app`
  stays until 3.5 gives `reload`/`is_faulted` a caller. The window smoke test was not run in this
  change — only the compile gate.)*
- **3.2.** **[DONE]** Move `init_env_logger`/`resolve_asset_path` from `surge-core::cli`/`surge-core::assets` to
  `surge-sdk` (Q32) — this becomes load-bearing exactly here, not before: it's the first point a module
  needs them *without* a `surge-core` dependency. This is also the first thing `surge-sdk` needs `env_logger`/
  `log` for, so this commit adds `env_logger = "0.11.8"` to `surge-sdk/Cargo.toml` (Q49b —
  `init_env_logger`'s body calls both directly; neither was ever assigned to a step before now). `log =
  "0.4.28"` is *not* added here: it landed a step earlier, at 2.3, because `drive()`'s per-request
  `log::debug!` (Q76) is the first `surge-sdk` code to call it.
  `surge-core::cli::print_logo` stays in `surge-core` (only `surge-player::main` calls it; not a module
  concern). This move also breaks `surge-player`'s *own* logger init (Q42) — `surge-player/src/main.rs:332`
  calls `sc::cli::init_env_logger()` for the player itself, and that function no longer exists in `surge-core`
  once this step lands — so this same commit adds a direct `surge-sdk` path dependency to
  `surge-player/Cargo.toml` and changes that call site to `surge_sdk::init_env_logger()`. Port
  `surge-mod-2048`: add `src/bin/dev.rs` (`surge_sdk::init_env_logger();
  surge_sdk::run_dev_module(surge_mod_2048::surge_register_app());`) **with an explicit `[[bin]] name =
  "surge_mod_2048", path = "src/bin/dev.rs"` table** (Q59 — Cargo would otherwise name it after the file,
  producing `target/debug/dev`: a path no `app_name` resolves to, and one that collides with
  `surge-mod-default`'s identically-named binary at 3.3, silently leaving whichever linked last), drop the
  `dylib` crate-type (keep `rlib`, needed by the eventual release `[[bin]]` target), swap the `surge-core`
  dependency for `surge-sdk` only. **The dependency swap is not the whole story** (Q51): `App2048::on_load` already calls
  `surge_core::cli::init_env_logger()` directly (`lib.rs:82`, matching `CLAUDE.md`'s documented "modules call
  `init_env_logger()` themselves in `on_load`" behavior) — left in place (even renamed to
  `surge_sdk::init_env_logger()`), `dev.rs`'s new call at process startup and the old one inside `on_load`
  both fire in the same process, and `env_logger::Builder::init()`'s second call panics (`log`'s global logger
  can only be set once). This same commit deletes that call from `on_load` entirely — logging is now
  initialized exactly once, in `dev.rs`'s `main()`, before `run_dev_module` runs — and sweeps every other
  remaining `surge_core::`-qualified path in the module's source over to `surge_sdk::` (`resolve_asset_path`,
  called from `on_load`/`on_swapchain_recreate`'s post-Q36 bodies), since the `surge-core` dependency this
  commit removes was the only thing making any of them resolve. **The entry point doesn't name the concrete
  app type** (Q53): `App2048` is `pub(crate)` inside a private `mod app_2048;` (`lib.rs`), so `App2048::new()`
  is unreachable from a separate `[[bin]]` crate in the same package — `src/bin/dev.rs` instead calls the
  already-`pub`, already-`#[unsafe(no_mangle)]` `surge_mod_2048::surge_register_app() -> Box<dyn SurgeApp>`
  every module already exports, which needs no visibility changes and keeps both modules' entry points
  constructed the same way. Test: compile gate + `SURGE_DEV_LOADER=process cargo run -p surge-player`
  with the root `config.toml` copied from `surge-modules/surge-mod-2048/config.toml` and its `app_folder`
  set to `target/debug` (Q72 — the checked-in copies say `app_folder = "."`, and what changed in this
  commit is the *filename* the player looks for under that folder: `surge_mod_2048`, no `lib` prefix, no
  `.so`) — full smoke test, this is the first real end-to-end proof
  of the IPC path (this is also where 2.5's deferred integration risk actually gets resolved); confirm the
  log banner prints exactly once, not twice, and that `ps` shows exactly one `surge_mod_2048` child while
  the window is open and none after it closes.
  *(As built: `surge-player` already had its `surge-sdk` dependency from 2.x, so only the call site changed.
  `surge-core` also dropped its `env_logger` dependency, since `init_env_logger` was its only user.
  `surge-mod-default` is still a `dlopen`ed dylib until 3.3, so its two call sites were repointed to
  `surge_sdk::` here purely to keep it compiling; its `on_load` still calls `init_env_logger()` for now.
  The smoke test ran with the process loader and a SIGINT after ~8s rather than a window close: one
  `surge_mod_2048` child while open, none after, `2048 startup` and the `OnLoad` request logged once.)*
- **3.3.** **[DONE]** Port `surge-mod-default` the same way — `AppDefault {}` is already a `pub` unit struct directly in
  `lib.rs`, so `surge_mod_default::surge_register_app()` (Q53, for consistency with 2048's entry point) works
  the same way without ever having hit the visibility problem 3.2 traced through; `on_load`'s own
  `init_env_logger()` call (`lib.rs:31`) is deleted the same way, for the same reason (Q51), and its
  `[[bin]]` table names the binary `surge_mod_default` (Q59 — this is the commit where a shared `dev` name
  would start colliding with 3.2's, so the explicit name has to be in place on both). Test: compile gate +
  `SURGE_DEV_LOADER=process` smoke test with `config.toml` pointed at `surge_mod_default`, **then re-run
  3.2's 2048 smoke test unchanged** — a passing 2048 run before this commit proves nothing about a
  colliding binary name introduced by it. This is also the first step whose smoke test can observe the
  shutdown round trip (Q65): `surge-mod-default` is the module with an `on_unload` body, so closing the
  window must print `Default app shutdown` — from the *child* process — before the player exits. Its
  absence means `Request::Shutdown` isn't being sent or isn't being dispatched, which nothing else in the
  plan would catch.
  *(As built: `surge-mod-default` dropped its `surge-core` dependency and the `dylib` crate-type, and gained
  `src/bin/dev.rs` plus the explicit `[[bin]] name = "surge_mod_default"` table. Smoke tests ran with the
  process loader and a SIGINT after ~8s for both modules: exactly one child while open and none after,
  `Default app startup` / `2048 startup` logged once each. **Not yet observed:** `Default app shutdown` —
  SIGINT does not go through a window close, and no tool was available to close the window from the
  shell, so the Q65 shutdown round trip still needs one manual window-close run.)*
- **3.4.** **[DONE]** Cutover complete: delete the `SURGE_DEV_LOADER` branch and default `main()` to the `process` path
  unconditionally; delete `surge-core/src/app/mod.rs` (`LoadedApp`, `load_from_dylib`), `AppError` and
  `ModuleError` from `surge-core/src/errors/mod.rs` (Q29 — `ModuleError` was already fully dead before this
  doc), and the `libloading` dependency from `surge-core/Cargo.toml`. The `StartupApp` enum from 3.1 (Q43)
  has nothing left to hold in its `Dylib` arm, so it collapses too: `SurgeContext.startup_app` becomes plain
  `Box<dyn SurgeApp>`, `StartupApp` itself is deleted. **Deleting that file takes two `use` sites and a
  module declaration with it** (Q70): `surge-core/src/app/mod.rs` is not only `LoadedApp`/`load_from_dylib`
  — since 1.6 it is also the `pub use surge_sdk::SurgeApp;` re-export that both modules imported through
  until 1.6 repointed them, and that `surge-player` still imports through as of 3.1. So this commit also
  drops `pub mod app;` from `surge-core/src/lib.rs` (otherwise the crate names a module whose file no
  longer exists — a hard error, not a warning) and changes `surge-player`'s `sc::app::SurgeApp` import to
  `surge_sdk::SurgeApp`, using the direct dependency 3.2 added. This is where the "Release mode" section's "swapping
  the `startup_app` field's type... is a small, mechanical change" claim actually finishes landing — not at
  4.1, which is pure code motion by the time it runs. Small trace-through worth doing here rather than leaving
  implicit (Q53): `load_from_dylib` was the only caller that ever needed `surge_register_app` to be an
  `#[unsafe(no_mangle)]`, dynamically-resolved symbol; both modules keep calling it as an ordinary `pub fn`
  from their `dev.rs`/(4.2's) `release.rs` binaries, so the attribute is vestigial from this commit on —
  harmless to leave (an unused `no_mangle` export costs nothing at runtime), fine to drop in the same commit
  if touching that line anyway, not worth its own commit either way. Test: compile gate + smoke test both
  modules (swap `config.toml`'s `app_name` between them), and assert the dependency is gone with
  `cargo tree -p surge-core | grep libloading` returning nothing (Q81c). Pass 7–13 specified an
  `ldd`/`nm` symbol check here; `ldd` is simply the wrong tool (`libloading` is a Rust rlib, statically
  linked — it was never going to appear as a shared-object dependency, so that half of the check passes
  vacuously today and would keep passing if the dependency were still there), and `nm` on a statically
  linked binary depends on whether the linker happened to garbage-collect unreferenced sections. The
  `cargo tree` assertion tests the actual claim — the dependency edge is gone — deterministically.
  *(As built: `SURGE_DEV_LOADER`, `StartupApp`, `surge-core/src/app/mod.rs`, `pub mod app;`, `AppError`,
  `ModuleError` and the `libloading` dependency are gone; `surge-player` imports `surge_sdk::SurgeApp` and
  `startup_app` is `Box<dyn SurgeApp>`. On non-Linux `main()` now panics, since the process loader is
  Linux-only. The `no_mangle` attributes were left alone. **The `cargo tree -p surge-core | grep libloading`
  assertion as written cannot pass:** `libloading 0.8.9` is still a transitive dependency via `ash` (and
  `winit`'s `dlib`). The check that holds is that `cargo tree -p surge-core --depth 1` no longer lists it —
  the direct edge, i.e. the `0.9` copy this step removes, is gone. Smoke tests with the env var unset:
  both modules ran as exactly one child while open and none after, each logging its startup line.)*
- **3.5.** Add `fn reload(&mut self) -> bool { false }` to `SurgeApp` (default no-op; Q45), overridden by
  `RemoteApp::reload` to run the inherent `RemoteApp::reload()` sequence from 2.6 and unconditionally return
  `true`. That sequence also sets `RemoteApp`'s `pending_on_load` flag once the re-handshake succeeds, and
  `RemoteApp::on_swapchain_recreate` consumes it by sending `Request::OnLoad { create_info }` instead of
  `Request::OnSwapchainRecreate` (Q60) — without it the respawned child's first ever request is
  `OnSwapchainRecreate` and it never receives `OnLoad` at all, contradicting both the connect sequence and
  Q14. Both real modules do the same work in either hook, so *the rendered result* cannot distinguish the
  two — but the module's own log can, now that 2.3's `drive()` names each `Request` it receives (Q76): a
  correct reload prints `OnLoad` from the fresh child, a broken one prints `OnSwapchainRecreate`, and the
  smoke test below reads that line instead of asserting an invariant it cannot see. That is the difference
  between a test that documents Q60's fix and one that can fail if the fix is missing. Add the throwaway F5 reload keybind (Q40) in `surge-player`'s dev binary: `SurgeContext`'s
  `WindowEvent::KeyboardInput` arm (`surge-player/src/main.rs:295-300`) special-cases F5 before forwarding
  to the module — `self.recreate_swapchain |= self.startup_app.reload();` instead of calling
  `keyboard_event`, so F5 never reaches the module's own key handler and the Q14 rebuild-on-reload path
  (which only `SurgeContext` can trigger, via that same flag) actually fires. Test: smoke test — press F5
  mid-session against `surge-mod-2048`, confirm the board re-renders, that the fresh child's log shows
  `OnLoad` and not `OnSwapchainRecreate` as its first request (Q60/Q76), that `ps` shows exactly one
  module process before and after (a *different* pid — same count, so a leaked child and a failed respawn
  are distinguishable), and no texture-cap warning after several presses (proves the Q14 `SpriteDatabase`
  rebuild-on-reload fix). This proves kill/respawn/reconnect/rebuild work in the ordinary, non-racing case —
  it is **not** proof that `reload_in_progress` (Q22) is correct: F5 is handled in `window_event`, the
  blocked `Request::Frame` read happens in `about_to_wait`, and winit calls these sequentially on one
  thread, so a keypress can never actually be delivered *while* the main thread is blocked inside that read
  (Q46). Q22's flag is still needed — it's just that nothing in this plan, including this step, exercises
  the race it exists for; that only happens once a real, necessarily-async trigger (file watcher or
  similar) replaces F5, which is explicitly out of this doc's scope.
- **3.6.** Surface the crash state (Q11's policy, Q66's mechanism — the last unbuilt requirement in the dev
  design, and the one three earlier steps route failures into as if it already existed). Add the second
  default method to `SurgeApp` in `surge-sdk`, `fn is_faulted(&self) -> bool { false }`, overridden by
  `RemoteApp` to report the `faulted` flag 2.5 already sets. `SurgeContext::about_to_wait` checks it first
  thing: on the transition into it, one `log::error!`, `window.set_title` gains a `— MODULE CRASHED`
  suffix, and every subsequent frame returns early without acquiring, drawing, or presenting, so the last
  presented image stays on screen. `exiting()` on a faulted app skips the wire and goes straight to the
  reap (Q65). Additive to the trait, same shape as 3.5's `reload()`, so release-mode apps inherit the
  default and never fault. Test: this step carries the plan's fault-injection cases, because it is the
  first commit where a failure has anywhere to be *displayed* — and because five earlier steps wrote code
  whose failure branches nothing else ever runs (Q74, Q75, Q77, and 0.2's overflow `Err`). Each case is a
  few seconds of work and each can fail:
  1. **Child dies** (Q66): `kill -9` the child from another shell. The player logs loudly *once* (not once
     per frame), retitles the window, freezes on the last frame rather than blanking or panicking, and does
     not respawn anything; F5 then brings the module back and reverts the title — the only recovery path
     Q11 allows.
  2. **Player dies** (Q74, the case nothing in passes 7–13 ever ran): `kill -9` the *player*. Confirm
     `ps` shows no surviving `surge_mod_*` process. This is the sole test of `PR_SET_PDEATHSIG`; without
     it, 2.4's `set_pdeathsig` could be a silent no-op through every other step in this plan.
  3. **Module ignores `SIGTERM`** (Q75): rebuild the module dev binary with a temporary `loop {}` (or a
     long sleep) in `drive()`'s `Shutdown` arm, then close the window. The player must still exit within
     the bounded wait, having escalated to `SIGKILL`, leaving no orphan. Revert the patch after. This is
     the only exercise of the escalation branch — F5 and normal shutdown both hit a module that exits
     promptly, where a "SIGTERM, then give up" implementation would look identical.
  4. **Version mismatch** (Q77): bump `WIRE_VERSION` in `surge-sdk` and rebuild **only the player**
     (`cargo build -p surge-player`). The module binary already on disk still carries the old constant, so
     the pair genuinely disagrees — this is not artificial injection but exactly the real-world case the
     handshake exists for (a stale module binary after a protocol change). Confirm the player refuses to
     send `OnLoad`, enters the same crash state as case 1, and says so; then rebuild the module and
     confirm recovery.
  5. **Module stalls without dying** (Q77, and the one case that must *not* fault): `kill -STOP` the
     child. The player logs `waiting on module response — Nms and counting` repeatedly and stays
     connected — no crash state, no title change. `kill -CONT` and confirm it resumes rendering. This is
     the single test that distinguishes Q67's stall rule from Q66's fault rule; conflating them is the
     likeliest implementation error in 2.5's read helper and nothing else would catch it.
  6. **Upload failure** (Q17's part 2, and the first execution of 0.2's `Err` branch — five phases after
     it was written): run with `max_textures` temporarily lowered below the module's asset count (e.g. 4)
     and confirm the failing `texture_uploads` entry routes into the same crash state rather than a partial
     upload or a silently wrong texture.

*(**Milestone C**: dev mode is exactly what this doc specifies, fully live, fully proven by hand, with zero
permanent scaffolding left over — every lifecycle hook delivered, reload working, and both failure
policies (stall, crash) visible rather than silent. `surge-player`'s binary has exactly one behavior, per
Q21.)*

### Phase 4 — Release mode

- **4.1.** Extract `surge-player`'s run loop into a `[lib]` target: move `SurgeContext`/the
  `ApplicationHandler` impl into `surge-player/src/lib.rs`, expose `pub fn run(app: Box<dyn SurgeApp>,
  config: EngineConfig)`; `surge-player`'s own `main.rs` (the dev binary) becomes `RemoteApp` setup +
  `run(Box::new(remote_app), config)`. **What must *not* move is as load-bearing as what does** (Q71):
  `mod remote_app;` and `mod proc;` stay declared from `main.rs`, so they compile into the binary crate
  only. Every release binary links this new `[lib]`, and moving the socket/spawn/signal code into it would
  compile the whole IPC machinery into shipped games — the identical objection Q20 used to keep that code
  out of `surge-core`, one crate over. The `[lib]`'s only knowledge of dev mode after this step is the two
  default `SurgeApp` methods (`reload`, `is_faulted`) it calls through the trait, which is exactly the
  point of their being trait methods. **`run()` also takes over process-wide startup** (Q62): the
  `sc::cli::print_logo()` and `init_env_logger()` calls at `main.rs:327,332` move *into* `run()` and out of
  `main.rs` — moved, not copied, since a second `env_logger::Builder::init()` in one process panics (Q51's
  failure, one crate over). Without this, 4.2's release binary — which never runs `surge-player`'s `main()`
  — ships with no logger installed and every `log::` call in `surge-core` and the module silently dropped. `SurgeContext.startup_app` is already plain `Box<dyn SurgeApp>` by
  this point (the `StartupApp` scaffolding collapsed at 3.4, per Q43) — this step is pure code motion (move
  the struct/impl, wrap the existing logic in a function signature), not where that retyping happens; earlier
  drafts of this doc described it that way, which is corrected in "Release mode" above. Doing this *after*
  Phase 3 rather than alongside it means the extraction is a mechanical move of already-proven code, not
  extraction-and-protocol-swap at once. **Also in this commit** (Q54): move `surge-core/src/config/mod.rs`
  (`WindowConfig`/`ResolutionConfig`/`RendererConfig`/`ClearColorConfig`/`StartupAppConfig`/`EngineConfig`/
  `parse_config`) and `ConfigError` (`surge-core/src/errors/mod.rs`, `config/mod.rs`'s only other caller) to
  `surge-sdk` wholesale, adding `toml = "0.9.8"` to `surge-sdk/Cargo.toml` (`log` is already there since
  3.2). **`ConfigError` also needs `thiserror`, not just `toml`** (Q57): checked directly against
  `surge-core/src/errors/mod.rs`'s current definition, `ConfigError` is `#[derive(Debug, Error)]` against
  `thiserror::Error`, same as every other error enum in that file — `thiserror` is a `surge-core`-only
  dependency today (nowhere else in the workspace), and nothing before this step ever adds it to `surge-sdk`.
  This same commit therefore also adds `thiserror = "2.0.17"` (matching `surge-core/Cargo.toml`'s pin) to
  `surge-sdk/Cargo.toml`, alongside `toml`. This is a prerequisite for 4.2 below, not an independent cleanup —
  nothing before Phase 4 needs a module to see `EngineConfig` at all, but 4.2's release `[[bin]]` target does,
  and it can't reach `surge-core`'s copy (module crates never depend on `surge-core` in this design, in either
  mode). `surge-core/src/lib.rs`'s `pub mod config;` becomes `pub use surge_sdk::config;` and `errors/mod.rs`'s
  local `ConfigError` becomes a `pub use surge_sdk::config::ConfigError;` — the same pure compatibility-shim
  pattern 1.3 already used for the six plain-data types, so the four existing `use crate::{config::
  EngineConfig, ...}` sites in `surge-core`'s Vulkan layer (`image.rs`, `ctx_swpc.rs`, `ctx_command.rs`,
  `ctx_new_drop.rs`) and `surge-player`'s own `sc::config::parse_config` call need no edits. This commit
  also **removes `serde` and `toml` from `surge-core/Cargo.toml`** (Q64a): `config/mod.rs` was the crate's
  only `use serde`, and `config/mod.rs` plus `ConfigError` its only `use toml` — both dead once this move
  lands, the same trace-through Q29 did for `libloading`. Test: compile gate + smoke test (dev mode
  unaffected by a re-export-shimmed move, same as 1.3's), plus confirm the logo and log output still appear
  now that `run()`, not `main()`, emits them.
- **4.2.** Per module, add the release `[[bin]]` target per Q37: `surge-player` as an `optional = true`
  dependency gated by a `release-bin` feature, a `src/bin/release.rs` —
  ```rust
  fn main() {
      let config = surge_sdk::config::parse_config("config.toml").unwrap();
      surge_player::run(surge_mod_2048::surge_register_app(), config);
  }
  ```
  (substituting the crate name per module; `config` now resolves per 4.1's move, Q54 — `release.rs` has no
  `surge-core` dependency to reach `EngineConfig`/`parse_config` through otherwise) with
  `required-features = ["release-bin"]` — same reason as 3.2's `dev.rs` (Q53): `App2048::new()` directly
  is unreachable from a separate `[[bin]]` crate, `surge_register_app()` isn't. The `[[bin]]` table carries
  an explicit `name = "surge_mod_2048_release"` (`surge_mod_default_release` for the other module) alongside
  its `path` and `required-features` (Q59): an unnamed `src/bin/release.rs` in both crates collides at
  `target/release/release` exactly as the dev binaries would, and the staged artifact needs a name that says
  which game it is. Test: compile gate on the dev target (must **not** pull in `surge-player`/`ash`/`vk-mem`
  — check with `cargo tree -p surge-mod-2048` with no features enabled) + `cargo build -p surge-mod-2048
  --features release-bin --release` + running the produced binary directly (no player process, no
  `config.toml` env-var games) against the module's own `config.toml`; confirm the logo/log output appears,
  which is what proves Q62's move into `run()` actually reaches this path. Also confirm the release goal
  itself rather than assuming it (Q71): `ps` shows no child process, and
  `nm -C target/release/surge_mod_2048_release | grep -Ei 'remote_app|postcard|UnixStream|prctl'` comes
  back empty — the binary links `surge_player`'s `[lib]`, and 4.1's target rule is what keeps the IPC
  machinery out of it. (`libc`/`postcard` are still *built* as dependencies of `surge-player`; the claim
  under test is that no reachable code in this binary uses them, not that Cargo skipped compiling them.)

*(**Milestone D**: release mode exists — a single self-contained binary per module, no `dlopen`, no IPC, no
`libloading` — and dev mode is unaffected.)*

### Phase 5 — Shippability cleanup

- **5.1.** Rewrite `stager/stager.py` (flagged, not designed, by resolved open question 7): stage a single
  per-module release binary (`target/release/<module>_release`, per 4.2's explicit `[[bin]] name` — Q59) +
  its `config.toml` + `assets/`, not a player-exe-plus-`.so` pair. Test:
  `python3 stager/stager.py stage surge-mod-2048`, run the staged binary from a directory other than the
  repo root, confirm assets resolve (exercises `resolve_asset_path`'s staged-path branch, now keyed off the
  *module's* `current_exe()` per the behavior change noted under Q32).
- **5.2.** Delete the now-fully-superseded `hot_reloading` feature flag from any leftover
  `surge-player/Cargo.toml` reference if 3.1–3.4 didn't already remove it (Q21). Sweep `CLAUDE.md`'s "Build
  & run" and "Frame lifecycle" sections for now-inaccurate statements (`load_from_dylib`, the `dlopen` ABI
  description, the CWD contract's `libsurge_mod_*.so` mention, the stale "no tests" line 1.2 already
  touched) — a documentation-only commit, not a design change, closing the loop this doc opened by citing
  `CLAUDE.md` as ground truth throughout. Test (Q79 — pass 7–13 gave this step none at all, and "it's only
  docs" is exactly how a stale `hot_reloading` feature or a leftover `load_from_dylib` reference survives):
  compile gate, plus `grep -rn "hot_reloading\|load_from_dylib\|libloading\|libsurge_mod" --include=*.toml
  --include=*.md --include=*.rs .` returning nothing outside this design doc's own history sections. A grep
  that must come back empty is a real test — it fails when the sweep was incomplete, which is the only way
  this commit can be wrong.

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
- Pass 9 didn't need the fifth kind of check pass 8 speculated about — it reused pass 4–7's method (open the
  real file, check what it actually declares, don't reason from memory or the plan's own paraphrase of it) but
  pointed it at ground passes 1–8 never covered: the not-yet-created `surge-sdk` crate's own `Cargo.toml`
  (open question 49) and the exact privacy modifiers already sitting in `surge-mod-2048`'s current source
  (open question 53), rather than the winit/nalgebra/orphan-rule questions passes 4–6 checked or the call-site
  completeness pass 7 checked. Two of its five findings (50, 51) are a variant one step removed from that: a
  step's own stated action not fully executed against the code it touches — "retype the trait" without
  retyping the free functions the trait's own module code routes its argument through on the way in; "swap the
  dependency" without removing the call site whose only remaining reason to compile *was* that dependency, and
  without noticing a second, already-existing call to the function it's also adding a fresh call to. One
  (52) is closer to pass 8's own shape — prose states a fix, no step performs it. So: five more gaps, found by
  widening *what* gets checked against reality rather than by inventing a new *way* of checking, which is
  worth stating plainly since it means pass 8's "there may be a ninth class" framing undersold how much
  mileage the existing methods still had, aimed at parts of the plan/codebase intersection nobody had pointed
  them at yet. Extend pass 8's own caveat one more time: no more gaps of the kinds passes 1 through 9 knew to
  look for, and no guarantee any of them checked every file the plan actually touches — only the ones each
  pass happened to open.
- Pass 10 split cleanly into the two things nine passes of "open the real file" hadn't tried yet: checking
  whether a step's own downstream *caller* can produce every value that step's signature demands (open
  question 54 — nobody had asked "can `release.rs` actually construct the `EngineConfig` `run()` wants,"
  only "does `run()`'s own signature/body make sense," which is a different question with a different
  answer), and checking a claim about the repository's state *as it stands today*, not as this plan would
  leave it, directly against that repository rather than against `CLAUDE.md`'s summary of it (open question
  55 — `surge-core` already has tests; step 1.2's "first test in the workspace" was never true, this doc's
  implementation status notwithstanding). Neither required a new *technique* — both are the same "open the
  real file, don't reason from memory or the plan's own paraphrase" discipline every source-checking pass
  since 4 has used — they're both instances of pointing that discipline at a target nobody had pointed it at:
  a step's caller's obligations rather than the step's own body, and a "this is true today" claim rather than
  a "this will be true once you add X" claim. Q54 is the more consequential of the two: unfixed, it's a
  compile error an implementer wouldn't hit until Phase 4, potentially session(s) after Phases 1–3 shipped
  and this doc was believed done with the hard part. Worth naming the pattern once more, since it's the same
  one four straight passes (7, 8, 9, 10) have now landed on from a different angle each time: a design doc
  that has been read for internal consistency as many times as this one has stops finding new bugs that way,
  but keeps finding them every time a pass asks a question about the plan's relationship to the actual
  repository that no earlier pass happened to ask — which is not evidence the method is exhausted, only that
  each pass so far has covered a strict subset of the questions worth asking. Nothing about pass 10's two
  findings suggests pass 11 wouldn't find an eleventh; anyone implementing this doc from here should keep
  verifying each step against the real, current source at the moment they write it, not treat "N passes with
  no findings would mean done" as a threshold this doc is close to hitting.
- Pass 11 confirmed pass 10's own prediction within one pass, and did it by asking two questions nothing in
  1–10 had asked in this exact form. The first (Q56, Q57) goes a level more granular than Q49/Q54's "was the
  dependency edge added at all" — it's "does the crate a step says to 'add' actually export, under its
  *default* features, the specific function/derive that step's own code calls" — checked against `postcard`'s
  and `serde`'s real `[features]` tables (both cached locally, read directly, not inferred from how some other
  crate in the workspace already happens to use them). It's notably the same failure *shape* Q26 already found
  once, for `nalgebra`'s `serde-serialize` feature — proof that catching a bug shape once in a doc this heavily
  reviewed doesn't retire it, it just means the next occurrence is sitting in a file nobody had opened yet
  (here, `surge-sdk`'s own two newest dependencies at 1.2, and a third, `thiserror`, missing by omission at
  4.1). The second (Q58) is a genuinely different question, not a deeper version of an old one: not "is this
  fix correct" (every fix from Q1–Q57 was individually checked and correct on its own terms) but "do two fixes
  that land in the *same commit*, each independently verified, actually agree with each other once combined" —
  Q50 and Q36 were each resolved carefully on their own terms, in passes 8 and 7 respectively, and neither
  pass's own verification had any reason to re-check the other's fix once its own was settled, so the conflict
  between them sat unnoticed through three more passes of "read the doc again." Worth naming as a checklist
  gap this doc's own methodology hadn't articulated yet: passes 7–10 all check a step (or a step-sequence)
  against reality; none of them, until now, checked whether the *union* of everything bundled into one specific
  commit is internally consistent, as distinct from each bundled piece being individually consistent with the
  source it's checked against. "Every fix in this doc is correct" and "every commit's bundle of fixes agrees
  with itself" turned out to be different claims — pass 12, if there is one, should keep both in view rather
  than assuming the first implies the second.
- Pass 12 kept that in view and it paid twice (Q60: Q14+Q44+Q45 each correct, their composition not; Q63: two
  backends of one trait method, each locally sensible, returning different values), but its distinctive
  question was a different one: **every pass from 4 through 11 checked source — none checked artifacts.**
  What does the build tool actually emit and name; which file does the player then execute; which lifecycle
  message does a respawned process actually receive; which binary actually runs the startup code. Every one
  of pass 12's findings lives in that gap, and Q59 is the sharpest illustration in the doc: `src/bin/dev.rs`
  is correct Rust, resolves every symbol, satisfies every dependency edge eleven passes verified — and
  produces a binary at a path the player never looks for and that the other module overwrites. No amount of
  reading source finds it; ten minutes of running `cargo build` in a throwaway two-package workspace finds it
  immediately, which is the same lesson Q8's IPC harness and Q33's compile spike each taught in their own
  domain and which this doc had never applied to the build itself. Two process points worth carrying forward.
  First, the "verify by running it" habit should extend past *compiles?* to *what did that produce, and does
  anything actually consume it by that name* — a question a design doc can answer with a scratch workspace in
  minutes, and an implementer otherwise answers by losing an afternoon to a silently-wrong module. Second,
  Q61 is a plain miss with no methodological excuse: Q52 wrote the version-skew argument out in full for
  `nalgebra`, and `winit` sat directly beneath it in the very same `Cargo.toml`, unexamined through three
  subsequent passes — when a pass finds a bug that belongs to a *category* (a shared external crate declared
  in two places), the next move is enumerating the category's other members, not fixing the instance and
  moving on. That is the fourth distinct time this doc has had to relearn some version of "the same bug shape
  is sitting in the file nobody opened yet" (Q26→Q56, Q36, Q53, now Q52→Q61), which is worth reading as a
  standing property of documents this size rather than a defect of any one pass.
- Pass 13 changed the unit of analysis rather than the depth of checking: every pass from 4 through 12 asked
  a question about *a step* (does it compile, does it name real things, does it emit the right file); pass 13
  asked about *the set* — take every behavior the design specifies, and check each one has a commit. Two did
  not (Q65's shutdown round trip, Q66's crash path), and the second is the more instructive: three separate
  steps route failures into "the Q11 crash path" as if it were existing machinery, and that phrase reads as
  an implemented thing precisely *because* it is cited so often. Frequent citation is not evidence of
  existence — arguably it is a mild counter-indicator, since a name that appears in five places and is
  defined in none has been treated as settled by everyone who read it. The generalization worth carrying:
  a design doc's cross-references are load-bearing claims that something exists, and they deserve the same
  "open the file and check" treatment passes 9–12 gave to function names and Cargo features. Two related
  observations. First, Q71 is Q61's lesson recurring in the shape the doc explicitly warned about — an
  argument (Q20's "release binaries must not link the IPC machinery") applied carefully to one crate and
  never re-run when the design later gave release binaries a dependency on a *different* crate holding that
  same code; enumerating a category's members has to happen again whenever the category gains a member, not
  once. Second, Q68 and Q70 show that "compile-blocking" bugs survive twelve passes when the broken commit
  is not the one that introduces the mistake: 1.2 omits a derive that fails at 2.2, 3.1 adds an import that
  fails at 3.4. A per-step read finds neither; only walking the plan as a single artifact does. If there is
  a pass 14, the highest-value question left is probably the mirror of this one — not "does every behavior
  have a commit" but "does every commit have a test that could actually fail," since several steps here are
  still gated only by the compile gate, and 2.5 in particular carries a named, accepted integration risk
  that nothing verifies until 3.2.
- Pass 14 asked that question and the suspicion was right, though not where it was aimed: 2.5's *named*
  risk was the safe kind — flagged, bounded, and closed at 3.2 — while the damage was in the unflagged
  kind. `PR_SET_PDEATHSIG` (Q74) had a verification plan ("deferred to 2.6/3.x") that pointed at steps
  which never test it, and that phrasing is worse than saying nothing: it reads as scheduled and is
  therefore never re-examined. The generalizable rule this pass ends on: **a deferral is only real if it
  names the step and the observable.** "Deferred to later" is how a test disappears; "first proven at 3.6
  case 2, `ps` after `kill -9` of the player" is a promise something can break. Two secondary observations
  worth carrying. First, four of this pass's eight findings were mechanical — extracting every `Test:`
  clause with `awk` and grepping the plan for `kill`/`cargo test` immediately exposed the four steps with
  no test at all and the fact that no test kills the player. When the artifact under review is a list, read
  it as a list, programmatically, rather than re-reading prose that has been re-read thirteen times.
  Second, the fault-injection cases 3.6 collects are cheap in a way this plan hadn't noticed for thirteen
  passes: because dev-mode modules are separate binaries on disk, half of them are produced by rebuilding
  *one side* (a stale module after a `WIRE_VERSION` bump is a genuine mismatched pair; a `loop {}` in the
  module's `Shutdown` arm needs no player rebuild) and the rest are `kill -9`/`kill -STOP` from another
  shell. Process isolation, adopted for reload safety, turns out to make the system's failure modes
  unusually easy to test — an argument for the architecture nobody had made yet. If there is a pass 15, the
  question left unasked is about the *ordering* of the plan's risk rather than its coverage: 3.6 now
  concentrates six failure-path tests in a single late commit, and whether some of those belong earlier —
  as soon as the code they exercise exists, rather than as soon as a screen can display the result — is a
  real sequencing question this pass did not open.
