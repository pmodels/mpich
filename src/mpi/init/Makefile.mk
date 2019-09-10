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
    src/mpi/init/init_global.c    \
    src/mpi/init/init_thread_cs.c \
    src/mpi/init/init_async.c     \
    src/mpi/init/init_windows.c   \
    src/mpi/init/init_bindings.c  \
    src/mpi/init/init_dbg_logging.c \
    src/mpi/init/init_topo.c      \
    src/mpi/init/netloc_util.c

noinst_HEADERS += src/mpi/init/mpi_init.h
