
/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "mpidrma.h"

#undef FUNCNAME
#define FUNCNAME MPID_Rput
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Rput(const void *origin_addr, int origin_count,
              MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
              int target_count, MPI_Datatype target_datatype, MPID_Win * win_ptr,
              MPID_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPIDI_msg_sz_t data_sz;
    MPID_Request *ureq;
    MPIDI_STATE_DECL(MPID_STATE_MPID_RPUT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_RPUT);

    /* request-based RMA operations are only valid within a passive epoch */
    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, data_sz, dtp, dt_true_lb);

    /* Create user request, initially cc=1, ref=1 */
    ureq = MPID_Request_create();
    MPIR_ERR_CHKANDJUMP(ureq == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    ureq->kind = MPID_WIN_REQUEST;

    /* This request is referenced by user and ch3 by default. */
    MPIU_Object_set_ref(ureq, 2);

    /* Enqueue or perform the RMA operation */
    if (target_rank != MPI_PROC_NULL && data_sz != 0) {
        mpi_errno = MPIDI_CH3I_Put(origin_addr, origin_count,
                                   origin_datatype, target_rank,
                                   target_disp, target_count, target_datatype, win_ptr, ureq);

        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }
    else {
        mpi_errno = MPID_Request_complete(ureq);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

    *request = ureq;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_RPUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_Rget
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Rget(void *origin_addr, int origin_count,
              MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
              int target_count, MPI_Datatype target_datatype, MPID_Win * win_ptr,
              MPID_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPIDI_msg_sz_t data_sz;
    MPID_Request *ureq;
    MPIDI_STATE_DECL(MPID_STATE_MPID_RGET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_RGET);

    /* request-based RMA operations are only valid within a passive epoch */
    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, data_sz, dtp, dt_true_lb);

    /* Create user request, initially cc=1, ref=1 */
    ureq = MPID_Request_create();
    MPIR_ERR_CHKANDJUMP(ureq == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    ureq->kind = MPID_WIN_REQUEST;

    /* This request is referenced by user and ch3 by default. */
    MPIU_Object_set_ref(ureq, 2);

    /* Enqueue or perform the RMA operation */
    if (target_rank != MPI_PROC_NULL && data_sz != 0) {
        mpi_errno = MPIDI_CH3I_Get(origin_addr, origin_count,
                                   origin_datatype, target_rank,
                                   target_disp, target_count, target_datatype, win_ptr, ureq);

        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }
    else {
        mpi_errno = MPID_Request_complete(ureq);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

    *request = ureq;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_RGET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_Raccumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Raccumulate(const void *origin_addr, int origin_count,
                     MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                     int target_count, MPI_Datatype target_datatype, MPI_Op op, MPID_Win * win_ptr,
                     MPID_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPIDI_msg_sz_t data_sz;
    MPID_Request *ureq;
    MPIDI_STATE_DECL(MPID_STATE_MPID_RACCUMULATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_RACCUMULATE);

    /* request-based RMA operations are only valid within a passive epoch */
    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Create user request, initially cc=1, ref=1 */
    ureq = MPID_Request_create();
    MPIR_ERR_CHKANDJUMP(ureq == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    ureq->kind = MPID_WIN_REQUEST;

    /* This request is referenced by user and ch3 by default. */
    MPIU_Object_set_ref(ureq, 2);

    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, data_sz, dtp, dt_true_lb);

    /* Enqueue or perform the RMA operation */
    if (target_rank != MPI_PROC_NULL && data_sz != 0) {
        mpi_errno = MPIDI_CH3I_Accumulate(origin_addr, origin_count,
                                          origin_datatype, target_rank,
                                          target_disp, target_count,
                                          target_datatype, op, win_ptr, ureq);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }
    else {
        mpi_errno = MPID_Request_complete(ureq);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

    *request = ureq;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_RACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_Rget_accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Rget_accumulate(const void *origin_addr, int origin_count,
                         MPI_Datatype origin_datatype, void *result_addr, int result_count,
                         MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                         int target_count, MPI_Datatype target_datatype, MPI_Op op,
                         MPID_Win * win_ptr, MPID_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPIDI_msg_sz_t data_sz, trg_data_sz;
    MPID_Request *ureq;
    MPIDI_STATE_DECL(MPID_STATE_MPID_RGET_ACCUMULATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_RGET_ACCUMULATE);

    /* request-based RMA operations are only valid within a passive epoch */
    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Create user request, initially cc=1, ref=1 */
    ureq = MPID_Request_create();
    MPIR_ERR_CHKANDJUMP(ureq == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    ureq->kind = MPID_WIN_REQUEST;

    /* This request is referenced by user and ch3 by default. */
    MPIU_Object_set_ref(ureq, 2);

    /* Note that GACC is only a no-op if no data goes in both directions */
    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, data_sz, dtp, dt_true_lb);
    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, trg_data_sz, dtp, dt_true_lb);

    /* Enqueue or perform the RMA operation */
    if (target_rank != MPI_PROC_NULL && (data_sz != 0 || trg_data_sz != 0)) {
        mpi_errno = MPIDI_CH3I_Get_accumulate(origin_addr, origin_count,
                                              origin_datatype, result_addr,
                                              result_count, result_datatype,
                                              target_rank, target_disp,
                                              target_count, target_datatype, op, win_ptr, ureq);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }
    else {
        mpi_errno = MPID_Request_complete(ureq);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

    *request = ureq;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_RGET_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
