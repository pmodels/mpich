#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

from distutils.core import setup, Extension

mtv = Extension("mtv",
                sources = ["mtv.c"],
                extra_compile_args = ["-O0", "-g"],
                extra_link_args = ["-O0", "-g"])

setup(name="mtv", version="1.0", ext_modules=[mtv])
