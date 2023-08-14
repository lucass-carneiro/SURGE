import os
import shutil
import sys


def make_staging_paths(exe_ext, lib_ext, staging_dir, module_name):
    player_path = os.path.join(staging_dir, "surge" + exe_ext)

    module_bin_dir = os.path.join(staging_dir, "bin")
    module_path = os.path.join(module_bin_dir, module_name + lib_ext)

    config_path = os.path.join(staging_dir, "config.yaml")
    shaders_path = os.path.join(staging_dir, "shaders")

    resources_path = os.path.join(staging_dir, "resources")

    return (player_path, module_path, config_path, shaders_path, resources_path, module_bin_dir)


def copy_data(src_paths, dest_paths):
    os.symlink(src_paths[0], dest_paths[0])
    shutil.copy2(src_paths[1], dest_paths[1])
    os.symlink(src_paths[2], dest_paths[2])
    os.symlink(src_paths[3], dest_paths[3])
    os.symlink(src_paths[4], dest_paths[4])


def should_update(source, dest):
    if os.stat(source).st_mtime != os.stat(dest).st_mtime:
        return True
    else:
        return False


def update_data(src_paths, dest_paths):
    if should_update(src_paths[1], dest_paths[1]):
        shutil.copy2(src_paths[1], dest_paths[1] + ".new")


def main():
    if os.name == "posix":
        exe_ext = ""
        lib_ext = ".so"
    elif os.name == "nt":
        exe_ext = ".exe"
        lib_ext = ".dll"

    build_type = sys.argv[1]
    module_name = "2048"

    root_dir = os.getcwd()
    staging_dir = os.path.join(root_dir, "staging_" + build_type)
    bin_dir = os.path.join(root_dir, build_type)

    player_path = os.path.join(bin_dir, "surge_player", "surge" + exe_ext)
    module_path = os.path.join(
        bin_dir, "surge_modules", module_name, module_name + lib_ext)

    config_path = os.path.join(
        root_dir, "surge_modules", module_name, "config.yaml")
    core_shaders_path = os.path.join(root_dir, "surge_player", "shaders")

    resources_path = os.path.join(
        root_dir, "surge_modules", module_name, "resources")

    src_paths = (player_path, module_path, config_path,
                 core_shaders_path, resources_path)
    dest_paths = make_staging_paths(exe_ext, lib_ext, staging_dir, module_name)

    if not os.path.exists(staging_dir):
        os.makedirs(dest_paths[-1])
        copy_data(src_paths, dest_paths)
    else:
        update_data(src_paths, dest_paths)


main()
