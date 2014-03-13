# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile.mtest

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS = \
    self          \
    simple        \
    sendrecv      \
    srtest        \
    wtime         \
    netpipe       \
    patterns      \
    adapt

noinst_HEADERS = GetOpt.h
netpipe_SOURCES = netmpi.c GetOpt.c
adapt_SOURCES = adapt.c GetOpt.c
