import os
import sys
from enum import Enum


class Config(Enum):
    DEBUG = 0
    RELEASE = 1
    PROFILE = 2
    UNKNOWN = 3


def parse_config(args):
    config_name = args["<configuration>"]

    if config_name.lower() == "debug":
        return Config.DEBUG
    elif config_name.lower() == "release":
        return Config.RELEASE
    elif config_name.lower() == "profile":
        return Config.PROFILE
    else:
        print("Unknown configuration", config_name)
        exit(1)


def get_config_folder(config: Config):
    if config == Config.DEBUG:
        return "Debug"
    elif config == Config.RELEASE:
        return "Release"
    elif config == Config.PROFILE:
        return "Profile"


def get_staging_folder(config: Config):
    if config == Config.DEBUG:
        return "staging-Debug"
    elif config == Config.RELEASE:
        return "staging-Release"
    elif config == Config.PROFILE:
        return "staging-Profile"


def make_staging_folder(staging_folder, config_folder):
    if os.path.exists(staging_folder):
        print(f"Unable to create staging directory {staging_folder} because it already exists.")
        sys.exit(1)

    if not os.path.exists(config_folder):
        print(f"Configuration folder {config_folder} does not exist")
        sys.exit(1)

    os.mkdir(staging_folder)


def win_make_engine_links(cwd, staging_folder, config_folder):
    shaders_src_folder = os.path.join(cwd, "shaders")
    shaders_dst_folder = os.path.join(cwd, staging_folder, "shaders")

    os.symlink(shaders_src_folder, shaders_dst_folder, True)

    player_src_folder = os.path.join(cwd, config_folder, "player", config_folder)
    player_dest_folder = os.path.join(cwd, staging_folder)

    for file in os.listdir(player_src_folder):
        _, file_ext = os.path.splitext(file)
        if file_ext == ".dll" or file_ext == ".exe":
            file_src = os.path.join(player_src_folder, file)
            file_dst = os.path.join(player_dest_folder, file)
            os.symlink(file_src, file_dst)


def win_new_staging(args):
    cwd = os.getcwd()
    config = parse_config(args)

    config_folder = get_config_folder(config)
    staging_folder = get_staging_folder(config)

    make_staging_folder(staging_folder, config_folder)
    win_make_engine_links(cwd, staging_folder, config_folder)


def win_populate(args):
    cwd = os.getcwd()
    config = parse_config(args)

    module_name = args["<module>"]
    module_dll = module_name + ".dll"

    config_folder = get_config_folder(config)
    staging_folder = get_staging_folder(config)
    module_folder = os.path.join(cwd, "modules", module_name)

    if not os.path.exists(module_folder):
        print(f"The module folder {module_folder} does not exist")
        exit(1)

    resources_src_folder = os.path.join(module_folder, "resources")
    resources_dst_folder = os.path.join(cwd, staging_folder, "resources")

    os.symlink(resources_src_folder, resources_dst_folder, True)

    module_src_folder = os.path.join(
        cwd,
        config_folder,
        "modules",
        module_name,
        config_folder,
        module_dll
    )
    module_dst_folder = os.path.join(cwd, staging_folder, module_dll)

    os.symlink(module_src_folder, module_dst_folder)
