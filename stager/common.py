import os
import tomllib
import logging

LOG = logging.getLogger(__name__)


def get_module_list():
    return os.listdir("surge-modules")


def cwd_is_surge_root():
    cwd = os.getcwd()

    # Test 1: Do we have a Cargo.toml file?
    if not os.path.exists(os.path.join(cwd, "Cargo.toml")):
        LOG.error("CWD is not SURGE root. Cargo.toml not found")
        return False

    # Test 2: Does the Cargo.toml file cointain the correct members?
    with open("Cargo.toml", "rb") as f:
        data = tomllib.load(f)

        if "workspace" not in data:
            LOG.error("Malformed Cargo.toml file")
            return False

        if "members" not in data["workspace"]:
            LOG.error("Malformed Cargo.toml file")
            return False

        members = data["workspace"]["members"]
        has_all_mambers = ("surge-core" in members) and (
            "surge-player" in members) and ("surge-modules/*" in members)

        if not has_all_mambers:
            LOG.error(
                "Cargo.toml does not have the expected SURGE crates as workspace members"
            )
            return False

    return True


def make_lib_name(basename: str):
    _basename = str.replace(basename, "-", "_")

    if os.name == "nt":
        return f"{_basename}.dll"
    elif os.name == "posix":
        return f"lib{_basename}.so"


def make_exe_name(basename: str):
    if os.name == "nt":
        return f"{basename}.exe"
    elif os.name == "posix":
        return basename


def module_release_binary_exists(module_name: str):
    release_folder = os.path.join("target", "release")

    mod_lib_name = make_lib_name(module_name)
    mod_lib_path = os.path.join(release_folder, mod_lib_name)

    if not os.path.exists(mod_lib_path):
        LOG.error(
            f"{module_name} has not been built in release mode. Make sure that the module is inside the  \"surge-modules\" folder and build it with \"cargo build --release\""
        )
        return False

    return True


def player_release_binary_exists():
    release_folder = os.path.join("target", "release")
    player_base_name = "surge-player"

    player_name = make_exe_name(player_base_name)

    player_path = os.path.join(release_folder, player_name)

    if not os.path.exists(player_path):
        LOG.error(
            f"The SURGE player has not been built in release mode. Make sure to build it with \"cargo build --release\""
        )
        return False

    return True
