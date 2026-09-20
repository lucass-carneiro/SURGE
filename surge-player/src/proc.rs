//! Process-lifecycle primitives for the dev-mode module child: make sure the child dies with the
//! player (crash included), and give it a chance at its own teardown before a hard kill.
//!
//! The mechanism is platform-specific. On Linux, `PR_SET_PDEATHSIG` is set *inside* the child
//! between `fork` and `exec`, so the kernel enforces it from the child's first instruction. Windows
//! has no such per-child signal; the equivalent is a Job Object created by the *parent* with
//! `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`, which the child is assigned to right after `spawn`. That
//! leaves a narrow window — between `spawn` and `attach` — where a player crash could still orphan
//! the child; `pre_exec` has no such window. Accepted as a platform limitation rather than
//! engineered around (it would need raw `CreateProcess` with `CREATE_SUSPENDED` in place of
//! `std::process::Command`), since modules are trusted, first-party code, not adversarial.

use std::io;
use std::process::{Child, Command};

/// Ties the module child's life to the player's. Created once and reused across reloads (`attach`
/// re-targets the same underlying mechanism at each freshly spawned child).
pub struct DeathGuard(imp::DeathGuardImpl);

impl DeathGuard {
    pub fn new() -> io::Result<Self> {
        Ok(Self(imp::DeathGuardImpl::new()?))
    }

    /// Called before `Command::spawn`. Installs the mechanism pre-exec where that's how it works
    /// (Linux); a no-op where it instead attaches from the parent side after spawn (Windows).
    pub fn configure(&self, command: &mut Command) {
        self.0.configure(command);
    }

    /// Called right after a successful `spawn`. A no-op where `configure` already did the job.
    pub fn attach(&self, child: &Child) -> io::Result<()> {
        self.0.attach(child)
    }
}

/// Sends the platform's graceful-stop signal, giving the module a chance to run its own teardown
/// before `remote_app::stop_child`'s harder fallback. On Linux this is `SIGTERM`, distinct from and
/// softer than `Child::kill()`'s `SIGKILL`. Windows has no signal a process can trap the way
/// `SIGTERM` works, so there this *is* the hard kill (`TerminateProcess`) — `stop_child`'s
/// subsequent `child.kill()` fallback then finds the child already gone and is a no-op.
pub fn terminate(pid: u32) -> io::Result<()> {
    imp::terminate(pid)
}

#[cfg(target_os = "linux")]
mod imp {
    use super::*;
    use std::os::unix::process::CommandExt;

    pub struct DeathGuardImpl;

    impl DeathGuardImpl {
        pub fn new() -> io::Result<Self> {
            Ok(Self)
        }

        pub fn configure(&self, command: &mut Command) {
            // SAFETY: `set_pdeathsig` only calls `prctl`, which is async-signal-safe as `pre_exec`
            // requires.
            unsafe { command.pre_exec(set_pdeathsig) };
        }

        pub fn attach(&self, _child: &Child) -> io::Result<()> {
            Ok(())
        }
    }

    /// Asks the kernel to `SIGKILL` this process when its parent dies, so a player that crashes
    /// without running `exiting()` cannot orphan its module process. Meant to run from a
    /// `Command::pre_exec` closure in the child, between `fork` and `exec`.
    fn set_pdeathsig() -> io::Result<()> {
        // SAFETY: `prctl` with `PR_SET_PDEATHSIG` only reads its integer arguments and is
        // async-signal-safe, as `pre_exec` requires.
        let rc = unsafe { libc::prctl(libc::PR_SET_PDEATHSIG, libc::SIGKILL as libc::c_ulong) };
        if rc == 0 {
            Ok(())
        } else {
            Err(io::Error::last_os_error())
        }
    }

    pub fn terminate(pid: u32) -> io::Result<()> {
        // SAFETY: `kill` has no memory-safety preconditions.
        let rc = unsafe { libc::kill(pid as libc::pid_t, libc::SIGTERM) };
        if rc == 0 {
            Ok(())
        } else {
            Err(io::Error::last_os_error())
        }
    }
}

#[cfg(windows)]
mod imp {
    use super::*;
    use std::os::windows::io::AsRawHandle;

    use windows_sys::Win32::Foundation::{CloseHandle, HANDLE};
    use windows_sys::Win32::System::JobObjects::{
        AssignProcessToJobObject, CreateJobObjectW, JobObjectExtendedLimitInformation,
        SetInformationJobObject, JOBOBJECT_EXTENDED_LIMIT_INFORMATION,
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE,
    };
    use windows_sys::Win32::System::Threading::{OpenProcess, TerminateProcess, PROCESS_TERMINATE};

    pub struct DeathGuardImpl {
        job: HANDLE,
    }

    // The job handle is only ever touched through the WinAPI calls below, all of which are safe to
    // call from any thread; `DeathGuard` has no interior mutability to race on.
    unsafe impl Send for DeathGuardImpl {}
    unsafe impl Sync for DeathGuardImpl {}

    impl DeathGuardImpl {
        pub fn new() -> io::Result<Self> {
            // SAFETY: a null name and null security attributes are both explicitly allowed by
            // `CreateJobObjectW` and just mean "anonymous job, default security".
            let job = unsafe { CreateJobObjectW(std::ptr::null(), std::ptr::null()) };
            if job.is_null() {
                return Err(io::Error::last_os_error());
            }

            let mut info: JOBOBJECT_EXTENDED_LIMIT_INFORMATION = unsafe { std::mem::zeroed() };
            info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            // SAFETY: `job` was just created above, and `info` is a fully-initialized value of the
            // exact type this call expects, whose size we pass alongside it.
            let ok = unsafe {
                SetInformationJobObject(
                    job,
                    JobObjectExtendedLimitInformation,
                    &info as *const _ as *const _,
                    std::mem::size_of_val(&info) as u32,
                )
            };
            if ok == 0 {
                let e = io::Error::last_os_error();
                // SAFETY: `job` is a valid handle we own and have not yet closed.
                unsafe { CloseHandle(job) };
                return Err(e);
            }
            Ok(Self { job })
        }

        pub fn configure(&self, _command: &mut Command) {}

        pub fn attach(&self, child: &Child) -> io::Result<()> {
            // SAFETY: `self.job` is a live job handle owned by `self`, and `child` is a live
            // process handle owned by `child`.
            let ok =
                unsafe { AssignProcessToJobObject(self.job, child.as_raw_handle() as HANDLE) };
            if ok == 0 {
                Err(io::Error::last_os_error())
            } else {
                Ok(())
            }
        }
    }

    impl Drop for DeathGuardImpl {
        fn drop(&mut self) {
            // SAFETY: `self.job` is a valid handle we own; nothing else references it.
            unsafe {
                CloseHandle(self.job);
            }
        }
    }

    pub fn terminate(pid: u32) -> io::Result<()> {
        // SAFETY: `OpenProcess`/`TerminateProcess`/`CloseHandle` are ordinary WinAPI calls on a
        // `pid` the OS itself assigned (via `Child::id()`); the handle is closed on every path.
        unsafe {
            let handle = OpenProcess(PROCESS_TERMINATE, 0, pid);
            if handle.is_null() {
                return Err(io::Error::last_os_error());
            }
            let ok = TerminateProcess(handle, 1);
            let result = if ok == 0 {
                Err(io::Error::last_os_error())
            } else {
                Ok(())
            };
            CloseHandle(handle);
            result
        }
    }
}
