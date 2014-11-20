# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile.mtest

EXTRA_DIST = testlist

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
## We must not list the EXTRA_PROGRAMS in the noinst_PROGRAMS list because
## doing so will include them in the default targets
noinst_PROGRAMS = \
    adderr        \
    commcall      \
    errfatal      \
    predef_eh     \
    errstring2    \
    dynamic_errcode_predefined_errclass

EXTRA_PROGRAMS = errcode errring errstring

errcode_SOURCES = errcode.c errmsg.c

