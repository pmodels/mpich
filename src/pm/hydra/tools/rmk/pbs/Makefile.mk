## -*- Mode: Makefile; -*-
##
## (C) 2009 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/tools/rmk/pbs

noinst_HEADERS += tools/rmk/pbs/pbs_rmk.h

libhydra_la_SOURCES += tools/rmk/pbs/pbs_detect.c \
		tools/rmk/pbs/pbs_query_node_list.c
