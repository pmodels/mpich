## -*- Mode: Makefile; -*-
##  (C) 2006 by Argonne National Laboratory.
##      See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.

include $(top_srcdir)/src/mpi/coll/transports/gentran/Makefile.mk
include $(top_srcdir)/src/mpi/coll/transports/stubtran/Makefile.mk

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/transports/common

noinst_HEADERS += \
    src/mpi/coll/transports/common/tsp_undef.h
