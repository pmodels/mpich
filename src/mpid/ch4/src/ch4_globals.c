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

#undef FUNCNAME
#define FUNCNAME MPID_Abort
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Abort(MPIR_Comm * comm, int mpi_errno, int exit_code, const char *error_msg)
{
    char sys_str[MPI_MAX_ERROR_STRING + 5] = "";
    char comm_str[MPI_MAX_ERROR_STRING] = "";
    char world_str[MPI_MAX_ERROR_STRING] = "";
    char error_str[2 * MPI_MAX_ERROR_STRING + 128];
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ABORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ABORT);

    if (MPIR_Process.comm_world) {
        int rank = MPIR_Process.comm_world->rank;
        snprintf(world_str, sizeof(world_str), " on node %d", rank);
    }

    if (comm) {
        int rank = comm->rank;
        int context_id = comm->context_id;
        snprintf(comm_str, sizeof(comm_str), " (rank %d in comm %d)", rank, context_id);
    }

    if (!error_msg)
        error_msg = "Internal error";

    if (mpi_errno != MPI_SUCCESS) {
        char msg[MPI_MAX_ERROR_STRING] = "";
        MPIR_Err_get_string(mpi_errno, msg, MPI_MAX_ERROR_STRING, NULL);
        snprintf(sys_str, sizeof(msg), " (%s)", msg);
    }
    MPL_snprintf(error_str, sizeof(error_str), "Abort(%d)%s%s: %s%s\n",
                 exit_code, world_str, comm_str, error_msg, sys_str);
    MPL_error_printf("%s", error_str);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ABORT);
    fflush(stderr);
    fflush(stdout);
    if (MPIR_Comm_size(comm) == 1)
        MPL_exit(exit_code);
#ifndef USE_PMIX_API
    PMI_Abort(exit_code, error_msg);
#endif
    return 0;
}

MPL_dbg_class MPIDI_CH4_DBG_GENERAL;

#ifdef MPL_USE_DBG_LOGGING
MPL_dbg_class MPIDI_CH4_DBG_GENERAL;
MPL_dbg_class MPIDI_CH4_DBG_MAP;
MPL_dbg_class MPIDI_CH4_DBG_COMM;
MPL_dbg_class MPIDI_CH4_DBG_MEMORY;
#endif
