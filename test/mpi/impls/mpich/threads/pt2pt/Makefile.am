##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_threads.mtest

EXTRA_DIST = testlist

AM_DEFAULT_SOURCE_EXT = .c

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS = sendrecv_vci_hint \
	multinic_infohints

# Copied from cxx/rma/Makefile.am
#BINDIR=${bindir}
