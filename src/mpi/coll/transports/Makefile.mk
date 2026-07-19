##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

include $(top_srcdir)/src/mpi/coll/transports/gentran/Makefile.mk

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/transports

noinst_HEADERS += \
    src/mpi/coll/transports/tsp_impl.h
