## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_sources +=                          \
    src/mpi/rma/accumulate.c            \
    src/mpi/rma/alloc_mem.c             \
    src/mpi/rma/compare_and_swap.c      \
    src/mpi/rma/fetch_and_op.c          \
    src/mpi/rma/free_mem.c              \
    src/mpi/rma/get.c                   \
    src/mpi/rma/get_accumulate.c        \
    src/mpi/rma/put.c                   \
    src/mpi/rma/raccumulate.c           \
    src/mpi/rma/rget.c                  \
    src/mpi/rma/rget_accumulate.c       \
    src/mpi/rma/rput.c                  \
    src/mpi/rma/win_allocate.c          \
    src/mpi/rma/win_allocate_shared.c   \
    src/mpi/rma/win_attach.c            \
    src/mpi/rma/win_complete.c          \
    src/mpi/rma/win_create.c            \
    src/mpi/rma/win_create_dynamic.c    \
    src/mpi/rma/win_detach.c            \
    src/mpi/rma/win_fence.c             \
    src/mpi/rma/win_flush.c             \
    src/mpi/rma/win_flush_all.c         \
    src/mpi/rma/win_flush_local.c       \
    src/mpi/rma/win_flush_local_all.c   \
    src/mpi/rma/win_free.c              \
    src/mpi/rma/win_get_group.c         \
    src/mpi/rma/win_get_info.c          \
    src/mpi/rma/win_get_name.c          \
    src/mpi/rma/win_lock.c              \
    src/mpi/rma/win_lock_all.c          \
    src/mpi/rma/win_post.c              \
    src/mpi/rma/win_set_info.c          \
    src/mpi/rma/win_set_name.c          \
    src/mpi/rma/win_shared_query.c      \
    src/mpi/rma/win_start.c             \
    src/mpi/rma/win_sync.c              \
    src/mpi/rma/win_test.c              \
    src/mpi/rma/win_unlock.c            \
    src/mpi/rma/win_unlock_all.c        \
    src/mpi/rma/win_wait.c

mpi_core_sources += \
    src/mpi/rma/winutil.c         \
    src/mpi/rma/rmatypeutil.c

