import sys
import shutil
import os
import platform
import subprocess


def new(args):
    if os.path.exists(args["--output"]):
        print("Unable to create staging directory because it already exists.")
        sys.exit(1)

    configuration_folder = os.path.join(
        args["--prefix"], args["<configuration>"])
    if not os.path.exists(configuration_folder):
        print("Configuration", configuration_folder, "does not exist")
        sys.exit(1)

    os.mkdir(args["--output"])

    # Player
    src_player_path = os.path.join(configuration_folder, "player", "surge")
    dst_player_path = os.path.join(args["--output"], "surge")

    if platform.system() == "Windows":
        src_player_path = src_player_path + ".exe"
        dst_player_path = dst_player_path + ".exe"

    # Shaders
    src_shaders_path = os.path.join(args["--prefix"], "shaders")
    dst_shaders_path = os.path.join(args["--output"], "shaders")

    shutil.copy2(src_player_path, dst_player_path)
    shutil.copytree(src_shaders_path, dst_shaders_path)


def delete(args):
    if os.path.exists(args["--output"]):
        shutil.rmtree(args["--output"])
    else:
        print("Unable to remove output directory",
              args["--output"], "because it does not exist")
        sys.exit(1)


def populate(args):
    if not os.path.exists(args["--output"]):
        print("Unable to enter staging directory.")
        sys.exit(1)

    module_name = args["<module>"]
    if platform.system() == "Windows":
        module_name = module_name + ".dll"
    else:
        module_name = module_name + ".so"

    src_module_path = os.path.join(
        args["--prefix"], args["<configuration>"], "modules", args["<module>"], module_name)

    src_config_path = os.path.join(
        args["--prefix"], "modules", args["<module>"], "config.yaml")

    shutil.copy2(src_module_path, os.path.join(args["--output"], module_name))
    shutil.copy2(src_config_path, os.path.join(
        args["--output"], "config.yaml"))


def update(args):
    module_name = args["<module>"]
    if platform.system() == "Windows":
        module_name = module_name + ".dll"
    else:
        module_name = module_name + ".so"

    staged_module = os.path.join(args["--output"], module_name)
    origin_module = os.path.join(
        args["--prefix"], args["<configuration>"], "modules", args["<module>"], module_name)

    if os.path.exists(staged_module):
        staged_mtime = os.stat(staged_module).st_mtime
        origin_mtime = os.stat(origin_module).st_mtime
        if staged_mtime != origin_mtime:
            shutil.copy2(origin_module, staged_module + ".new")
    else:
        print("Unable to update module because it was never populated")
        sys.exit(1)


def run(args):
    if not os.path.exists(args["--output"]):
        print("Unable to enter staging directory.")
        sys.exit(1)

    exec_name = os.path.join(".", "surge")
    if platform.system() == "Windows":
        exec_name = exec_name + ".exe"

    os.chdir(args["--output"])
    print(os.getcwd())
    subprocess.run([exec_name])


def main(args):
    if args["new"]:
        new(args)
    elif args["delete"]:
        delete(args)
    elif args["populate"]:
        populate(args)
    elif args["update"]:
        update(args)
    elif args["run"]:
        run(args)

    sys.exit(0)
