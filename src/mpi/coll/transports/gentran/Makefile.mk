##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
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
