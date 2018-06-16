## -*- Mode: Makefile; -*-
##  (C) 2006 by Argonne National Laboratory.
##      See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.

AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/algorithms/recexchalgo

mpi_core_sources += \
    src/mpi/coll/algorithms/recexchalgo/recexchalgo.c

noinst_HEADERS += \
    src/mpi/coll/algorithms/recexchalgo/recexchalgo.h
