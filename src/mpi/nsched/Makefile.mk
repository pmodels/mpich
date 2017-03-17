## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2016 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_core_sources +=      \
    src/mpi/nsched/mpir_nsched_init.c \
    src/mpi/nsched/mpir_nsched_ops.c  \
    src/mpi/nsched/mpir_nsched_progress.c

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/nsched

noinst_HEADERS += src/mpi/nsched/mpir_nsched.h
