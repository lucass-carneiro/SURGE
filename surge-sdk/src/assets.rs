//! Module asset path resolution.
//!
//! A module's assets need to be found in two different working-directory layouts, and a module
//! has no way to tell which one it's running under:
//!
//! - **Dev**: the player runs from the repo root (see the CWD contract in `CLAUDE.md`), and a
//!   module's assets live at `surge-modules/<module_name>/assets/`.
//! - **Staged**: `stager/stager.py` copies a module's `assets/` folder to sit next to the
//!   player executable, so assets live at `<exe_dir>/assets/`.
//!
//! [`resolve_asset_path`] tries the staged, executable-relative location first and falls back to
//! the dev, repo-root-relative location if that doesn't exist, so a module can hardcode a single
//! relative asset path and have it resolve correctly in both a debug build run from the repo
//! root and a `stager.py`-produced release directory.

use std::path::{Path, PathBuf};

/// Resolves `relative_path` (e.g. `"board.png"`) against `module_name`'s assets folder.
///
/// `module_name` is the module's directory name under `surge-modules/` (e.g.
/// `"surge-mod-2048"`), used only for the dev fallback.
pub fn resolve_asset_path(module_name: &str, relative_path: &str) -> PathBuf {
    if let Ok(exe_path) = std::env::current_exe() {
        if let Some(exe_dir) = exe_path.parent() {
            let staged_path = exe_dir.join("assets").join(relative_path);
            if staged_path.exists() {
                return staged_path;
            }
        }
    }

    Path::new("surge-modules")
        .join(module_name)
        .join("assets")
        .join(relative_path)
}
