"""SURGE stager.

Usage:
  stager.py [options] list
  stager.py [options] stage <module-name>

Commands:
  list   Lists available modules.
  stage  Creates a new staging for a module

Arguments:
  <module-name>  The name of a module.

Options:
  -h --help       Show this screen.
  --version       Show version.
  -o --overwrite  Overwrites data in a staging folder, even if the folder already exists
"""
from docopt import docopt
import sys
import os
import logging
import shutil
import common

LOG = logging.getLogger(__name__)
LOGGER_FORMAT = "[%(filename)s] SURGE Stager %(levelname)s: %(message)s"


def list_modules():
    modules = common.get_module_list()
    LOG.info("SURGE modules detected:")
    for module in modules:
        print(f"  {module}")


def make_staging(args):
    overwrite = bool(args["--overwrite"])

    av_modules = common.get_module_list()
    req_module = args["<module-name>"]

    # Check if the requested module actually exists
    if req_module not in av_modules:
        LOG.error(
            f"Cannot create staging of module {req_module}. No such module with this name"
        )
        exit(1)

    # Check if the player has been built in release mode
    if not common.player_release_binary_exists():
        exit(1)

    # Check if the requested module has been built in release mode
    if not common.module_release_binary_exists(req_module):
        exit(1)

    # Check if the module has a config file
    src_module_config = os.path.join(
        "surge-modules",
        req_module,
        "config.toml"
    )

    if not os.path.exists(src_module_config):
        LOG.error(f"No config file found for module {req_module}")
        exit(1)

    # Make staging folder
    staging_folder = f"staging-{req_module}"
    if os.path.exists(staging_folder) and overwrite:
        shutil.rmtree(staging_folder)
    elif os.path.exists(staging_folder) and not overwrite:
        LOG.info(
            f"Staging folder {staging_folder} already exists. To overwrite it, rerun with the \"-o\" option enabled"
        )
        exit(0)

    os.mkdir(staging_folder)

    # Copy Module
    mod_lib_name = common.make_lib_name(req_module)

    src_module_lib = os.path.join(
        "target",
        "release",
        mod_lib_name
    )

    dst_module_lib = os.path.join(
        staging_folder,
        mod_lib_name
    )

    shutil.copy2(src_module_lib, dst_module_lib)

    # Copy Player
    player_exe_name = common.make_exe_name("surge-player")

    src_player_exe = os.path.join(
        "target",
        "release",
        player_exe_name
    )

    dst_player_exe = os.path.join(
        staging_folder,
        player_exe_name
    )

    shutil.copy2(src_player_exe, dst_player_exe)

    # Copy config file
    dst_module_config = os.path.join(
        staging_folder,
        "config.toml"
    )

    shutil.copy2(src_module_config, dst_module_config)

    # Copy assets
    src_assets_folder = os.path.join(
        "surge-modules",
        req_module,
        "assets"
    )

    dst_assets_folder = os.path.join(
        staging_folder,
        "assets"
    )

    shutil.copytree(src_assets_folder, dst_assets_folder)


def main(args):
    if not common.cwd_is_surge_root():
        exit(1)

    if args["list"]:
        list_modules()
    elif args["stage"]:
        make_staging(args)


if __name__ == "__main__":
    logging.basicConfig(
        level=logging.INFO,
        format=LOGGER_FORMAT,
        handlers=[
            logging.StreamHandler(sys.stdout)
        ]
    )

    args = docopt(__doc__, version="SURGE stager v1.0.0")

    main(args)
