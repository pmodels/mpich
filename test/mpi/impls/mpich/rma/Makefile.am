##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_single.mtest

EXTRA_DIST = testlist.in

AM_DEFAULT_SOURCE_EXT = .c

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS = \
	win_info

# Copied from cxx/rma/Makefile.am
#BINDIR=${bindir}
