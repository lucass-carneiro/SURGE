import os
import shutil
import sys

root_dir = sys.argv[1]
build_type = sys.argv[2]

staging_dir = root_dir + "/staging_" + build_type
bin_dir = root_dir + "/" + build_type

module_name_list = os.listdir(bin_dir + "/surge_modules/")
module_name_list = [m for m in module_name_list if m not in (
    "Makefile", "CMakeFiles", "cmake_install.cmake")]

if not os.path.exists(staging_dir):
    print("Creating staging directory")
    os.makedirs(staging_dir)

    print("Copying config file")
    shutil.copy2(root_dir + "/config.yaml", staging_dir + "/config.yaml")

    print("Copying player")
    shutil.copy2(bin_dir + "/surge_player/surge", staging_dir + "/surge")

    print("Copying modules")
    for mod in module_name_list:
        print("  Copying", mod)
        shutil.copy2(bin_dir + "/surge_modules/" + mod + "/" +
                     mod + ".so", staging_dir + "/" + mod + ".so")

    print("Done")

else:
    print("Updating staging directory")

    if os.stat(root_dir + "/config.yaml").st_mtime != os.stat(staging_dir + "/config.yaml").st_mtime:
        print("Updating config file")
        shutil.copy2(root_dir + "/config.yaml", staging_dir + "/config.yaml")

    if os.stat(bin_dir + "/surge_player/surge").st_mtime != os.stat(staging_dir + "/surge").st_mtime:
        print("Updating player")
        shutil.copy2(bin_dir + "/surge_player/surge", staging_dir + "/surge")

    print("Done")
