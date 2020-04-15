##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/allreduce_group/

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_core_sources += \
    src/mpi/coll/allreduce_group/allreduce_group.c

noinst_HEADERS +=                    \
    src/mpi/coll/allreduce_group/allreduce_group.h
