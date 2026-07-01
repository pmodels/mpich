##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/algorithms/stubalgo

mpi_core_sources += \
    src/mpi/coll/algorithms/stubalgo/stubalgo.c

noinst_HEADERS += \
    src/mpi/coll/algorithms/stubalgo/stubalgo.h       \
    src/mpi/coll/algorithms/stubalgo/stubalgo_types.h
