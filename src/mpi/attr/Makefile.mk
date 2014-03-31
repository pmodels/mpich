## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_sources +=                                      \
    src/mpi/attr/attr_delete.c        \
    src/mpi/attr/attr_get.c           \
    src/mpi/attr/attr_put.c           \
    src/mpi/attr/comm_create_keyval.c \
    src/mpi/attr/comm_delete_attr.c   \
    src/mpi/attr/comm_free_keyval.c   \
    src/mpi/attr/comm_get_attr.c      \
    src/mpi/attr/comm_set_attr.c      \
    src/mpi/attr/keyval_create.c      \
    src/mpi/attr/keyval_free.c        \
    src/mpi/attr/type_create_keyval.c \
    src/mpi/attr/type_delete_attr.c   \
    src/mpi/attr/type_free_keyval.c   \
    src/mpi/attr/type_get_attr.c      \
    src/mpi/attr/type_set_attr.c      \
    src/mpi/attr/win_create_keyval.c  \
    src/mpi/attr/win_delete_attr.c    \
    src/mpi/attr/win_free_keyval.c    \
    src/mpi/attr/win_get_attr.c       \
    src/mpi/attr/win_set_attr.c

noinst_HEADERS += src/mpi/attr/attr.h

mpi_core_sources +=         \
    src/mpi/attr/attrutil.c \
    src/mpi/attr/dup_fn.c
