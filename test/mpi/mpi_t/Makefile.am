##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_single.mtest

EXTRA_DIST = testlist

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS =     \
    mpi_t_str   \
    mpit_isendirecv \
    mpit_vars   \
    cvarwrite   \
    getindex

if NOT_STRICTMPI
noinst_PROGRAMS += \
    qmpi_test
endif
