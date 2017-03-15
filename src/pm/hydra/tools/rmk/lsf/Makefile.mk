## -*- Mode: Makefile; -*-
##
## (C) 2009 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/tools/rmk/lsf

noinst_HEADERS += tools/rmk/lsf/lsf_rmk.h

libhydra_la_SOURCES += tools/rmk/lsf/lsf_detect.c \
		tools/rmk/lsf/lsf_query_node_list.c
