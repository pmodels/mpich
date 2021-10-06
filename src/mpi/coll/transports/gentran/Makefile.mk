##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/transports/gentran

mpi_core_sources += \
    src/mpi/coll/transports/gentran/gentran_impl.c      \
    src/mpi/coll/transports/gentran/gentran_utils.c	\
    src/mpi/coll/transports/gentran/tsp_gentran.c

noinst_HEADERS += \
    src/mpi/coll/transports/gentran/gentran_utils.h	\
    src/mpi/coll/transports/gentran/gentran_impl.h      \
    src/mpi/coll/transports/gentran/gentran_types.h
