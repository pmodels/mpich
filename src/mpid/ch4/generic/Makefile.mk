## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2016 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/generic

noinst_HEADERS += src/mpid/ch4/generic/mpidig_send.h \
                  src/mpid/ch4/generic/mpidig_recv.h \
                  src/mpid/ch4/generic/mpidig_startall.h \
                  src/mpid/ch4/generic/mpidig.h

mpi_core_sources += src/mpid/ch4/generic/mpidig_globals.c \
                    src/mpid/ch4/generic/mpidig_init.c
