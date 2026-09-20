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


def make_exe_name(basename: str):
    if os.name == "nt":
        return f"{basename}.exe"
    elif os.name == "posix":
        return basename


def make_release_bin_name(module_name: str):
    # The release [[bin]] is named after the library name (underscores), plus "_release"
    return make_exe_name(f"{str.replace(module_name, '-', '_')}_release")


def module_release_binary_exists(module_name: str):
    bin_path = os.path.join("target", "release", make_release_bin_name(module_name))

    if not os.path.exists(bin_path):
        LOG.error(
            f"{module_name} has not been built as a release binary. Make sure that the module is inside the \"surge-modules\" folder and build it with \"cargo build --release --features release-bin\""
        )
        return False

    return True
