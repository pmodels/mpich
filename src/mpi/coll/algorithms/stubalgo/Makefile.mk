##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/algorithms/stubalgo

mpi_core_sources += \
    src/mpi/coll/algorithms/stubalgo/stubalgo.c

noinst_HEADERS += \
    src/mpi/coll/algorithms/stubalgo/stubalgo.h       \
    src/mpi/coll/algorithms/stubalgo/stubalgo_types.h
