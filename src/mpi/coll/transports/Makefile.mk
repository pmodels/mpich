##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/src/mpi/coll/transports/gentran/Makefile.mk

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/transports

noinst_HEADERS += \
    src/mpi/coll/transports/tsp_impl.h
