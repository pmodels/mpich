##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/algorithms/recexchalgo

mpi_core_sources += \
    src/mpi/coll/algorithms/recexchalgo/recexchalgo.c

noinst_HEADERS += \
    src/mpi/coll/algorithms/recexchalgo/recexchalgo.h
