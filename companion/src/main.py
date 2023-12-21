"""SCOMP - SURGE Companion.

Usage:
  scomp staging new <configuration> [--prefix=<root>]
  scomp staging delete
  scomp staging populate <configuration> <module> [--prefix=<root>]
  scomp staging update <configuration> <module> [--prefix=<root>]
  scomp staging run <configuration>
  scomp (-h | --help)
  scomp --version

Options:
  -h --help              Show this screen.
  --version              Show version.
  --prefix=<root>        The SURGE root directory [default: .].
"""
from docopt import docopt

import staging

import sys


def main():
    args = docopt(__doc__, version="SCOMP - SURGE Companion. 1.0.0")

    if args["staging"]:
        output_folder = "staging-" + args["<configuration>"]
        staging.main(args, output_folder)

    sys.exit(0)


if __name__ == "__main__":
    main()
