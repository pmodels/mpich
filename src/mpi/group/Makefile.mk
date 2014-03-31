## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_sources +=                            \
    src/mpi/group/group_compare.c         \
    src/mpi/group/group_difference.c      \
    src/mpi/group/group_excl.c            \
    src/mpi/group/group_free.c            \
    src/mpi/group/group_incl.c            \
    src/mpi/group/group_intersection.c    \
    src/mpi/group/group_range_excl.c      \
    src/mpi/group/group_range_incl.c      \
    src/mpi/group/group_rank.c            \
    src/mpi/group/group_size.c            \
    src/mpi/group/group_translate_ranks.c \
    src/mpi/group/group_union.c

mpi_core_sources += \
    src/mpi/group/grouputil.c

noinst_HEADERS += src/mpi/group/group.h

