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
    grouptest     \
    grouptest2    \
    groupcreate   \
    gtranks       \
    groupnullincl

# glpid is a whitebox test that uses mpiimpl.h; it is unlikely to build with the
# current build system setup
#EXTRA_PROGRAMS = glpid
