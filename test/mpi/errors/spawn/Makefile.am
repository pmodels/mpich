##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_single.mtest

EXTRA_DIST = testlist

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS = badport unpub lookup_name \
				  connect_timeout_no_accept \
				  connect_timeout_mismatch

connect_timeout_no_accept_SOURCES  = connect_timeout.c
connect_timeout_mismatch_SOURCES   = connect_timeout.c

connect_timeout_no_accept_CPPFLAGS = -DTEST_NOACCEPT $(AM_CPPFLAGS)
connect_timeout_mismatch_CPPFLAGS  = -DTEST_MISMATCH $(AM_CPPFLAGS)
