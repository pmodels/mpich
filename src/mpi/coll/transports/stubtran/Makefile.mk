##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/transports/stubtran

mpi_core_sources += \
    src/mpi/coll/transports/stubtran/stubtran_impl.c		\
    src/mpi/coll/transports/stubtran/tsp_stubtran.c

noinst_HEADERS += \
    src/mpi/coll/transports/stubtran/stubtran_impl.h 		\
    src/mpi/coll/transports/stubtran/tsp_stubtran.h 		\
    src/mpi/coll/transports/stubtran/tsp_stubtran_types.h
