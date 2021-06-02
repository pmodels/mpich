##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_single.mtest

EXTRA_DIST = testlist

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS = \
    attrself      \
    exitst1       \
    exitst2       \
    exitst3       \
    initstat      \
    session       \
    session_mult_init \
    session_psets \
    session_self  \
    version       \
    library_version \
    timeout       \
    finalized

session_mult_init_SOURCES = session.c
session_mult_init_CPPFLAGS = -DMULT_INIT $(AM_CPPFLAGS)
