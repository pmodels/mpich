##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
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
    src/mpi/init/globals.c        \
    src/mpi/init/initinfo.c       \
    src/mpi/init/local_proc_attrs.c \
    src/mpi/init/mutex.c \
    src/mpi/init/init_async.c     \
    src/mpi/init/init_windows.c   \
    src/mpi/init/init_bindings.c  \
    src/mpi/init/init_dbg_logging.c

noinst_HEADERS += src/mpi/init/mpi_init.h
