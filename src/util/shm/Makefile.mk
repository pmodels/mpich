## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2016 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_core_sources +=                  \
    src/util/shm/mpir_shm_alloc.c    \
    src/util/shm/mpir_shm_barrier.c

# there are no AC_OUTPUT_FILES headers, so builddir is unnecessary
AM_CPPFLAGS += -I$(top_srcdir)/src/util/shm

noinst_HEADERS +=                      \
    src/util/shm/mpir_generic_queue.h  \
    src/util/shm/mpir_shm_impl.h       \
    src/util/shm/mpir_shm.h
