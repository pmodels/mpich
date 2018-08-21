## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/debugger

noinst_HEADERS += libhydra/debugger/hydra_debugger.h

libhydra_la_SOURCES += libhydra/debugger/hydra_debugger.c
