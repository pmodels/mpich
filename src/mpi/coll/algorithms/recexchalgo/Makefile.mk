##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/algorithms/recexchalgo

mpi_core_sources += \
    src/mpi/coll/algorithms/recexchalgo/recexchalgo.c

noinst_HEADERS += \
    src/mpi/coll/algorithms/recexchalgo/recexchalgo.h
