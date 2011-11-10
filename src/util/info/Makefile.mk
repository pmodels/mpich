## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

lib_lib@MPILIBNAME@_la_SOURCES +=   \
    src/util/info/infoutil.c

mpi_sources +=                     \
    src/util/info/info_create.c    \
    src/util/info/info_delete.c    \
    src/util/info/info_dup.c       \
    src/util/info/info_free.c      \
    src/util/info/info_get.c       \
    src/util/info/info_getn.c      \
    src/util/info/info_getnth.c    \
    src/util/info/info_getvallen.c \
    src/util/info/info_set.c

noinst_HEADERS += src/util/info/mpiinfo.h


