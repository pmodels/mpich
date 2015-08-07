## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_builddir)/src/util/timers
nodist_noinst_HEADERS += src/util/timers/mpiu_timer.h
mpi_core_sources += src/util/timers/mpiu_timer.c
