/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* All global ADI data structures need to go in this file */
/* reference them with externs from other files           */

#include <mpidimpl.h>
#include "ch4_impl.h"

MPIDI_CH4_Global_t MPIDI_global;
MPIDI_av_table_t **MPIDI_av_table;
MPIDI_av_table_t *MPIDI_av_table0;

MPIDI_NM_funcs_t *MPIDI_NM_func;
MPIDI_NM_native_funcs_t *MPIDI_NM_native_func;

#if defined(MPIDI_CH4_USE_WORK_QUEUES)
struct MPIDI_workq_elemt MPIDI_workq_elemt_direct[MPIDI_WORKQ_ELEMT_PREALLOC];

MPIR_Object_alloc_t MPIDI_workq_elemt_mem = {
    0, 0, 0, 0, MPIR_INTERNAL, sizeof(struct MPIDI_workq_elemt), MPIDI_workq_elemt_direct,
    MPIDI_WORKQ_ELEMT_PREALLOC, NULL
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
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_dt ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_dt_ack ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_put_data ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_acc_dt ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_dt ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_acc_dt_ack ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_dt_ack ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_acc_data ATTRIBUTE((unused));
MPIR_T_pvar_timer_t PVAR_TIMER_rma_targetcb_get_acc_data ATTRIBUTE((unused));

#ifdef MPID_DEVICE_DEFINES_THREAD_CS
pthread_mutex_t MPIDI_Mutex_lock[MPIDI_NUM_LOCKS];
#endif

#ifdef HAVE_SIGNAL
void MPIDI_sigusr1_handler(int sig)
{
    MPIDI_global.sigusr1_count++;
    if (MPIDI_global.prev_sighandler)
        MPIDI_global.prev_sighandler(sig);
}
#endif

int MPID_Abort(MPIR_Comm * comm, int mpi_errno, int exit_code, const char *error_msg)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ABORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ABORT);

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

#ifdef HAVE_DEBUGGER_SUPPORT
    MPIR_Debugger_set_aborting(error_msg);
#endif
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ABORT);
    fflush(stderr);
    fflush(stdout);
    if (NULL == comm || (MPIR_Comm_size(comm) == 1 && comm->comm_kind == MPIR_COMM_KIND__INTRACOMM))
        MPL_exit(exit_code);

    if (comm != MPIR_Process.comm_world) {
        MPIDIG_am_comm_abort(comm, exit_code);
    } else {
        MPIR_pmi_abort(exit_code, error_msg);
    }
    return 0;
}

int MPIDI_check_for_failed_procs(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CHECK_FOR_FAILED_PROCS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CHECK_FOR_FAILED_PROCS);

    /* FIXME: Currently this only handles failed processes in
     * comm_world.  We need to fix hydra to include the pgid along
     * with the rank, then we need to create the failed group from
     * something bigger than comm_world. */

    char *failed_procs_string = MPIR_pmi_get_failed_procs();

    if (failed_procs_string) {
        MPL_free(failed_procs_string);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST,
                         "Received proc fail notification: %s", failed_procs_string));

        /* FIXME: handle ULFM failed groups here */
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CHECK_FOR_FAILED_PROCS);
    return mpi_errno;
}

MPL_dbg_class MPIDI_CH4_DBG_GENERAL;

#ifdef MPL_USE_DBG_LOGGING
MPL_dbg_class MPIDI_CH4_DBG_GENERAL;
MPL_dbg_class MPIDI_CH4_DBG_MAP;
MPL_dbg_class MPIDI_CH4_DBG_COMM;
MPL_dbg_class MPIDI_CH4_DBG_MEMORY;
#endif
