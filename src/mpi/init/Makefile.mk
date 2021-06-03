##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# for mpi_init.h, which is included generated binding functions
AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/init

mpi_core_sources += \
    src/mpi/init/init_impl.c      \
    src/mpi/init/init_util.c      \
    src/mpi/init/mpir_init.c      \
    src/mpi/init/globals.c        \
    src/mpi/init/initinfo.c       \
    src/mpi/init/local_proc_attrs.c \
    src/mpi/init/mutex.c \
    src/mpi/init/init_async.c     \
    src/mpi/init/init_windows.c   \
    src/mpi/init/init_bindings.c  \
    src/mpi/init/init_dbg_logging.c

noinst_HEADERS += src/mpi/init/mpi_init.h
