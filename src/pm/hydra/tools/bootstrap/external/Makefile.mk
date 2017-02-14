## -*- Mode: Makefile; -*-
##
## (C) 2010 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

noinst_HEADERS +=                     \
    tools/bootstrap/external/common.h \
    tools/bootstrap/external/ll.h     \
    tools/bootstrap/external/lsf.h    \
    tools/bootstrap/external/pbs.h    \
    tools/bootstrap/external/rsh.h    \
    tools/bootstrap/external/sge.h    \
    tools/bootstrap/external/slurm.h  \
    tools/bootstrap/external/ssh.h

libhydra_la_SOURCES += tools/bootstrap/external/external_common.c \
	tools/bootstrap/external/external_common_launch.c \
	tools/bootstrap/external/fork_init.c \
	tools/bootstrap/external/rsh_init.c \
	tools/bootstrap/external/rsh_env.c \
	tools/bootstrap/external/ssh_init.c \
	tools/bootstrap/external/ssh.c \
	tools/bootstrap/external/ssh_env.c \
	tools/bootstrap/external/ssh_finalize.c \
	tools/bootstrap/external/slurm_init.c \
	tools/bootstrap/external/slurm_launch.c \
	tools/bootstrap/external/slurm_env.c \
	tools/bootstrap/external/slurm_query_proxy_id.c \
	tools/bootstrap/external/ll_init.c \
	tools/bootstrap/external/ll_launch.c \
	tools/bootstrap/external/ll_query_proxy_id.c \
	tools/bootstrap/external/ll_env.c \
	tools/bootstrap/external/lsf_init.c \
	tools/bootstrap/external/lsf_env.c \
	tools/bootstrap/external/sge_init.c \
	tools/bootstrap/external/sge_env.c \
	tools/bootstrap/external/pbs_init.c

if hydra_pbs_launcher
libhydra_la_SOURCES += \
	tools/bootstrap/external/pbs_finalize.c \
	tools/bootstrap/external/pbs_launch.c \
	tools/bootstrap/external/pbs_wait.c \
	tools/bootstrap/external/pbs_env.c
endif
