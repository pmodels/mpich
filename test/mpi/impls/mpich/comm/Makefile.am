# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile.mtest

EXTRA_DIST = testlist.in

AM_DEFAULT_SOURCE_EXT = .c

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS = comm_infokey_hint_support

# Copied from cxx/rma/Makefile.am
#BINDIR=${bindir}
