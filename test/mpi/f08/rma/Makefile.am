# This Makefile.am generated automatically by f77tof90
# from test/mpi/f77/rma/Makefile.am.  DO NOT EDIT
# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile_f08.mtest

EXTRA_DIST = testlist.in

# avoid having to write many "foo_SOURCES = foo.f90" lines
AM_DEFAULT_SOURCE_EXT = .f90

noinst_PROGRAMS = \
    winscale1f08    \
    winscale2f08    \
    winfencef08     \
    wingetf08       \
    winerrf08       \
    winnamef08      \
    wingroupf08     \
    winaccf08       \
    winaccf08       \
    c2f2cwinf08     \
    baseattrwinf08  \
    winattrf08      \
    winattr2f08

if BUILD_MPIX_TESTS
noinst_PROGRAMS += aintf08
endif

## this test was commented out in the simplemake version...
##    allocmem

c2f2cwinf08_SOURCES = c2f2cwinf08.f90 c2f902cwin.c
