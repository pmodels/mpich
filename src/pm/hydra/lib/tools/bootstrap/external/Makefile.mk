##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

noinst_HEADERS +=                     \
    lib/tools/bootstrap/external/common.h \
    lib/tools/bootstrap/external/ll.h     \
    lib/tools/bootstrap/external/lsf.h    \
    lib/tools/bootstrap/external/pbs.h    \
    lib/tools/bootstrap/external/rsh.h    \
    lib/tools/bootstrap/external/sge.h    \
    lib/tools/bootstrap/external/slurm.h  \
    lib/tools/bootstrap/external/cobalt.h \
    lib/tools/bootstrap/external/ssh.h

libhydra_la_SOURCES += lib/tools/bootstrap/external/external_common.c \
    lib/tools/bootstrap/external/external_common_launch.c \
    lib/tools/bootstrap/external/fork_init.c \
    lib/tools/bootstrap/external/user_init.c \
    lib/tools/bootstrap/external/manual_init.c \
    lib/tools/bootstrap/external/rsh_init.c \
    lib/tools/bootstrap/external/rsh_env.c \
    lib/tools/bootstrap/external/ssh_init.c \
    lib/tools/bootstrap/external/ssh.c \
    lib/tools/bootstrap/external/ssh_env.c \
    lib/tools/bootstrap/external/ssh_finalize.c \
    lib/tools/bootstrap/external/slurm_init.c \
    lib/tools/bootstrap/external/slurm_launch.c \
    lib/tools/bootstrap/external/slurm_env.c \
    lib/tools/bootstrap/external/slurm_query_native_int.c \
    lib/tools/bootstrap/external/slurm_query_node_list.c \
    lib/tools/bootstrap/external/slurm_query_proxy_id.c \
    lib/tools/bootstrap/external/ll_init.c \
    lib/tools/bootstrap/external/ll_launch.c \
    lib/tools/bootstrap/external/ll_query_native_int.c \
    lib/tools/bootstrap/external/ll_query_node_list.c \
    lib/tools/bootstrap/external/ll_query_proxy_id.c \
    lib/tools/bootstrap/external/ll_env.c \
    lib/tools/bootstrap/external/lsf_init.c \
    lib/tools/bootstrap/external/lsf_query_native_int.c \
    lib/tools/bootstrap/external/lsf_query_node_list.c \
    lib/tools/bootstrap/external/lsf_env.c \
    lib/tools/bootstrap/external/sge_init.c \
    lib/tools/bootstrap/external/sge_query_native_int.c \
    lib/tools/bootstrap/external/sge_query_node_list.c \
    lib/tools/bootstrap/external/sge_env.c \
    lib/tools/bootstrap/external/pbs_init.c \
    lib/tools/bootstrap/external/pbs_query_native_int.c \
    lib/tools/bootstrap/external/pbs_query_node_list.c \
    lib/tools/bootstrap/external/cobalt_init.c \
    lib/tools/bootstrap/external/cobalt_query_native_int.c \
    lib/tools/bootstrap/external/cobalt_query_node_list.c

if hydra_pbs_launcher
libhydra_la_SOURCES += \
    lib/tools/bootstrap/external/pbs_finalize.c \
    lib/tools/bootstrap/external/pbs_launch.c \
    lib/tools/bootstrap/external/pbs_wait.c \
    lib/tools/bootstrap/external/pbs_env.c
endif
