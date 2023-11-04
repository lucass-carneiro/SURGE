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
    if platform.system() == "Windows":
        src_player_path = os.path.join(
            configuration_folder, "player", args["<configuration>"])
        dst_player_path = args["--output"]
    else:
        src_player_path = os.path.join(configuration_folder, "player", "surge")
        dst_player_path = os.path.join(args["--output"], "surge")

    # Shaders
    src_shaders_path = os.path.join(args["--prefix"], "shaders")
    dst_shaders_path = os.path.join(args["--output"], "shaders")

    # On windows, copy all DLLs and .exe
    if platform.system() == "Windows":
        for file in os.listdir(src_player_path):
            if file.endswith(".exe") or file.endswith(".dll"):
                src_file = os.path.join(src_player_path, file)
                dst_file = os.path.join(dst_player_path, file)
                shutil.copy2(src_file, dst_file)
    else:
        shutil.copy2(src_player_path, dst_player_path)

    shutil.copytree(src_shaders_path, dst_shaders_path)

    # Mimalloc injection
    if platform.system() == "Windows":
        wd = os.path.abspath(os.getcwd())

        minject = os.path.abspath(
            os.path.join(
                args["--prefix"], "companion", "minject.exe"
            )
        )

        mimalloc = os.path.join(args["--prefix"], "vcpkg", "packages",
                                "mimalloc_x64-windows", "bin", "mimalloc.dll")
        shutil.copy2(mimalloc, args["--output"])

        os.chdir(args["--output"])

        subprocess.run([
            minject,
            "--force",
            "--inplace",
            "surge.exe"
        ])

        os.chdir(wd)

    print("Staging created")


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

    # Copy module. In windows, copy all DLLs
    if platform.system() == "Windows":
        src_module_path = os.path.join(
            args["--prefix"], args["<configuration>"], "modules", args["<module>"], args["<configuration>"])

        for file in os.listdir(src_module_path):
            if file.endswith(".dll"):
                src_file = os.path.join(src_module_path, file)
                dst_file = os.path.join(args["--output"], file)
                shutil.copy2(src_file, dst_file)
    else:
        module_name = module_name + ".so"
        src_module_path = os.path.join(
            args["--prefix"], args["<configuration>"], "modules", args["<module>"], module_name)
        shutil.copy2(src_module_path, os.path.join(
            args["--output"], module_name))

    # Copy config
    src_config_path = os.path.join(
        args["--prefix"], "modules", args["<module>"], "config.yaml")
    shutil.copy2(src_config_path, os.path.join(
        args["--output"], "config.yaml"))

    print("Staging populated")


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
