//! Dev-mode `SurgeApp`: the module runs as a child process and this type forwards the trait's
//! calls to it over a Unix socket (Linux only — the abstract socket namespace has no equivalent
//! elsewhere). The module-side counterpart is `surge_sdk::run_dev_module`.

use std::{
    io::{self, Read},
    os::{
        linux::net::SocketAddrExt,
        unix::{
            net::{SocketAddr, UnixListener, UnixStream},
            process::CommandExt,
        },
    },
    path::{Path, PathBuf},
    process::{Child, Command},
    thread,
    time::{Duration, Instant},
};

use surge_sdk::config::StartupAppConfig;
use surge_sdk::{
    CanvasOps, InputEvent, KeyEvent, Request, Response, SpriteCanvas, SurgeApp, WIRE_VERSION,
    read_message,
    winit::event::{DeviceId, ElementState, MouseButton, MouseScrollDelta, TouchPhase},
    write_message,
};

use crate::proc;

/// How long a blocked read waits before logging that the module is slow. A stall is not a fault:
/// a module paused under a debugger must not look like a crash.
const STALL_LOG_INTERVAL: Duration = Duration::from_millis(500);
/// How long a freshly spawned module gets to connect and send its handshake.
const CONNECT_TIMEOUT: Duration = Duration::from_secs(10);
/// How long `on_unload` waits for `ShutdownAck` before giving up and killing the child.
const SHUTDOWN_ACK_TIMEOUT: Duration = Duration::from_secs(2);
/// How long a module gets to exit on its own after acking shutdown.
const EXIT_GRACE: Duration = Duration::from_millis(500);
/// How long a module gets to exit after `SIGTERM` before `SIGKILL`.
const TERM_GRACE: Duration = Duration::from_secs(2);

pub struct RemoteApp {
    // Kept for the lifetime of the app so a respawn can reuse the same socket name.
    listener: UnixListener,
    socket_name: String,
    module_path: PathBuf,
    child: Child,
    stream: UnixStream,
    /// Input received since the last `Request::Frame`, drained into the next one.
    pending_input: Vec<InputEvent>,
    /// Set when the module stops answering correctly. Every `SurgeApp` method is a no-op from
    /// then on.
    faulted: bool,
    /// True while `reload` is tearing the old module down and bringing the new one up. A
    /// transport failure in that window is the player's own doing, not a module crash, so
    /// `fault` ignores it.
    reload_in_progress: bool,
    /// Set by a successful `reload`: the respawned module has never seen `OnLoad`, so the next
    /// `on_swapchain_recreate` sends that instead.
    pending_on_load: bool,
}

impl RemoteApp {
    /// Binds the socket, spawns the module binary named by `config`, and completes the
    /// handshake. Fails if the binary is missing, never connects, or speaks another
    /// `WIRE_VERSION`.
    pub fn new(config: &StartupAppConfig) -> io::Result<Self> {
        let module_path = resolve_module_path(config)?;
        let socket_name = format!("surge-{}", std::process::id());
        let listener = UnixListener::bind_addr(&SocketAddr::from_abstract_name(&socket_name)?)?;
        let (child, stream) = launch(&listener, &socket_name, &module_path)?;
        Ok(Self {
            listener,
            socket_name,
            module_path,
            child,
            stream,
            pending_input: Vec::new(),
            faulted: false,
            reload_in_progress: false,
            pending_on_load: false,
        })
    }

    /// Replaces the module process with a freshly spawned one: stops the current child,
    /// relaunches from the same path (so a rebuilt binary is picked up), and re-handshakes. On
    /// success clears `faulted`, which makes this the only way out of the crash state. On failure
    /// the app stays faulted and the error is returned; calling `reload` again retries.
    ///
    /// The new module has received no `Request` yet and the caller's canvas still holds the old
    /// module's textures, so the caller must rebuild the canvas and send `OnLoad` next.
    pub fn reload(&mut self) -> io::Result<()> {
        self.reload_in_progress = true;
        stop_child(&mut self.child);
        let result = launch(&self.listener, &self.socket_name, &self.module_path);
        self.reload_in_progress = false;

        match result {
            Ok((child, stream)) => {
                self.child = child;
                self.stream = stream;
                self.pending_input.clear();
                self.faulted = false;
                self.pending_on_load = true;
                Ok(())
            }
            Err(e) => {
                self.fault(format_args!("reload failed: {e}"));
                Err(e)
            }
        }
    }

    fn fault(&mut self, reason: impl std::fmt::Display) {
        if self.reload_in_progress {
            log::debug!("ignoring failure during reload: {reason}");
            return;
        }
        if !self.faulted {
            log::error!("module faulted: {reason}");
        }
        self.faulted = true;
    }

    fn write_request(&mut self, request: &Request) -> io::Result<()> {
        write_message(&self.stream, request)
    }

    /// Reads one response, logging every `STALL_LOG_INTERVAL` while waiting. Only EOF or a
    /// transport error comes back as `Err`.
    fn read_response(&mut self) -> io::Result<Response> {
        read_response(&self.stream, None)
    }

