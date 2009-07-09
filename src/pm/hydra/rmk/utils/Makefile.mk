# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/rmk/utils

libhydra_a_SOURCES += $(top_srcdir)/rmk/utils/rmku_query_node_list.c
