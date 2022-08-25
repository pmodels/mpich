/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidi_ch3_impl.h"
#include "mpidrma.h"

int MPID_Rput(const void *origin_addr, MPI_Aint origin_count,
              MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
              MPI_Aint target_count, MPI_Datatype target_datatype, MPIR_Win * win_ptr,
              MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig ATTRIBUTE((unused));
    MPIR_Datatype*dtp;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    intptr_t data_sz;
    MPIR_Request *ureq;

    MPIR_FUNC_ENTER;

    /* request-based RMA operations are only valid within a passive epoch */
    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, data_sz, dtp, dt_true_lb);

    /* Create user request, initially cc=1, ref=1 */
    ureq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
    MPIR_ERR_CHKANDJUMP(ureq == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    /* This request is referenced by user and ch3 by default. */
    MPIR_Object_set_ref(ureq, 2);

    /* Enqueue or perform the RMA operation */
    if (data_sz != 0) {
        mpi_errno = MPIDI_CH3I_Put(origin_addr, origin_count,
                                   origin_datatype, target_rank,
                                   target_disp, target_count, target_datatype, win_ptr, ureq);

        MPIR_ERR_CHECK(mpi_errno);
    }
    else {
        mpi_errno = MPID_Request_complete(ureq);
        MPIR_ERR_CHECK(mpi_errno);
    }

    *request = ureq;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


int MPID_Rget(void *origin_addr, MPI_Aint origin_count,
              MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
              MPI_Aint target_count, MPI_Datatype target_datatype, MPIR_Win * win_ptr,
              MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig ATTRIBUTE((unused));
    MPIR_Datatype*dtp;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    intptr_t data_sz;
    MPIR_Request *ureq;

    MPIR_FUNC_ENTER;

    /* request-based RMA operations are only valid within a passive epoch */
    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, data_sz, dtp, dt_true_lb);

    /* Create user request, initially cc=1, ref=1 */
    ureq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
    MPIR_ERR_CHKANDJUMP(ureq == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    /* This request is referenced by user and ch3 by default. */
    MPIR_Object_set_ref(ureq, 2);

    /* Enqueue or perform the RMA operation */
    if (data_sz != 0) {
        mpi_errno = MPIDI_CH3I_Get(origin_addr, origin_count,
                                   origin_datatype, target_rank,
                                   target_disp, target_count, target_datatype, win_ptr, ureq);

        MPIR_ERR_CHECK(mpi_errno);
    }
    else {
        mpi_errno = MPID_Request_complete(ureq);
        MPIR_ERR_CHECK(mpi_errno);
    }

    *request = ureq;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


int MPID_Raccumulate(const void *origin_addr, MPI_Aint origin_count,
                     MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                     MPI_Aint target_count, MPI_Datatype target_datatype, MPI_Op op, MPIR_Win * win_ptr,
                     MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig ATTRIBUTE((unused));
    MPIR_Datatype*dtp;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    intptr_t data_sz;
    MPIR_Request *ureq;

    MPIR_FUNC_ENTER;

    /* request-based RMA operations are only valid within a passive epoch */
    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Create user request, initially cc=1, ref=1 */
    ureq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
    MPIR_ERR_CHKANDJUMP(ureq == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    /* This request is referenced by user and ch3 by default. */
    MPIR_Object_set_ref(ureq, 2);

    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, data_sz, dtp, dt_true_lb);

    /* Enqueue or perform the RMA operation */
    if (data_sz != 0) {
        mpi_errno = MPIDI_CH3I_Accumulate(origin_addr, origin_count,
                                          origin_datatype, target_rank,
                                          target_disp, target_count,
                                          target_datatype, op, win_ptr, ureq);
        MPIR_ERR_CHECK(mpi_errno);
    }
    else {
        mpi_errno = MPID_Request_complete(ureq);
        MPIR_ERR_CHECK(mpi_errno);
    }

    *request = ureq;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


int MPID_Rget_accumulate(const void *origin_addr, MPI_Aint origin_count,
                         MPI_Datatype origin_datatype, void *result_addr, MPI_Aint result_count,
                         MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                         MPI_Aint target_count, MPI_Datatype target_datatype, MPI_Op op,
                         MPIR_Win * win_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig ATTRIBUTE((unused));
    MPIR_Datatype*dtp;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    intptr_t data_sz, trg_data_sz;
    MPIR_Request *ureq;

    MPIR_FUNC_ENTER;

    /* request-based RMA operations are only valid within a passive epoch */
    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Create user request, initially cc=1, ref=1 */
    ureq = MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
    MPIR_ERR_CHKANDJUMP(ureq == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    /* This request is referenced by user and ch3 by default. */
    MPIR_Object_set_ref(ureq, 2);

    /* Note that GACC is only a no-op if no data goes in both directions */
    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, data_sz, dtp, dt_true_lb);
    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, trg_data_sz, dtp, dt_true_lb);

    /* Enqueue or perform the RMA operation */
    if (data_sz != 0 || trg_data_sz != 0) {
        mpi_errno = MPIDI_CH3I_Get_accumulate(origin_addr, origin_count,
                                              origin_datatype, result_addr,
                                              result_count, result_datatype,
                                              target_rank, target_disp,
                                              target_count, target_datatype, op, win_ptr, ureq);
        MPIR_ERR_CHECK(mpi_errno);
    }
    else {
        mpi_errno = MPID_Request_complete(ureq);
        MPIR_ERR_CHECK(mpi_errno);
    }

    *request = ureq;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
