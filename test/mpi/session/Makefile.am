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
    session       \
    session_mult_init \
    session_re_init \
    session_psets \
    session_self

session_mult_init_SOURCES = session.c
session_mult_init_CPPFLAGS = -DMULT_INIT $(AM_CPPFLAGS)
session_re_init_SOURCES = session.c
session_re_init_CPPFLAGS = -DRE_INIT $(AM_CPPFLAGS)