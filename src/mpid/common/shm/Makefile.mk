## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2016 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_MPID_COMMON_SHM

mpi_core_sources +=                          \
    src/mpid/common/shm/mpidu_shm_alloc.c    \
    src/mpid/common/shm/mpidu_shm_barrier.c

# there are no AC_OUTPUT_FILES headers, so builddir is unnecessary
AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/common/shm

noinst_HEADERS +=                                 \
    src/mpid/common/shm/mpidu_generic_queue.h      \
    src/mpid/common/shm/mpidu_shm_impl.h      \
    src/mpid/common/shm/mpidu_shm.h

endif BUILD_MPID_COMMON_SHM
