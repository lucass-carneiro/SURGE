"""Surge Build Tool.

Usage:
  sbt.py build [--target=<build-target>] [--config=<engine-config>]
  sbt.py run
  sbt.py build-run
  sbt.py (-h | --help)
  sbt.py --version

Options:
  --target=<build-target>   The build target [default: debug].
  --config=<engine-config>  The engine config file [default: config.yaml].
  -h --help                 Show this screen.
  --version                 Show version.
"""
from docopt import docopt

import os
import platform
import subprocess
import shutil


def copy_if_changed(src, dest):
    if not os.path.exists(dest) or os.path.getmtime(src) != os.path.getmtime(dest):
        shutil.copy2(src, dest)


def copy_to_new(src, dest):
    if os.path.exists(dest) and os.path.getmtime(src) != os.path.getmtime(dest):
        shutil.copy2(src, dest + ".new")
    else:
        shutil.copy2(src, dest)


def build(staging_dir, player_dir, shaders_dir, config_dir, target_dir, lib_ext):
    if not os.path.exists(staging_dir):
        print("Creating staging dir", staging_dir)
        os.makedirs(staging_dir)

    try:
        print("Building")
        subprocess.check_call(["cargo", "build"])
    except subprocess.CalledProcessError:
        print("Engine build process failed")

    print("Copying player")
    if platform.system() == "Windows":
        copy_if_changed(player_dir, os.path.join(staging_dir, "surge.exe"))
    else:
        copy_if_changed(player_dir, os.path.join(staging_dir, "surge"))

    print("Copying shaders")
    shutil.copytree(shaders_dir, os.path.join(
        staging_dir, "shaders"), dirs_exist_ok=True)

    print("Copying config")
    copy_if_changed(config_dir, staging_dir)

    print("Copying module files")
    for file in os.listdir(target_dir):
        _, ext = os.path.splitext(file)

        if ext == lib_ext:
            copy_to_new(os.path.join(target_dir, file),
                        os.path.join(staging_dir, file))


def run(staging_dir):
    print("Running engine")
    os.chdir(staging_dir)
    subprocess.check_call([os.path.join(".", "surge")])
    os.chdir("..")


def build_run(staging_dir, player_dir, shaders_dir,
              config_dir, target_dir, lib_ext):
    build(staging_dir, player_dir, shaders_dir,
          config_dir, target_dir, lib_ext)
    run(staging_dir)


def main(arguments):
    staging_dir = os.path.join(".", "staging")

    target_dir = os.path.join(".", "target", arguments["--target"])

    if platform.system() == "Windows":
        player_dir = os.path.join(target_dir, "surge_player.exe")
    else:
        player_dir = os.path.join(target_dir, "surge_player")

    if platform.system() == "Windows":
        lib_ext = ".dll"
    else:
        lib_ext = ".so"

    shaders_dir = os.path.join(".", "shaders")

    config_dir = os.path.join(".", arguments["--config"])

    if arguments["build"]:
        build(staging_dir, player_dir, shaders_dir,
              config_dir, target_dir, lib_ext)
    elif arguments["run"]:
        run(staging_dir)
    elif arguments["build-run"]:
        build_run(staging_dir, player_dir, shaders_dir,
                  config_dir, target_dir, lib_ext)


if __name__ == "__main__":
    arguments = docopt(__doc__, version="Surge Build Tool 1.0.0")
    main(arguments)
