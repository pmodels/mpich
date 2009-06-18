# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

libhydra_a_SOURCES += $(top_srcdir)/bootstrap/slurm/slurm_init.c \
	$(top_srcdir)/bootstrap/slurm/slurm_launch.c \
	$(top_srcdir)/bootstrap/slurm/slurm_query_node_list.c \
	$(top_srcdir)/bootstrap/slurm/slurm_query_partition_id.c
