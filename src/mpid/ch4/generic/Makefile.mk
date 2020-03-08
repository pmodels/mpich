## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2020 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

include $(top_srcdir)/src/mpid/ch4/generic/am/Makefile.mk

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/generic

noinst_HEADERS += src/mpid/ch4/generic/mpidig.h

mpi_core_sources += src/mpid/ch4/generic/mpidig_init.c
