# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

libhydra_a_SOURCES += $(top_builddir)/bootstrap/src/bsci_init.c \
	$(top_srcdir)/bootstrap/src/bsci_finalize.c \
	$(top_srcdir)/bootstrap/src/bsci_launch.c \
	$(top_srcdir)/bootstrap/src/bsci_query_node_list.c \
	$(top_srcdir)/bootstrap/src/bsci_query_proxy_id.c \
	$(top_srcdir)/bootstrap/src/bsci_usize.c \
	$(top_srcdir)/bootstrap/src/bsci_wait.c
