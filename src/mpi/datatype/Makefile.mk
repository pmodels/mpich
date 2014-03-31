## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

# for datatype.h, which is included by some other dirs
AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/datatype

mpi_sources +=                                   \
    src/mpi/datatype/address.c                   \
    src/mpi/datatype/get_address.c               \
    src/mpi/datatype/get_count.c                 \
    src/mpi/datatype/get_elements.c              \
    src/mpi/datatype/get_elements_x.c            \
    src/mpi/datatype/pack.c                      \
    src/mpi/datatype/unpack.c                    \
    src/mpi/datatype/pack_size.c                 \
    src/mpi/datatype/status_set_elements.c       \
    src/mpi/datatype/status_set_elements_x.c     \
    src/mpi/datatype/type_get_name.c             \
    src/mpi/datatype/type_set_name.c             \
    src/mpi/datatype/type_size.c                 \
    src/mpi/datatype/type_size_x.c               \
    src/mpi/datatype/type_extent.c               \
    src/mpi/datatype/type_vector.c               \
    src/mpi/datatype/type_commit.c               \
    src/mpi/datatype/type_indexed.c              \
    src/mpi/datatype/type_hindexed.c             \
    src/mpi/datatype/type_struct.c               \
    src/mpi/datatype/type_contiguous.c           \
    src/mpi/datatype/type_free.c                 \
    src/mpi/datatype/type_hvector.c              \
    src/mpi/datatype/type_dup.c                  \
    src/mpi/datatype/type_get_envelope.c         \
    src/mpi/datatype/type_get_contents.c         \
    src/mpi/datatype/type_ub.c                   \
    src/mpi/datatype/type_lb.c                   \
    src/mpi/datatype/type_get_extent.c           \
    src/mpi/datatype/type_get_extent_x.c         \
    src/mpi/datatype/type_get_true_extent.c      \
    src/mpi/datatype/type_get_true_extent_x.c    \
    src/mpi/datatype/type_match_size.c           \
    src/mpi/datatype/type_create_struct.c        \
    src/mpi/datatype/type_create_hindexed.c      \
    src/mpi/datatype/type_create_hvector.c       \
    src/mpi/datatype/pack_external.c             \
    src/mpi/datatype/pack_external_size.c        \
    src/mpi/datatype/unpack_external.c           \
    src/mpi/datatype/type_create_indexed_block.c \
    src/mpi/datatype/type_create_hindexed_block.c \
    src/mpi/datatype/type_create_resized.c       \
    src/mpi/datatype/type_create_darray.c        \
    src/mpi/datatype/type_create_subarray.c

## what's the scoop here with win_sources?
##    src/mpi/datatype/register_datarep.c

noinst_HEADERS += src/mpi/datatype/datatype.h

mpi_core_sources +=              \
    src/mpi/datatype/typeutil.c

