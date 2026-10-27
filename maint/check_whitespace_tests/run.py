#! /usr/bin/env python3
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# Runs maint/check_whitespace.py on every *.c file in this directory. Each
# file starts with "/* expect: CODES */" (or "/* expect: none */"); a file
# passes when the set of rule codes reported equals CODES.
#
# Usage: maint/check_whitespace_tests/run.py [-v] [files...]

import os
import re
import sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(here))
import check_whitespace  # noqa: E402


def main():
    args = sys.argv[1:]
    verbose = "-v" in args
    names = [a for a in args if a != "-v"]
    if not names:
        names = sorted(os.path.join(here, n) for n in os.listdir(here) if n.endswith(".c"))
    fails = 0
    for path in names:
        with open(path) as f:
            m = re.match(r"/\* expect: ([\w ,]+) \*/", f.readline())
        if not m:
            print("%s: missing '/* expect: ... */' line" % path)
            fails += 1
            continue
        want = set() if m.group(1) == "none" else set(re.findall(r"\w+", m.group(1)))
        reports = check_whitespace.check_file(path, None)
        got = {r[2] for r in reports}
        if got != want:
            print("FAIL %s: got %s, expected %s" % (os.path.basename(path),
                                                    ",".join(sorted(got)) or "none",
                                                    ",".join(sorted(want)) or "none"))
            fails += 1
        if verbose or got != want:
            for line, col, code, msg in reports:
                print("    %d:%d: %s %s" % (line, col, code, msg))
    print("%d of %d passed" % (len(names) - fails, len(names)))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
