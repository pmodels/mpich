##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/algorithms/treealgo

mpi_core_sources += \
    src/mpi/coll/algorithms/treealgo/treealgo.c \
    src/mpi/coll/algorithms/treealgo/treeutil.c

noinst_HEADERS += \
    src/mpi/coll/algorithms/treealgo/treealgo.h       \
    src/mpi/coll/algorithms/treealgo/treealgo_types.h \
    src/mpi/coll/algorithms/treealgo/treeutil.h
