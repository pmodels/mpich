## -*- Mode: Makefile; -*-
##
## (C) 2009 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/tools/rmk/cobalt

noinst_HEADERS += tools/rmk/cobalt/cobalt_rmk.h

libhydra_la_SOURCES += tools/rmk/cobalt/cobalt_detect.c \
		tools/rmk/cobalt/cobalt_query_node_list.c
