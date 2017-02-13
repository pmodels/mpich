## -*- Mode: Makefile; -*-
##
## (C) 2009 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/tools/rmk/src

noinst_HEADERS += tools/rmk/src/rmk.h

libhydra_la_SOURCES += tools/rmk/src/rmk.c
