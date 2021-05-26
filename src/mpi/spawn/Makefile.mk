##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

mpi_core_sources += \
    src/mpi/spawn/spawn_impl.c

noinst_HEADERS += src/mpi/spawn/namepub.h

# for namepub.h, which is included by some other dirs
AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/spawn
