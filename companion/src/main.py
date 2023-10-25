"""SCOMP - SURGE Companion.

Usage:
  scomp staging new <configuration> [--prefix=<root>] [--output=<output-dir>]
  scomp staging delete
  scomp staging populate <configuration> <module> [--prefix=<root>] [--output=<output-dir>]
  scomp staging update <configuration> <module> [--prefix=<root>] [--output=<output-dir>]
  scomp staging run
  scomp (-h | --help)
  scomp --version

Options:
  -h --help              Show this screen.
  --version              Show version.
  --prefix=<root>        The SURGE root directory [default: .].
  --output=<output-dir>  The output folder of the staging [default: staging].
"""
from docopt import docopt

import staging

import sys


def main():
    args = docopt(__doc__, version="SCOMP - SURGE Companion. 1.0.0")
    print(args)

    if args["staging"]:
        staging.main(args)

    sys.exit(0)


if __name__ == "__main__":
    main()
