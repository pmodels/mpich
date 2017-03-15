## -*- Mode: Makefile; -*-
##
## (C) 2009 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/tools/rmk/sge

noinst_HEADERS += tools/rmk/sge/sge_rmk.h

libhydra_la_SOURCES += tools/rmk/sge/sge_detect.c \
		tools/rmk/sge/sge_query_node_list.c
