/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* All global ADI data structures need to go in this file */
/* reference them with externs from other files           */

#include <mpidimpl.h>
#include "ch4_impl.h"

MPIDI_CH4_Global_t MPIDI_CH4_Global;
MPIDI_av_table_t **MPIDI_av_table;
MPIDI_av_table_t *MPIDI_av_table0;

MPIDI_NM_funcs_t *MPIDI_NM_func;
MPIDI_NM_native_funcs_t *MPIDI_NM_native_func;

#if defined(MPIDI_CH4_USE_WORK_QUEUES)
struct MPIDI_workq_elemt MPIDI_workq_elemt_direct[MPIDI_WORKQ_ELEMT_PREALLOC] = { {0}
};

MPIR_Object_alloc_t MPIDI_workq_elemt_mem = {
    0, 0, 0, 0, MPIR_WORKQ_ELEM, sizeof(struct MPIDI_workq_elemt), MPIDI_workq_elemt_direct,
    MPIDI_WORKQ_ELEMT_PREALLOC
};
#endif /* #if defined(MPIDI_CH4_USE_WORK_QUEUES) */

unsigned PVAR_LEVEL_posted_recvq_length ATTRIBUTE((unused));
unsigned PVAR_LEVEL_unexpected_recvq_length ATTRIBUTE((unused));
unsigned long long PVAR_COUNTER_posted_recvq_match_attempts ATTRIBUTE((unused));
unsigned long long PVAR_COUNTER_unexpected_recvq_match_attempts ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_time_failed_matching_postedq ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_time_matching_unexpectedq ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_winlock_getlocallock ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_wincreate_allgather ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_amhdr_set ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_ack ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_ack ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_cas ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_cas_ack ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_acc ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_acc_ack ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_ack ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_win_ctrl ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_iov ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_iov_ack ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_data ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_acc_iov ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_iov ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_acc_iov_ack ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_iov_ack ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_acc_data ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_data ATTRIBUTE((unused));

#ifdef MPID_DEVICE_DEFINES_THREAD_CS
pthread_mutex_t MPIDI_Mutex_lock[MPIDI_NUM_LOCKS];
#endif

#ifdef HAVE_SIGNAL
void MPIDI_sigusr1_handler(int sig)
{
    MPIDI_CH4_Global.sigusr1_count++;
    if (MPIDI_CH4_Global.prev_sighandler)
        MPIDI_CH4_Global.prev_sighandler(sig);
}
#endif

#undef FUNCNAME
#define FUNCNAME MPID_Abort
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Abort(MPIR_Comm * comm, int mpi_errno, int exit_code, const char *error_msg)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ABORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ABORT);

    char world_str[MPI_MAX_ERROR_STRING] = "";
    if (MPIR_Process.comm_world) {
        int rank = MPIR_Process.comm_world->rank;
        snprintf(world_str, sizeof(world_str), " on node %d", rank);
    }

    char comm_str[MPI_MAX_ERROR_STRING] = "";
    if (comm) {
        int rank = comm->rank;
        int context_id = comm->context_id;
        snprintf(comm_str, sizeof(comm_str), " (rank %d in comm %d)", rank, context_id);
    }

    if (!error_msg)
        error_msg = "Internal error";

    char sys_str[MPI_MAX_ERROR_STRING + 5] = "";
    if (mpi_errno != MPI_SUCCESS) {
        char msg[MPI_MAX_ERROR_STRING] = "";
        MPIR_Err_get_string(mpi_errno, msg, MPI_MAX_ERROR_STRING, NULL);
        snprintf(sys_str, sizeof(sys_str), " (%s)", msg);
    }

    char error_str[3 * MPI_MAX_ERROR_STRING + 128];
    MPL_snprintf(error_str, sizeof(error_str), "Abort(%d)%s%s: %s%s\n",
                 exit_code, world_str, comm_str, error_msg, sys_str);
    MPL_error_printf("%s", error_str);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ABORT);
    fflush(stderr);
    fflush(stdout);
    if (NULL == comm || (MPIR_Comm_size(comm) == 1 && comm->comm_kind == MPIR_COMM_KIND__INTRACOMM))
        MPL_exit(exit_code);

    if (comm != MPIR_Process.comm_world) {
        MPIDIG_comm_abort(comm, exit_code);
    } else {
#ifdef USE_PMIX_API
        PMIx_Abort(exit_code, error_msg, NULL, 0);
#elif defined(USE_PMI2_API)
        PMI2_Abort(TRUE, error_msg);
#else
        PMI_Abort(exit_code, error_msg);
#endif
    }
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_check_for_failed_procs
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_check_for_failed_procs(void)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int len;
    char *kvsname = MPIDI_CH4_Global.jobid;
    char *failed_procs_string = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CHECK_FOR_FAILED_PROCS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CHECK_FOR_FAILED_PROCS);

    /* FIXME: Currently this only handles failed processes in
     * comm_world.  We need to fix hydra to include the pgid along
     * with the rank, then we need to create the failed group from
     * something bigger than comm_world. */
#ifdef USE_PMIX_API
    MPIR_Assert(0);
#elif defined(USE_PMI2_API)
    {
        int vallen = 0;
        len = PMI2_MAX_VALLEN;
        failed_procs_string = MPL_malloc(len, MPL_MEM_OTHER);
        MPIR_Assert(failed_procs_string);
        pmi_errno =
            PMI2_KVS_Get(kvsname, PMI2_ID_NULL, "PMI_dead_processes", failed_procs_string,
                         len, &vallen);
        MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get");
        MPL_free(failed_procs_string);
    }
#else
    pmi_errno = PMI_KVS_Get_value_length_max(&len);
    MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_value_length_max");
    failed_procs_string = MPL_malloc(len, MPL_MEM_OTHER);
    MPIR_Assert(failed_procs_string);
    pmi_errno = PMI_KVS_Get(kvsname, "PMI_dead_processes", failed_procs_string, len);
    MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get");
    MPL_free(failed_procs_string);
#endif

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "Received proc fail notification: %s", failed_procs_string));

    /* FIXME: handle ULFM failed groups here */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CHECK_FOR_FAILED_PROCS);
    return mpi_errno;
  fn_fail:
    MPL_free(failed_procs_string);
    goto fn_exit;
}

MPL_dbg_class MPIDI_CH4_DBG_GENERAL;

#ifdef MPL_USE_DBG_LOGGING
MPL_dbg_class MPIDI_CH4_DBG_GENERAL;
MPL_dbg_class MPIDI_CH4_DBG_MAP;
MPL_dbg_class MPIDI_CH4_DBG_COMM;
MPL_dbg_class MPIDI_CH4_DBG_MEMORY;
#endif
