# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

libhydra_la_SOURCES += $(top_srcdir)/tools/bootstrap/external/external_init.c \
	$(top_srcdir)/tools/bootstrap/external/external_launch.c \
	$(top_srcdir)/tools/bootstrap/external/external_finalize.c \
	$(top_srcdir)/tools/bootstrap/external/external_env.c \
	$(top_srcdir)/tools/bootstrap/external/ssh.c \
	$(top_srcdir)/tools/bootstrap/external/slurm_launch.c \
	$(top_srcdir)/tools/bootstrap/external/slurm_query_node_list.c \
	$(top_srcdir)/tools/bootstrap/external/slurm_query_proxy_id.c \
	$(top_srcdir)/tools/bootstrap/external/ll_launch.c \
	$(top_srcdir)/tools/bootstrap/external/ll_query_node_list.c \
	$(top_srcdir)/tools/bootstrap/external/ll_query_proxy_id.c \
	$(top_srcdir)/tools/bootstrap/external/lsf_query_node_list.c
