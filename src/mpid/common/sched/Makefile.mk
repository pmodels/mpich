## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_MPID_COMMON_SCHED

mpi_core_sources +=      \
    src/mpid/common/sched/mpid_sched.c

# so that the the device (e.g., ch3) can successfully include mpid_sched_pre.h
# There are no AC_OUTPUT_FILES, so the builddir path does not need to be added.
AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/common/sched

noinst_HEADERS +=                          \
    src/mpid/common/sched/mpid_sched.h     \
    src/mpid/common/sched/mpid_sched_pre.h


endif BUILD_MPID_COMMON_SCHED

