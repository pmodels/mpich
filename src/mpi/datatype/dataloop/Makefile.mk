## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_core_sources +=                                    \
    src/mpi/datatype/dataloop/darray_support.c               \
    src/mpi/datatype/dataloop/dataloop.c                     \
    src/mpi/datatype/dataloop/dataloop_create.c              \
    src/mpi/datatype/dataloop/dataloop_create_blockindexed.c \
    src/mpi/datatype/dataloop/dataloop_create_contig.c       \
    src/mpi/datatype/dataloop/dataloop_create_indexed.c      \
    src/mpi/datatype/dataloop/dataloop_create_pairtype.c     \
    src/mpi/datatype/dataloop/dataloop_create_struct.c       \
    src/mpi/datatype/dataloop/dataloop_create_vector.c       \
    src/mpi/datatype/dataloop/segment.c                      \
    src/mpi/datatype/dataloop/segment_count.c                \
    src/mpi/datatype/dataloop/segment_flatten.c              \
    src/mpi/datatype/dataloop/subarray_support.c

# several headers are included by the rest of MPICH
AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/datatype

noinst_HEADERS +=                                        \
    src/mpi/datatype/dataloop/typesize_support.h

