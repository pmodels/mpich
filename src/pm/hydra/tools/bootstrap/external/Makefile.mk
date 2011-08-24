# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

libhydra_la_SOURCES += $(top_srcdir)/tools/bootstrap/external/external_common.c \
	$(top_srcdir)/tools/bootstrap/external/external_common_launch.c \
	$(top_srcdir)/tools/bootstrap/external/fork_init.c \
	$(top_srcdir)/tools/bootstrap/external/user_init.c \
	$(top_srcdir)/tools/bootstrap/external/manual_init.c \
	$(top_srcdir)/tools/bootstrap/external/rsh_init.c \
	$(top_srcdir)/tools/bootstrap/external/rsh_env.c \
	$(top_srcdir)/tools/bootstrap/external/ssh_init.c \
	$(top_srcdir)/tools/bootstrap/external/ssh.c \
	$(top_srcdir)/tools/bootstrap/external/ssh_env.c \
	$(top_srcdir)/tools/bootstrap/external/ssh_finalize.c \
	$(top_srcdir)/tools/bootstrap/external/slurm_init.c \
	$(top_srcdir)/tools/bootstrap/external/slurm_launch.c \
	$(top_srcdir)/tools/bootstrap/external/slurm_env.c \
	$(top_srcdir)/tools/bootstrap/external/slurm_query_native_int.c \
	$(top_srcdir)/tools/bootstrap/external/slurm_query_node_list.c \
	$(top_srcdir)/tools/bootstrap/external/slurm_query_proxy_id.c \
	$(top_srcdir)/tools/bootstrap/external/slurm_query_jobid.c \
	$(top_srcdir)/tools/bootstrap/external/ll_init.c \
	$(top_srcdir)/tools/bootstrap/external/ll_launch.c \
	$(top_srcdir)/tools/bootstrap/external/ll_query_native_int.c \
	$(top_srcdir)/tools/bootstrap/external/ll_query_node_list.c \
	$(top_srcdir)/tools/bootstrap/external/ll_query_proxy_id.c \
	$(top_srcdir)/tools/bootstrap/external/ll_env.c \
	$(top_srcdir)/tools/bootstrap/external/lsf_init.c \
	$(top_srcdir)/tools/bootstrap/external/lsf_query_native_int.c \
	$(top_srcdir)/tools/bootstrap/external/lsf_query_node_list.c \
	$(top_srcdir)/tools/bootstrap/external/lsf_env.c \
	$(top_srcdir)/tools/bootstrap/external/sge_init.c \
	$(top_srcdir)/tools/bootstrap/external/sge_query_native_int.c \
	$(top_srcdir)/tools/bootstrap/external/sge_query_node_list.c \
	$(top_srcdir)/tools/bootstrap/external/sge_env.c \
	$(top_srcdir)/tools/bootstrap/external/pbs_init.c \
	$(top_srcdir)/tools/bootstrap/external/pbs_query_native_int.c \
	$(top_srcdir)/tools/bootstrap/external/pbs_query_node_list.c \
	$(top_srcdir)/tools/bootstrap/external/pbs_query_jobid.c

if hydra_pbs_launcher
libhydra_la_SOURCES += \
	$(top_srcdir)/tools/bootstrap/external/pbs_finalize.c \
	$(top_srcdir)/tools/bootstrap/external/pbs_launch.c \
	$(top_srcdir)/tools/bootstrap/external/pbs_wait.c \
	$(top_srcdir)/tools/bootstrap/external/pbs_env.c
endif
