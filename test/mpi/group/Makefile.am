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
noinst_PROGRAMS = \
    grouptest     \
    grouptest2    \
    groupcreate   \
    gtranks       \
    groupnullincl \
    gtranksperf

# gtranksperf uses fabs, which may require -lm
gtranksperf_LDADD = $(LDADD) -lm

# glpid is a whitebox test that uses mpiimpl.h; it is unlikely to build with the
# current build system setup
#EXTRA_PROGRAMS = glpid

