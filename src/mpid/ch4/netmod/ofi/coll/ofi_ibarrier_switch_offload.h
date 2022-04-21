/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OFI_IBARRIER_H_INCLUDED
#define OFI_IBARRIER_H_INCLUDED

#include <math.h>
#include "ofi_impl.h"

typedef struct {
    MPIR_Comm *comm_ptr;;
    MPIR_Request *req;
} OFI_barrier_offload_t;

MPL_STATIC_INLINE_PREFIX int ibarrier_issue(void *v, int *done)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req_ptr;
    OFI_barrier_offload_t *barrier_data = (OFI_barrier_offload_t *) v;

    req_ptr = MPIR_Request_create(MPIR_REQUEST_KIND__COLL);
    if (!req_ptr)
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIDI_OFI_REQUEST(req_ptr, event_id) = MPIDI_OFI_EVENT_COLL;
    MPIR_Request_add_ref(req_ptr);
    barrier_data->req = req_ptr;

    MPIDI_OFI_CALL(fi_barrier(MPIDI_OFI_global.ctx[0].ep,
                              MPIDI_OFI_COMM(barrier_data->comm_ptr).offload_coll.coll_addr,
                              (void *) &(MPIDI_OFI_REQUEST(req_ptr, context))), fi_barrier);

    *done = 0;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int ibarrier_check_completions(void *v, int *done)
{
    int mpi_errno = MPI_SUCCESS;
    OFI_barrier_offload_t *barrier_data = (OFI_barrier_offload_t *) v;

    if (MPIR_Request_is_complete(barrier_data->req))
        *done = 1;
    else
        *done = 0;

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int ibarrier_free(void *v)
{
    int mpi_errno = MPI_SUCCESS;
    OFI_barrier_offload_t *barrier_data = (OFI_barrier_offload_t *) v;

    MPIR_Request_free(barrier_data->req);
    /* Free the struct */
    MPL_free(barrier_data);

    return mpi_errno;
}

/*
 * This is a function to create schedule for performing non-blocking
 * switch-based Barrier.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Ibarrier_sched_intra_switch_offload(MPIR_Comm * comm_ptr,
                                                                           MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int vtx_id, type_id;
    OFI_barrier_offload_t *barrier_data;

    /* Construct a generic vertex */
    type_id =
        MPIR_TSP_sched_new_type(sched, ibarrier_issue, ibarrier_check_completions, ibarrier_free);

    barrier_data =
        (OFI_barrier_offload_t *) MPL_malloc(sizeof(OFI_barrier_offload_t), MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDSTMT(barrier_data == NULL, mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
    barrier_data->comm_ptr = comm_ptr;

    MPIR_TSP_sched_generic(type_id, barrier_data, sched, 0, NULL, &vtx_id);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;

}

#endif
