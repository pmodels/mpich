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
    src/mpi/pt2pt/rsend.c                \
    src/mpi/pt2pt/rsend_init.c           \
    src/mpi/pt2pt/send.c                 \
    src/mpi/pt2pt/send_init.c            \
    src/mpi/pt2pt/sendrecv.c             \
    src/mpi/pt2pt/sendrecv_rep.c         \
    src/mpi/pt2pt/ssend.c                \
    src/mpi/pt2pt/ssend_init.c

mpi_core_sources += \
    src/mpi/pt2pt/bsendutil.c

noinst_HEADERS += src/mpi/pt2pt/bsendutil.h
