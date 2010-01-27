# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/tools/rmk/utils

libhydra_la_SOURCES += $(top_srcdir)/tools/rmk/utils/rmku_query_node_list.c
