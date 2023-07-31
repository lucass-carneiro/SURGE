import os
import shutil
import sys

if os.name == "posix":
    exe_ext = ""
    lib_ext = ".so"
elif os.name == "nt":
    exe_ext = ".exe"
    lib_ext = ".dll"

root_dir = sys.argv[1]
build_type = sys.argv[2]

staging_dir = root_dir + "/staging_" + build_type
bin_dir = root_dir + "/" + build_type

module_name_list = os.listdir(bin_dir + "/surge_modules/")
module_name_list = [m for m in module_name_list if m not in (
    "Makefile", "CMakeFiles", "cmake_install.cmake")]

config_file_name = root_dir + "/config.yaml"
staging_config_file_name = staging_dir + "/config.yaml"

bin_file_name = bin_dir + "/surge_player/surge" + exe_ext
staging_bin_file_name = staging_dir + "/surge" + exe_ext

if not os.path.exists(staging_dir):
    print("Creating staging directory")
    os.makedirs(staging_dir)

    print("Copying config file")
    shutil.copy2(config_file_name, staging_config_file_name)

    print("Copying player")
    shutil.copy2(bin_file_name, staging_bin_file_name)

    print("Copying modules")
    for mod in module_name_list:
        print("  Copying", mod)
        mod_file = bin_dir + "/surge_modules/" + mod + "/" + mod + lib_ext
        staging_mod_file = staging_dir + "/" + mod + lib_ext
        shutil.copy2(mod_file, staging_mod_file)

    print("Done")

else:
    print("Updating staging directory")

    if os.stat(config_file_name).st_mtime != os.stat(staging_config_file_name).st_mtime:
        print("Updating config file")
        shutil.copy2(config_file_name, staging_config_file_name)

    if os.stat(bin_file_name).st_mtime != os.stat(staging_bin_file_name).st_mtime:
        print("Updating player")
        shutil.copy2(bin_file_name, staging_bin_file_name)

    for mod in module_name_list:
        mod_file = bin_dir + "/surge_modules/" + mod + "/" + mod + lib_ext
        staging_mod_file = staging_dir + "/" + mod + lib_ext

        if os.stat(mod_file).st_mtime != os.stat(staging_mod_file).st_mtime:
            print("Updating module", mod)
            shutil.copy2(mod_file, staging_mod_file + ".new")

    print("Done")
