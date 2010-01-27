# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/tools/debugger

libhydra_la_SOURCES += $(top_srcdir)/tools/debugger/debugger.c
