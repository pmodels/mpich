## -*- Mode: Makefile; -*-
##
## (C) 2008 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/tools/debugger

noinst_HEADERS += tools/debugger/debugger.h

libhydra_la_SOURCES += tools/debugger/debugger.c
