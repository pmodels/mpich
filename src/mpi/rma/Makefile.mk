## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_sources +=                  \
    src/mpi/rma/accumulate.c    \
    src/mpi/rma/alloc_mem.c     \
    src/mpi/rma/free_mem.c      \
    src/mpi/rma/get.c           \
    src/mpi/rma/put.c           \
    src/mpi/rma/win_complete.c  \
    src/mpi/rma/win_create.c    \
    src/mpi/rma/win_fence.c     \
    src/mpi/rma/win_free.c      \
    src/mpi/rma/win_get_group.c \
    src/mpi/rma/win_get_name.c  \
    src/mpi/rma/win_lock.c      \
    src/mpi/rma/win_post.c      \
    src/mpi/rma/win_set_name.c  \
    src/mpi/rma/win_start.c     \
    src/mpi/rma/win_unlock.c    \
    src/mpi/rma/win_wait.c      \
    src/mpi/rma/win_test.c

noinst_HEADERS += src/mpi/rma/rma.h

lib_lib@MPILIBNAME@_la_SOURCES += \
    src/mpi/rma/winutil.c

