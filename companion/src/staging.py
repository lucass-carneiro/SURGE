import sys
import shutil
import os
import platform
import subprocess


def new(args, output_folder):
    if os.path.exists(output_folder):
        print("Unable to create staging directory because it already exists.")
        sys.exit(1)

    configuration_folder = os.path.join(args["--prefix"], args["<configuration>"])
    if not os.path.exists(configuration_folder):
        print("Configuration", configuration_folder, "does not exist")
        sys.exit(1)

    os.mkdir(output_folder)

    # Player
    if platform.system() == "Windows":

        # Because the way the Profile build is created on windows, this ugly hack is necessary
        if (args["<configuration>"] == "Profile"):
            src_player_path = os.path.join(configuration_folder, "player", "Release")
        else:
            src_player_path = os.path.join(configuration_folder, "player", args["<configuration>"])

        dst_player_path = output_folder
    else:
        src_player_path = os.path.join(configuration_folder, "player", "surge")
        dst_player_path = os.path.join(output_folder, "surge")

    # Shaders
    src_shaders_path = os.path.join(args["--prefix"], "shaders")
    dst_shaders_path = os.path.join(output_folder, "shaders")
    shutil.copytree(src_shaders_path, dst_shaders_path)

    # On windows, copy all DLLs, .exe and other files
    if platform.system() == "Windows":
        for file in os.listdir(src_player_path):
            src_file = os.path.join(src_player_path, file)
            dst_file = os.path.join(dst_player_path, file)
            shutil.copy2(src_file, dst_file)
    else:
        shutil.copy2(src_player_path, dst_player_path)

    # Mimalloc injection
    if platform.system() == "Windows":
        wd = os.path.abspath(os.getcwd())

        minject = os.path.abspath(os.path.join(args["--prefix"], "companion", "minject.exe"))

        os.chdir(output_folder)

        subprocess.run([
            minject,
            "--force",
            "--inplace",
            "surge.exe"
        ])

        os.chdir(wd)

    print("Staging created")


def delete(args, output_folder):
    if os.path.exists(output_folder):
        shutil.rmtree(output_folder)
    else:
        print("Unable to remove output directory",
              output_folder, "because it does not exist")
        sys.exit(1)


def populate(args, output_folder):
    if not os.path.exists(output_folder):
        print("Unable to enter staging directory.")
        sys.exit(1)

    # Copy module. In windows, copy all DLLs
    if platform.system() == "Windows":
        if args["<configuration>"] == "Profile":
            src_module_path = os.path.join(
                args["--prefix"],
                args["<configuration>"],
                "modules", args["<module>"],
                "Release")
        else:
            src_module_path = os.path.join(
                args["--prefix"],
                args["<configuration>"],
                "modules", args["<module>"],
                args["<configuration>"])

        for file in os.listdir(src_module_path):
            if file.endswith(".dll"):
                src_file = os.path.join(src_module_path, file)
                dst_file = os.path.join(output_folder, file)
                shutil.copy2(src_file, dst_file)
    else:
        module_name = args["<module>"] + ".so"
        src_module_path = os.path.join(
            args["--prefix"],
            args["<configuration>"],
            "modules", args["<module>"],
            module_name
        )
        shutil.copy2(src_module_path, os.path.join(
            output_folder, module_name))

    # Copy config
    src_config_path = os.path.join(args["--prefix"], "modules", args["<module>"], "config.ini")
    dst_config_path = os.path.join(output_folder, "config.ini")
    shutil.copy2(src_config_path, dst_config_path)

    # Copy resources folder if it exists
    src_resources_path = os.path.join(args["--prefix"], "modules", args["<module>"], "resources")
    dst_resources_path = os.path.join(output_folder, "resources")

    if os.path.exists(src_resources_path):
        shutil.copytree(src_resources_path, dst_resources_path)

    print("Staging populated")


def update(args, output_folder):
    module_name = args["<module>"]
    if platform.system() == "Windows":
        module_name = module_name + ".dll"
    else:
        module_name = module_name + ".so"

    staged_module = os.path.join(output_folder, module_name)
    origin_module = os.path.join(
        args["--prefix"],
        args["<configuration>"],
        "modules", args["<module>"],
        module_name)

    if os.path.exists(staged_module):
        staged_mtime = os.stat(staged_module).st_mtime
        origin_mtime = os.stat(origin_module).st_mtime
        if staged_mtime != origin_mtime:
            shutil.copy2(origin_module, staged_module + ".new")
    else:
        print("Unable to update module because it was never populated")
        sys.exit(1)


def run(output_folder):
    if not os.path.exists(output_folder):
        print("Unable to enter staging directory.")
        sys.exit(1)

    exec_name = os.path.join(".", "surge")
    if platform.system() == "Windows":
        exec_name = exec_name + ".exe"

    os.chdir(output_folder)
    subprocess.run([exec_name])


def main(args, output_folder):
    if args["new"]:
        new(args, output_folder)
    elif args["delete"]:
        delete(args, output_folder)
    elif args["populate"]:
        populate(args, output_folder)
    elif args["update"]:
        update(args, output_folder)
    elif args["run"]:
        run(output_folder)

    sys.exit(0)
