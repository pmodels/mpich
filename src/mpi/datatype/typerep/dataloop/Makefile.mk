##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

mpi_core_sources +=                                    \
    src/mpi/datatype/typerep/dataloop/dataloop.c                     \
    src/mpi/datatype/typerep/dataloop/dataloop_create_blockindexed.c \
    src/mpi/datatype/typerep/dataloop/dataloop_create_contig.c       \
    src/mpi/datatype/typerep/dataloop/dataloop_create_indexed.c      \
    src/mpi/datatype/typerep/dataloop/dataloop_create_struct.c       \
    src/mpi/datatype/typerep/dataloop/dataloop_create_vector.c       \
    src/mpi/datatype/typerep/dataloop/dataloop_iov.c                 \
    src/mpi/datatype/typerep/dataloop/looputil.c                     \
    src/mpi/datatype/typerep/dataloop/segment.c                      \
    src/mpi/datatype/typerep/dataloop/segment_count.c                \
    src/mpi/datatype/typerep/dataloop/segment_flatten.c              \
    src/mpi/datatype/typerep/dataloop/dataloop_debug.c

# several headers are included by the rest of MPICH
AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/datatype/typerep/dataloop

noinst_HEADERS +=                                        \
    src/mpi/datatype/typerep/dataloop/typesize_support.h         \
    src/mpi/datatype/typerep/dataloop/dataloop_internal.h \
    src/mpi/datatype/typerep/dataloop/veccpy.h
