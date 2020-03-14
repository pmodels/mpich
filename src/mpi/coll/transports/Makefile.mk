##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/src/mpi/coll/transports/gentran/Makefile.mk
include $(top_srcdir)/src/mpi/coll/transports/stubtran/Makefile.mk

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/transports/common

noinst_HEADERS += \
    src/mpi/coll/transports/common/tsp_undef.h
