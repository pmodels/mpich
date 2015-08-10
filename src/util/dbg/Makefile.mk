## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/util/dbg

noinst_HEADERS += src/util/dbg/mpidbg.h

mpi_core_sources += \
    src/util/dbg/dbg_printf.c     \
    src/util/dbg/timelimit.c
