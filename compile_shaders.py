import sys
import platform
import os
import subprocess

# Command line args
current_dir = sys.argv[1]
shaderc_path = sys.argv[2]

# Directories
source_dir = current_dir + "/src/"
bin_dir = current_dir + "/bin/"

# Shader source files
vert_sources = [f for f in os.listdir(source_dir) if f.endswith(".vert")]
frag_sources = [f for f in os.listdir(source_dir) if f.endswith(".frag")]

# Platform detection
platform_arg = platform.system().lower()

# Compile all vertex shaders
for file in vert_sources:
    print("Compiling vertex shader", file)
    subprocess.check_call(
        [
            shaderc_path,
            "-f",
            source_dir + file,
            "-i",
            source_dir,
            "-o",
            bin_dir + file + ".bin",
            "--profile",
            "spirv",
            "--type",
            "vertex"
            "--varyingdef",
            source_dir
        ]
    )

# Compile all fragment shaders
for file in frag_sources:
    print("Compiling fragment shader", file)
    subprocess.check_call(
        [
            shaderc_path,
            "-f",
            source_dir + file,
            "-i",
            source_dir,
            "-o",
            bin_dir + file + ".bin",
            "--profile",
            "spirv",
            "--type",
            "fragment"
            "--varyingdef",
            source_dir
        ]
    )
