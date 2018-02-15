## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_sources +=                           \
    src/mpi/request/cancel.c               \
    src/mpi/request/greq_start.c           \
    src/mpi/request/greq_complete.c        \
    src/mpi/request/request_free.c         \
    src/mpi/request/request_get_status.c   \
    src/mpi/request/status_set_cancelled.c \
    src/mpi/request/start.c                \
    src/mpi/request/startall.c             \
    src/mpi/request/test.c                 \
    src/mpi/request/test_cancelled.c       \
    src/mpi/request/testall.c              \
    src/mpi/request/testany.c              \
    src/mpi/request/testsome.c             \
    src/mpi/request/wait.c                 \
    src/mpi/request/waitall.c              \
    src/mpi/request/waitany.c              \
    src/mpi/request/waitsome.c

mpi_core_sources += \
    src/mpi/request/mpir_request.c
