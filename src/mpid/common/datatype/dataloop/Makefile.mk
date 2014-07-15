## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

## this file is already guarded by an "if BUILD_MPID_COMMON_DATATYPE"

mpi_core_sources +=                                    \
    src/mpid/common/datatype/dataloop/darray_support.c               \
    src/mpid/common/datatype/dataloop/dataloop.c                     \
    src/mpid/common/datatype/dataloop/dataloop_create.c              \
    src/mpid/common/datatype/dataloop/dataloop_create_blockindexed.c \
    src/mpid/common/datatype/dataloop/dataloop_create_contig.c       \
    src/mpid/common/datatype/dataloop/dataloop_create_indexed.c      \
    src/mpid/common/datatype/dataloop/dataloop_create_pairtype.c     \
    src/mpid/common/datatype/dataloop/dataloop_create_struct.c       \
    src/mpid/common/datatype/dataloop/dataloop_create_vector.c       \
    src/mpid/common/datatype/dataloop/segment.c                      \
    src/mpid/common/datatype/dataloop/segment_count.c                \
    src/mpid/common/datatype/dataloop/segment_flatten.c              \
    src/mpid/common/datatype/dataloop/segment_packunpack.c           \
    src/mpid/common/datatype/dataloop/subarray_support.c

# several headers are included by the rest of MPICH
AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/common/datatype

noinst_HEADERS +=                                        \
    src/mpid/common/datatype/dataloop/dataloop.h         \
    src/mpid/common/datatype/dataloop/dataloop_parts.h   \
    src/mpid/common/datatype/dataloop/dataloop_create.h  \
    src/mpid/common/datatype/dataloop/typesize_support.h \
    src/mpid/common/datatype/dataloop/veccpy.h

