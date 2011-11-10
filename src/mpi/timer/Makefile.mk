## -*- Mode: Makefile; -*-
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_sources +=            \
    src/mpi/timer/wtime.c \
    src/mpi/timer/wtick.c

lib_lib@MPILIBNAME@_la_SOURCES += \
    src/mpi/timer/mpidtime.c

