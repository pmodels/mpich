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
noinst_PROGRAMS = noalias noalias2 noalias3 rerr bcastlength reduce_local nb_noalias nb_rerr ibcastlength

ibcastlength_SOURCES = bcastlength.c
ibcastlength_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

nb_noalias_SOURCES = noalias.c
nb_noalias_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

nb_rerr_SOURCES = rerr.c
nb_rerr_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

