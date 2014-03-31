## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_sources +=                           \
    src/mpi/pt2pt/bsend.c                \
    src/mpi/pt2pt/bsend_init.c           \
    src/mpi/pt2pt/bufattach.c            \
    src/mpi/pt2pt/buffree.c              \
    src/mpi/pt2pt/cancel.c               \
    src/mpi/pt2pt/greq_start.c           \
    src/mpi/pt2pt/greq_complete.c        \
    src/mpi/pt2pt/ibsend.c               \
    src/mpi/pt2pt/improbe.c              \
    src/mpi/pt2pt/imrecv.c               \
    src/mpi/pt2pt/iprobe.c               \
    src/mpi/pt2pt/irecv.c                \
    src/mpi/pt2pt/irsend.c               \
    src/mpi/pt2pt/isend.c                \
    src/mpi/pt2pt/issend.c               \
    src/mpi/pt2pt/mprobe.c               \
    src/mpi/pt2pt/mrecv.c                \
    src/mpi/pt2pt/probe.c                \
    src/mpi/pt2pt/recv.c                 \
    src/mpi/pt2pt/recv_init.c            \
    src/mpi/pt2pt/request_free.c         \
    src/mpi/pt2pt/request_get_status.c   \
    src/mpi/pt2pt/rsend.c                \
    src/mpi/pt2pt/rsend_init.c           \
    src/mpi/pt2pt/send.c                 \
    src/mpi/pt2pt/send_init.c            \
    src/mpi/pt2pt/sendrecv.c             \
    src/mpi/pt2pt/sendrecv_rep.c         \
    src/mpi/pt2pt/status_set_cancelled.c \
    src/mpi/pt2pt/ssend.c                \
    src/mpi/pt2pt/ssend_init.c           \
    src/mpi/pt2pt/start.c                \
    src/mpi/pt2pt/startall.c             \
    src/mpi/pt2pt/test.c                 \
    src/mpi/pt2pt/test_cancelled.c       \
    src/mpi/pt2pt/testall.c              \
    src/mpi/pt2pt/testany.c              \
    src/mpi/pt2pt/testsome.c             \
    src/mpi/pt2pt/wait.c                 \
    src/mpi/pt2pt/waitall.c              \
    src/mpi/pt2pt/waitany.c              \
    src/mpi/pt2pt/waitsome.c

mpi_core_sources += \
    src/mpi/pt2pt/bsendutil.c     \
    src/mpi/pt2pt/mpir_request.c

noinst_HEADERS += src/mpi/pt2pt/bsendutil.h

