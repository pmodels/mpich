/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "common.h"
#include "pbs.h"

HYD_status HYDT_bscd_pbs_query_env_inherit(const char *env_name, int *ret)
{
    const char *env_list[] = { "PBSCOREDUMP",
        "PBSDEBUG",
        "PBSLOGLEVEL",
        "PBS_ARRAYID",
        "PBS_CLIENTRETRY",
        "PBS_DEFAULT",
        "PBS_DPREFIX",
        "PBS_ENVIRONMENT",
        "PBS_GPUFILE",
        "PBS_JOBCOOKIE",
        "PBS_JOBID",
        "PBS_JOBNAME",
        "PBS_MOMPORT",
        "PBS_NODEFILE",
        "PBS_NODENUM",
        "PBS_NUM_NODES",
        "PBS_NUM_PPN",
        "PBS_O_HOME",
        "PBS_O_HOST",
        "PBS_O_JOBID",
        "PBS_O_LANG",
        "PBS_O_LOGNAME",
        "PBS_O_MAIL",
        "PBS_O_PATH",
        "PBS_O_QUEUE",
        "PBS_O_SHELL",
        "PBS_O_WORKDIR",
        "PBS_QSTAT_EXECONLY",
        "PBS_QUEUE",
        "PBS_SERVER",
        "PBS_TASKNUM",
        "PBS_VERSION",
        "PBS_VNODENUM",
        NULL
    };

    HYDU_FUNC_ENTER();

    *ret = !HYDTI_bscd_in_env_list(env_name, env_list);

    HYDU_FUNC_EXIT();

    return HYD_SUCCESS;
}
