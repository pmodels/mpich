# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

libhydra_a_SOURCES += $(top_srcdir)/tools/bootstrap/poe/poe_init.c \
	$(top_srcdir)/tools/bootstrap/poe/poe_launch.c \
	$(top_srcdir)/tools/bootstrap/poe/poe_query_node_list.c \
	$(top_srcdir)/tools/bootstrap/poe/poe_query_proxy_id.c
