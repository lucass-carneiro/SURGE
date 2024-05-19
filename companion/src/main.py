"""SCOMP - SURGE Companion.

Usage:
  scomp staging new <configuration>
  scomp staging populate <configuration> <module>
  scomp (-h | --help)
  scomp --version

Options:
  -h --help              Show this screen.
  --version              Show version.
"""
from docopt import docopt
import staging as stg

import sys
import ctypes
import os


def get_os_and_privileges():
    admin = False
    wind = False
    try:
        admin = os.getuid() == 0
    except AttributeError:
        admin = ctypes.windll.shell32.IsUserAnAdmin() != 0
        wind = True

    return admin, wind


def main():
    args = docopt(__doc__, version="SCOMP - SURGE Companion. 1.0.0")
    admin, windows = get_os_and_privileges()

    if windows and not admin:
        print("Cannot execut scomp on windows withou admin privileges.")
        print("This is because Windows is a bad operating system and requires")
        print("people to have admin rights to create symbolic links")
        exit(1)

    if args["staging"] and args["new"]:
        if windows:
            stg.win_new_staging(args)
        else:
            stg.lin_new_staging(args)
    elif args["staging"] and args["populate"]:
        if windows:
            stg.win_populate(args)
        else:
            stg.win_populate(args)

    sys.exit(0)


if __name__ == "__main__":
    main()
