## -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*-
##
##  (C) 2006 by Argonne National Laboratory.
##      See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/allreduce_group/

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_core_sources += \
    src/mpi/coll/allreduce_group/allreduce_group.c

noinst_HEADERS +=                    \
    src/mpi/coll/allreduce_group/allreduce_group.h
