# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/tools/bootstrap/utils

libhydra_a_SOURCES += $(top_srcdir)/tools/bootstrap/utils/bscu_finalize.c \
	$(top_srcdir)/tools/bootstrap/utils/bscu_query_node_list.c \
	$(top_srcdir)/tools/bootstrap/utils/bscu_query_proxy_id.c \
	$(top_srcdir)/tools/bootstrap/utils/bscu_usize.c \
	$(top_srcdir)/tools/bootstrap/utils/bscu_wait.c
