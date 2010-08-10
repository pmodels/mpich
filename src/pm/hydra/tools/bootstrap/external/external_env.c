/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "external.h"

HYD_status HYDT_bscd_external_query_env_inherit(const char *env_name, int *ret)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *ret = 1;

    if (!strcmp(HYDT_bsci_info.bootstrap, "ssh")) {
        const char *env_list[] = { "DISPLAY", NULL };

        for (i = 0; env_list[i]; i++) {
            if (!strcmp(env_name, env_list[i])) {
                *ret = 0;
                goto fn_exit;
            }
        }
    }

    if (!strcmp(HYDT_bsci_info.bootstrap, "lsf")) {
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
                                   NULL };

        for (i = 0; env_list[i]; i++) {
            if (!strcmp(env_name, env_list[i])) {
                *ret = 0;
                goto fn_exit;
            }
        }
    }

    if (!strcmp(HYDT_bsci_info.bootstrap, "sge")) {
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
                                   "SHELL", "TMPDIR", "TMP", "TZ", "USER", NULL };

        for (i = 0; env_list[i]; i++) {
            if (!strcmp(env_name, env_list[i])) {
                *ret = 0;
                goto fn_exit;
            }
        }
    }

    if (!strcmp(HYDT_bsci_info.bootstrap, "slurm")) {
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
                                   "SLURM_HOSTFILE", NULL };

        for (i = 0; env_list[i]; i++) {
            if (!strcmp(env_name, env_list[i])) {
                *ret = 0;
                goto fn_exit;
            }
        }
    }

    if (!strcmp(HYDT_bsci_info.bootstrap, "ll")) {
        const char *env_list[] = { "LOADL_STEP_CLASS", "LOADL_STEP_ARGS",
                                   "LOADL_STEP_ID", "LOADL_STARTD_PORT",
                                   "LOADL_STEP_NICE", "LOADL_STEP_IN", "LOADL_STEP_ERR",
                                   "LOADL_STEP_GROUP", "LOADL_STEP_NAME", "LOADL_STEP_ACCT",
                                   "LOADL_STEP_TYPE", "LOADL_STEP_OWNER", "LOADL_ACTIVE",
                                   "LOADL_STEP_COMMAND", "LOADL_JOB_NAME", "LOADL_STEP_OUT",
                                   "LOADL_STEP_INITDIR", "LOADL_PROCESSOR_LIST",
                                   "LOADLBATCH",

                                   "AIX_MINKTHREADS", "AIX_MNRATIO",
                                   "AIX_PTHREAD_SET_STACKSIZE", "AIXTHREAD_COND_DEBUG",
                                   "AIXTHREAD_MUTEX_DEBUG", "AIXTHREAD_RWLOCK_DEBUG",
                                   "AIXTHREAD_SCOPE", "AIXTHREAD_SLPRATIO", "MALLOCDEBUG",
                                   "MALLOCTYPE", "MALLOCMULTIHEAP", "MP_ADAPTER_USE",
                                   "MP_BUFFER_MEM", "MP_CHECKDIR", "MP_CHECKFILE",
                                   "MP_CLOCK_SOURCE", "MP_CMDFILE", "MP_COREDIR",
                                   "MP_COREFILE_FORMAT", "MP_COREFILE_SIGTERM", "MP_CPU_USE",
                                   "MP_CSS_INTERRUPT", "MP_DBXPROMPTMOD",
                                   "MP_DEBUG_INITIAL_STOP", "MP_DEBUG_LOG", "MP_EAGER_LIMIT",
                                   "MP_EUIDEVELOP", "MP_EUIDEVICE", "MP_EUILIB",
                                   "MP_EUILIBPATH", "MP_FENCE", "MP_HINTS_FILTERED",
                                   "MP_HOLD_STDIN", "MP_HOSTFILE", "MP_INFOLEVEL",
                                   "MP_INTRDELAY", "MP_IONODEFILE", "MP_LABELIO",
                                   "MP_LLFILE", "MP_MAX_TYPEDEPTH", "MP_MSG_API",
                                   "MP_NEWJOB", "MP_NOARGLIST", "MP_NODES", "MP_PGMMODEL",
                                   "MP_PMD_VERSION", "MP_PMDLOG", "MP_PMDSUFFIX",
                                   "MP_PMLIGHTS", "MP_POLLING_INTERVAL", "MP_PRIORITY",
                                   "MP_PROCS", "MP_PULSE", "MP_RESD", "MP_RETRY",
                                   "MP_RETRYCOUNT", "MP_RMPOOL", "MP_SAMPLEFREQ",
                                   "MP_SAVE_LLFILE", "MP_SAVEHOSTFILE", "MP_SHARED_MEMORY",
                                   "MP_SINGLE_THREAD", "MP_STDINMODE", "MP_STDOUTMODE",
                                   "MP_SYNC_ON_CONNECT", "MP_TASKS_PER_NODE", "MP_TBUFFSIZE",
                                   "MP_TBUFFWRAP", "MP_THREAD_STACKSIZE", "MP_TIMEOUT",
                                   "MP_TMPDIR", "MP_TRACEDIR", "MP_TRACELEVEL",
                                   "MP_TTEMPSIZE", "MP_USE_FLOW_CONTROL", "MP_USRPORT",
                                   "MP_WAIT_MODE", "PSALLOC", "RT_GRQ", "SPINLOOPTIME",
                                   "YIELDLOOPTIME", "XLSMPOPTS", NULL };

        for (i = 0; env_list[i]; i++) {
            if (!strcmp(env_name, env_list[i])) {
                *ret = 0;
                goto fn_exit;
            }
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
