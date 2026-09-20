//! Process-lifecycle primitives for the dev-mode module child (Unix only).

use std::io;

/// Asks the kernel to SIGKILL this process when its parent dies, so a player that crashes without
/// running `exiting()` cannot orphan its module process. Meant to be called from a
/// `Command::pre_exec` closure in the child, between `fork` and `exec`.
pub fn set_pdeathsig() -> io::Result<()> {
    // SAFETY: `prctl` with `PR_SET_PDEATHSIG` only reads its integer arguments and is
    // async-signal-safe, as `pre_exec` requires.
    let rc = unsafe { libc::prctl(libc::PR_SET_PDEATHSIG, libc::SIGKILL as libc::c_ulong) };
    if rc == 0 {
        Ok(())
    } else {
        Err(io::Error::last_os_error())
    }
}

/// Sends SIGTERM to `pid`, giving the module a chance to exit cleanly. Deliberately not
/// `Child::kill()`, which sends SIGKILL and skips the module's own teardown.
pub fn terminate(pid: u32) -> io::Result<()> {
    // SAFETY: `kill` has no memory-safety preconditions.
    let rc = unsafe { libc::kill(pid as libc::pid_t, libc::SIGTERM) };
    if rc == 0 {
        Ok(())
    } else {
        Err(io::Error::last_os_error())
    }
}
