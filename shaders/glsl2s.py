#!/usr/bin/python

"""glsl2s: Convert a shader into a C++ static string.

Usage:
  glsl2s.py <shader>

Options:
  -h --help     Show this screen.
  --version     Show version.

"""
from docopt import docopt
import os
import subprocess
import re

if __name__ == '__main__':
  arguments = docopt(__doc__, version='glsl2s 1.0')

  input_shader_name = arguments["<shader>"]
  
  output_shader_base_name, output_shader_type = os.path.splitext(input_shader_name)
  output_shader_type = output_shader_type.replace(".", "_")

  output_shader_name_no_ext = output_shader_base_name + output_shader_type
  output_shader_name = output_shader_name_no_ext + ".hpp"

  shader_file = open(input_shader_name, "r")
  shader_file_data = shader_file.read()
  shader_file_data = shader_file_data.replace("\n", "\\n")
  shader_file_data = shader_file_data.replace("\"", "\\\"")
  shader_file_data = shader_file_data.replace("\'", "\\\'")
  shader_file_data = shader_file_data.replace("    ", "")
  shader_file_data = shader_file_data + "\\0"
  shader_file.close()

  shader_file_skeleton = f"""#ifndef {"SURGE_SHADER_" + output_shader_name_no_ext.upper() + "_HPP"}
#define {"SURGE_SHADER_" + output_shader_name_no_ext.upper() + "_HPP"}

namespace surge \u007b

constexpr const char *{"shader_" + output_shader_name_no_ext + "_src"} = "{shader_file_data}";

\u007d; // namespace surge

#endif // {"SURGE_SHADER_" + output_shader_name_no_ext.upper() + "_HPP"}
"""

  output_file = open(output_shader_name, "w")
  output_file.write(shader_file_skeleton)
  output_file.close()
  subprocess.call(["clang-format", "-i", output_shader_name])