## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/util/pointer

noinst_HEADERS += src/util/pointer/mpiu_pointer.h

mpi_core_sources +=
