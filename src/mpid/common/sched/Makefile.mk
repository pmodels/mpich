##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

if BUILD_MPID_COMMON_SCHED

mpi_core_sources +=      \
    src/mpid/common/sched/mpidu_sched.c

# so that the the device (e.g., ch3) can successfully include mpid_sched_pre.h
# There are no AC_OUTPUT_FILES, so the builddir path does not need to be added.
AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/common/sched

noinst_HEADERS +=                          \
    src/mpid/common/sched/mpidu_sched.h


endif BUILD_MPID_COMMON_SCHED
