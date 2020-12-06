##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/src/mpi/datatype/typerep/Makefile.mk

# for datatype.h, which is included by some other dirs
AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/datatype

## what's the scoop here with win_sources?
##    src/mpi/datatype/register_datarep.c

noinst_HEADERS += src/mpi/datatype/datatype.h

mpi_core_sources +=                              \
    src/mpi/datatype/datatype_impl.c             \
    src/mpi/datatype/count_util.c                \
    src/mpi/datatype/typeutil.c                  \
    src/mpi/datatype/type_blockindexed.c         \
    src/mpi/datatype/type_create_pairtype.c      \
    src/mpi/datatype/type_debug.c
