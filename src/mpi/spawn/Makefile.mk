##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

mpi_core_sources += \
    src/mpi/spawn/spawn_impl.c

noinst_HEADERS += src/mpi/spawn/namepub.h

# for namepub.h, which is included by some other dirs
AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/spawn
