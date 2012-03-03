## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_sources +=                       \
    src/mpi/comm/comm_compare.c      \
    src/mpi/comm/comm_create.c       \
    src/mpi/comm/comm_create_group.c \
    src/mpi/comm/comm_dup.c          \
    src/mpi/comm/comm_free.c         \
    src/mpi/comm/comm_get_name.c     \
    src/mpi/comm/comm_group.c        \
    src/mpi/comm/comm_idup.c         \
    src/mpi/comm/comm_rank.c         \
    src/mpi/comm/comm_size.c         \
    src/mpi/comm/comm_remote_group.c \
    src/mpi/comm/comm_remote_size.c  \
    src/mpi/comm/comm_set_name.c     \
    src/mpi/comm/comm_split.c        \
    src/mpi/comm/comm_test_inter.c   \
    src/mpi/comm/intercomm_create.c  \
    src/mpi/comm/intercomm_merge.c   \
    src/mpi/comm/comm_split_type.c

lib_lib@MPILIBNAME@_la_SOURCES += \
    src/mpi/comm/commutil.c

noinst_HEADERS += src/mpi/comm/mpicomm.h

