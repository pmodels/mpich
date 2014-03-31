## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2012 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_core_sources +=   \
    src/mpi/info/infoutil.c

mpi_sources +=                     \
    src/mpi/info/info_create.c    \
    src/mpi/info/info_delete.c    \
    src/mpi/info/info_dup.c       \
    src/mpi/info/info_free.c      \
    src/mpi/info/info_get.c       \
    src/mpi/info/info_getn.c      \
    src/mpi/info/info_getnth.c    \
    src/mpi/info/info_getvallen.c \
    src/mpi/info/info_set.c