    /// Sends `request` and applies the `CanvasOps` reply to `canvas`. Faults instead of
    /// returning an error.
    fn turn(&mut self, request: Request, canvas: &mut dyn SpriteCanvas) {
        if self.faulted {
            return;
        }
        if let Err(e) = self.write_request(&request) {
            self.fault(format_args!("sending request failed: {e}"));
            return;
        }
        match self.read_response() {
            Ok(Response::CanvasOps(ops)) => self.apply_ops(ops, canvas),
            Ok(other) => self.fault(format_args!("expected CanvasOps, got {other:?}")),
            Err(e) => self.fault(format_args!("reading response failed: {e}")),
        }
    }

    /// Texture uploads first, in order, since instances refer to them by position; then
    /// instances. The first failed upload stops everything: the module's positional
    /// `texture_id`s are wrong from that point on, so there is nothing sensible to reconcile.
    fn apply_ops(&mut self, ops: CanvasOps, canvas: &mut dyn SpriteCanvas) {
        for path in &ops.texture_uploads {
            if let Err(e) = canvas.upload_texture(path) {
                self.fault(format_args!("uploading texture {path:?} failed: {e}"));
                return;
            }
        }
        for instance in &ops.instances {
            canvas.add_instance(instance);
        }
    }
}

impl SurgeApp for RemoteApp {
    fn on_load(&mut self, canvas: &mut dyn SpriteCanvas) {
        let create_info = canvas.get_create_info();
        self.turn(Request::OnLoad { create_info }, canvas);
    }

    fn on_swapchain_recreate(&mut self, canvas: &mut dyn SpriteCanvas) {
        let create_info = canvas.get_create_info();
        // A faulted app must keep the flag: `turn` sends nothing, and the retry still needs it.
        let request = if self.pending_on_load && !self.faulted {
            self.pending_on_load = false;
            Request::OnLoad { create_info }
        } else {
            Request::OnSwapchainRecreate { create_info }
        };
        self.turn(request, canvas);
    }

    fn update(&mut self, dt: f32, canvas: &mut dyn SpriteCanvas) {
        let input_events = std::mem::take(&mut self.pending_input);
        self.turn(Request::Frame { dt, input_events }, canvas);
    }

    fn keyboard_event(&mut self, _: DeviceId, event: KeyEvent, is_synthetic: bool) {
        if !self.faulted {
            self.pending_input.push(InputEvent::Keyboard {
                event,
                is_synthetic,
            });
        }
    }

    fn mouse_button_event(&mut self, _: DeviceId, state: ElementState, button: MouseButton) {
        if !self.faulted {
            self.pending_input
                .push(InputEvent::MouseButton { state, button });
        }
    }

    fn mouse_wheel_event(&mut self, _: DeviceId, delta: MouseScrollDelta, phase: TouchPhase) {
        if !self.faulted {
            self.pending_input
                .push(InputEvent::MouseWheel { delta, phase });
        }
    }

    /// Always asks the player to rebuild its canvas, even if the respawn failed: the failure is
    /// recorded as a fault, and the rebuilt canvas is simply never drawn to.
    fn reload(&mut self) -> bool {
        if let Err(e) = RemoteApp::reload(self) {
            log::error!("reload failed: {e}");
        }
        true
    }

    fn is_faulted(&self) -> bool {
        self.faulted
    }

    /// Asks the module to shut down, then reaps it — escalating to `SIGTERM`/`SIGKILL` if it does
    /// not answer or exit — so no module process outlives the player's clean exit.
    fn on_unload(&mut self) {
        if !self.faulted {
            match self
                .write_request(&Request::Shutdown)
                .and_then(|()| read_response(&self.stream, Some(SHUTDOWN_ACK_TIMEOUT)))
            {
                Ok(Response::ShutdownAck) => {
                    // The module exits on its own right after acking.
                    if let Ok(Some(_)) = wait_exit(&mut self.child, EXIT_GRACE) {
                        return;
                    }
                }
                Ok(other) => log::warn!("expected ShutdownAck, got {other:?}"),
                Err(e) => log::warn!("no ShutdownAck from module: {e}"),
            }
        }
        stop_child(&mut self.child);
    }
}

fn resolve_module_path(config: &StartupAppConfig) -> io::Result<PathBuf> {
    let joined = Path::new(&config.app_folder).join(&config.app_name);
    joined.canonicalize().map_err(|e| {
        io::Error::new(
            e.kind(),
            format!("module binary {} not found: {e}", joined.display()),
        )
    })
}

/// Spawns the module, accepts its connection, and checks its handshake. On any failure the child
/// is stopped before returning, so nothing is left running.
fn launch(
    listener: &UnixListener,
    socket_name: &str,
    module_path: &Path,
) -> io::Result<(Child, UnixStream)> {
    let mut command = Command::new(module_path);
    command.arg(socket_name);
    // SAFETY: `set_pdeathsig` only calls `prctl`, which is async-signal-safe as `pre_exec`
    // requires.
    unsafe { command.pre_exec(proc::set_pdeathsig) };
    let mut child = command.spawn().map_err(|e| {
        io::Error::new(
            e.kind(),
            format!("spawning {} failed: {e}", module_path.display()),
        )
    })?;

    match connect(listener, &mut child) {
        Ok(stream) => Ok((child, stream)),
        Err(e) => {
            stop_child(&mut child);
            Err(e)
        }
    }
}

