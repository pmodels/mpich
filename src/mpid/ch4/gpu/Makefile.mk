## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2020 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/gpu

noinst_HEADERS += src/mpid/ch4/gpu/pipeline.h

mpi_core_sources += src/mpid/ch4/gpu/pipeline.c
