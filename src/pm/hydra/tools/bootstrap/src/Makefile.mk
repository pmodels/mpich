# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

libhydra_la_SOURCES += $(top_builddir)/tools/bootstrap/src/bsci_init.c \
	$(top_srcdir)/tools/bootstrap/src/bsci_finalize.c \
	$(top_srcdir)/tools/bootstrap/src/bsci_launch.c \
	$(top_srcdir)/tools/bootstrap/src/bsci_query_node_list.c \
	$(top_srcdir)/tools/bootstrap/src/bsci_query_job_id.c \
	$(top_srcdir)/tools/bootstrap/src/bsci_query_proxy_id.c \
	$(top_srcdir)/tools/bootstrap/src/bsci_query_native_int.c \
	$(top_srcdir)/tools/bootstrap/src/bsci_wait.c \
	$(top_srcdir)/tools/bootstrap/src/bsci_env.c