fn connect(listener: &UnixListener, child: &mut Child) -> io::Result<UnixStream> {
    let deadline = Instant::now() + CONNECT_TIMEOUT;
    listener.set_nonblocking(true)?;
    let stream = loop {
        match listener.accept() {
            Ok((stream, _)) => break stream,
            Err(e) if e.kind() == io::ErrorKind::WouldBlock => {
                // A module that dies before connecting would otherwise leave us waiting out the
                // whole timeout.
                if let Some(status) = child.try_wait()? {
                    return Err(io::Error::other(format!(
                        "module exited before connecting ({status})"
                    )));
                }
                if Instant::now() >= deadline {
                    return Err(io::Error::new(
                        io::ErrorKind::TimedOut,
                        "module did not connect in time",
                    ));
                }
                thread::sleep(Duration::from_millis(10));
            }
            Err(e) => return Err(e),
        }
    };
    listener.set_nonblocking(false)?;
    stream.set_nonblocking(false)?;
    stream.set_read_timeout(Some(STALL_LOG_INTERVAL))?;

    match read_response(
        &stream,
        Some(deadline.saturating_duration_since(Instant::now())),
    )? {
        Response::Handshake { wire_version } if wire_version == WIRE_VERSION => Ok(stream),
        Response::Handshake { wire_version } => Err(io::Error::new(
            io::ErrorKind::InvalidData,
            format!("module speaks wire version {wire_version}, player speaks {WIRE_VERSION}"),
        )),
        other => Err(io::Error::new(
            io::ErrorKind::InvalidData,
            format!("expected Handshake, got {other:?}"),
        )),
    }
}

/// Reads one `Response`. A read timeout is a stall: it is logged and the read continues, until
/// `limit` (if any) runs out, which is reported as `TimedOut`. Only EOF or a transport error
/// otherwise fails.
fn read_response(stream: &UnixStream, limit: Option<Duration>) -> io::Result<Response> {
    read_message(StallReader {
        stream,
        started: Instant::now(),
        deadline: limit.map(|limit| Instant::now() + limit),
    })
}

/// Retries timed-out reads under the `read_exact` inside `read_message`, so a stall that lands
/// mid-frame does not drop the bytes read so far and desynchronize the stream.
struct StallReader<'a> {
    stream: &'a UnixStream,
    started: Instant,
    deadline: Option<Instant>,
}

impl Read for StallReader<'_> {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        loop {
            match (&*self.stream).read(buf) {
                Err(e) if e.kind() == io::ErrorKind::Interrupted => {}
                Err(e)
                    if matches!(
                        e.kind(),
                        io::ErrorKind::WouldBlock | io::ErrorKind::TimedOut
                    ) =>
                {
                    if self.deadline.is_some_and(|d| Instant::now() >= d) {
                        return Err(io::Error::new(
                            io::ErrorKind::TimedOut,
                            "timed out waiting on module",
                        ));
                    }
                    log::warn!(
                        "waiting on module response — {}ms and counting",
                        self.started.elapsed().as_millis()
                    );
                }
                result => return result,
            }
        }
    }
}

/// Polls for the child's exit for up to `timeout`.
fn wait_exit(child: &mut Child, timeout: Duration) -> io::Result<Option<std::process::ExitStatus>> {
    let deadline = Instant::now() + timeout;
    loop {
        if let Some(status) = child.try_wait()? {
            return Ok(Some(status));
        }
        if Instant::now() >= deadline {
            return Ok(None);
        }
        thread::sleep(Duration::from_millis(5));
    }
}

/// `SIGTERM`, a bounded wait, then `SIGKILL`, and always a final `wait` so no zombie is left.
/// `Child::kill` alone would skip straight to `SIGKILL` and deny the module its own teardown.
/// Shared by shutdown, reload, and failed launches.
fn stop_child(child: &mut Child) {
    // Already exited (e.g. crashed, or stopped by an earlier call): reap it and stop, since
    // signalling a reaped pid could hit an unrelated process that reused it.
    if let Ok(Some(_)) = child.try_wait() {
        return;
    }
    if let Err(e) = proc::terminate(child.id()) {
        log::warn!("SIGTERM to module failed: {e}");
    }
    match wait_exit(child, TERM_GRACE) {
        Ok(Some(_)) => return,
        Ok(None) => log::warn!("module ignored SIGTERM for {TERM_GRACE:?}; sending SIGKILL"),
        Err(e) => log::warn!("waiting on module failed: {e}"),
    }
    if let Err(e) = child.kill() {
        log::warn!("SIGKILL to module failed: {e}");
    }
    if let Err(e) = child.wait() {
        log::warn!("reaping module failed: {e}");
    }
}
