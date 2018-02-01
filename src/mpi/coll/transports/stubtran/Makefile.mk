## -*- Mode: Makefile; -*-
##  (C) 2006 by Argonne National Laboratory.
##      See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/transports/stubtran

mpi_core_sources += \
    src/mpi/coll/transports/stubtran/stubtran_impl.c		\
    src/mpi/coll/transports/stubtran/tsp_stubtran.c

noinst_HEADERS += \
    src/mpi/coll/transports/stubtran/stubtran_impl.h 		\
    src/mpi/coll/transports/stubtran/tsp_stubtran.h 		\
    src/mpi/coll/transports/stubtran/tsp_stubtran_types.h
