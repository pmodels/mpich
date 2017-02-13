## -*- Mode: Makefile; -*-
##
## (C) 2009 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/tools/rmk/ll

noinst_HEADERS += tools/rmk/ll/ll_rmk.h

libhydra_la_SOURCES += tools/rmk/ll/ll_detect.c \
		tools/rmk/ll/ll_query_node_list.c
