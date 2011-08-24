/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "common.h"

HYD_status HYDT_bscd_lsf_query_env_inherit(const char *env_name, int *ret)
{
    const char *env_list[] = { "DISPLAY", "EGO_SERVERDIR", "LSB_TRAPSIGS",
        "LSF_SERVERDIR", "HOSTTYPE", "LSB_HOSTS",
        "LSF_BINDIR", "EGO_BINDIR", "PWD", "HOME",
        "LSB_ERRORFILE", "LSB_OUTPUTFILE", "TMPDIR",
        "LSF_LIBDIR", "EGO_LIBDIR", "LSB_MCPU_HOSTS",
        "PATH", "LD_LIBRARY_PATH", "LSB_JOBRES_PID",
        "LSB_EEXEC_REAL_UID", "LS_EXEC_T", "LSB_INTERACTIVE",
        "LSB_CHKFILENAME", "SPOOLDIR", "LSB_ACCT_FILE",
        "LSB_EEXEC_REAL_GID", "LSB_CHKPNT_DIR",
        "LSB_CHKPNT_PERIOD", "LSB_JOB_STARTER",
        "LSB_EXIT_REQUEUE", "LSB_DJOB_RU_INTERVAL",
        "LSB_DJOB_HB_INTERVAL", "LSB_DJOB_HOSTFILE",
        "LSB_JOBEXIT_INFO", "LSB_JOBPEND", "LSB_EXECHOSTS",
        "LSB_JOBID", "LSB_JOBINDEX", "LSB_JOBINDEX_STEP",
        "LSB_JOBINDEX_END", "LSB_JOBPID", "LSB_JOBNAME",
        "LSB_JOBFILENAME", "LSB_TASKID", "LSB_TASKINDEX",
        NULL
    };

    HYDU_FUNC_ENTER();

    *ret = !HYDTI_bscd_in_env_list(env_name, env_list);

    HYDU_FUNC_EXIT();

    return HYD_SUCCESS;
}
