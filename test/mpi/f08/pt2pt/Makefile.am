# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2014 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile_f08.mtest

EXTRA_DIST = testlist

# avoid having to write many "foo_SOURCES = foo.f90" lines
AM_DEFAULT_SOURCE_EXT = .f90

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS = allpairf08 greqf08 statusesf08 mprobef08

greqf08_SOURCES = greqf08.f90 dummyf08.f90
