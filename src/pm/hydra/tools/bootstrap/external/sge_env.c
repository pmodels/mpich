/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "common.h"

HYD_status HYDT_bscd_sge_query_env_inherit(const char *env_name, int *ret)
{
    const char *env_list[] = { "DISPLAY", "SGE_ROOT", "SGE_CELL", "SGE_DEBUG_LEVEL",
        "SGE_QMASTER_PORT", "SGE_O_HOME", "SGE_O_HOST",
        "SGE_O_LOGNAME", "SGE_O_MAIL", "SGE_O_PATH",
        "SGE_O_SHELL", "SGE_O_TZ", "SGE_O_WORKDIR",
        "SGE_ARCH", "SGE_CKPT_ENV", "SGE_CKPT_DIR",
        "SGE_STDERR_PATH", "SGE_STDOUT_PATH", "SGE_STDIN_PATH",
        "SGE_JOB_SPOOL_DIR", "SGE_TASK_ID", "SGE_TASK_FIRST",
        "SGE_TASK_LAST", "SGE_TASK_STEPSIZE", "SGE_BINARY_PATH",
        "SGE_JSV_TIMEOUT", "SGE_BINDING", "ARC", "ENVIRONMENT",
        "HOME", "HOSTNAME", "JOB_ID", "JOB_NAME", "JOB_SCRIPT",
        "LOGNAME", "NHOSTS", "NQUEUES", "NSLOTS", "PATH",
        "PE", "PE_HOSTFILE", "QUEUE", "REQUEST", "RESTARTED",
        "SHELL", "TMPDIR", "TMP", "TZ", "USER", NULL
    };

    HYDU_FUNC_ENTER();

    *ret = !HYDTI_bscd_in_env_list(env_name, env_list);

    HYDU_FUNC_EXIT();

    return HYD_SUCCESS;
}
