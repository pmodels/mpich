/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "common.h"

HYD_status HYDT_bscd_slurm_query_env_inherit(const char *env_name, int *ret)
{
    const char *env_list[] = { "SLURM_ACCOUNT", "SLURM_CPU_BIND",
        "SLURM_CPUS_PER_TASK", "SLURM_CONN_TYPE",
        "SLURM_CORE_FORMAT", "SLURM_DEBUG", "SLURMD_DEBUG",
        "SLURM_DISABLE_STATUS", "SLURM_DISTRIBUTION",
        "SLURM_GEOMETRY", "SLURM_LABELIO", "SLURM_MEM_BIND",
        "SLURM_NETWORK", "SLURM_NNODES", "SLURM_NO_ROTATE",
        "SLURM_NPROCS", "SLURM_OVERCOMMIT", "SLURM_PARTITION",
        "SLURM_REMOTE_CWD", "SLURM_SRUN_COMM_IFHN",
        "SLURM_STDERRMODE", "SLURM_STDINMODE",
        "SLURM_STDOUTMODE", "SLURM_TASK_EPILOG",
        "SLURM_TASK_PROLOG", "SLURM_TIMELIMIT", "SLURM_WAIT",
        "SLURM_CPU_BIND_VERBOSE", "SLURM_CPU_BIND_TYPE",
        "SLURM_CPU_BIND_LIST", "SLURM_CPUS_ON_NODE",
        "SLURM_JOBID", "SLURM_LAUNCH_NODE_IPADDR",
        "SLURM_LOCALID", "SLURM_MEM_BIND_VERBOSE",
        "SLURM_MEM_BIND_TYPE", "SLURM_MEM_BIND_LIST",
        "SLURM_NODEID", "SLURM_NODELIST", "SLURM_PROCID",
        "SLURM_TASKS_PER_NODE", "MAX_TASKS_PER_NODE",
        "SLURM_HOSTFILE", NULL
    };

    HYDU_FUNC_ENTER();

    *ret = !HYDTI_bscd_in_env_list(env_name, env_list);

    HYDU_FUNC_EXIT();

    return HYD_SUCCESS;
}
