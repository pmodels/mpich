#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

from distutils.core import setup, Extension

mtv = Extension("mtv",["mtv.c"])

setup(name="mtv", version="1.0", ext_modules=[mtv])
