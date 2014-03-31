## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_sources +=                 \
    src/mpi/init/abort.c       \
    src/mpi/init/init.c        \
    src/mpi/init/initialized.c \
    src/mpi/init/initthread.c  \
    src/mpi/init/ismain.c      \
    src/mpi/init/finalize.c    \
    src/mpi/init/finalized.c   \
    src/mpi/init/querythread.c

mpi_core_sources += \
    src/mpi/init/initinfo.c       \
    src/mpi/init/async.c

noinst_HEADERS += src/mpi/init/mpi_init.h

