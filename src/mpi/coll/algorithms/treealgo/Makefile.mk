##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/algorithms/treealgo

mpi_core_sources += \
    src/mpi/coll/algorithms/treealgo/treealgo.c \
    src/mpi/coll/algorithms/treealgo/treeutil.c

noinst_HEADERS += \
    src/mpi/coll/algorithms/treealgo/treealgo.h       \
    src/mpi/coll/algorithms/treealgo/treealgo_types.h \
    src/mpi/coll/algorithms/treealgo/treeutil.h
