##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi_t

mpi_core_sources += \
        src/mpi_t/mpit_finalize.c       \
        src/mpi_t/mpit_initthread.c     \
        src/mpi_t/mpit_impl.c \
        src/mpi_t/pvar_impl.c \
        src/mpi_t/mpit.c \
        src/mpi_t/events_impl.c \
        src/mpi_t/qmpi_register.c
