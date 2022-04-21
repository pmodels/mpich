/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OFI_IBCAST_SWITCH_OFFLOAD_H_INCLUDED
#define OFI_IBCAST_SWITCH_OFFLOAD_H_INCLUDED

#include <math.h>
#include "ofi_impl.h"

typedef struct {
    MPIR_Comm *comm_ptr;
    MPIR_Request *req;
    void *buffer;
    enum fi_datatype fi_dt;
    int count;
    int root;
} OFI_bcast_offload_t;

MPL_STATIC_INLINE_PREFIX int ibcast_issue(void *v, int *done)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req_ptr;
    OFI_bcast_offload_t *bcast_data = (OFI_bcast_offload_t *) v;

    req_ptr = MPIR_Request_create(MPIR_REQUEST_KIND__COLL);
    if (!req_ptr)
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIDI_OFI_REQUEST(req_ptr, event_id) = MPIDI_OFI_EVENT_COLL;
    MPIR_Request_add_ref(req_ptr);
    bcast_data->req = req_ptr;

    MPIDI_OFI_CALL(fi_broadcast(MPIDI_OFI_global.ctx[0].ep, bcast_data->buffer,
                                bcast_data->count, NULL,
                                MPIDI_OFI_COMM(bcast_data->comm_ptr).offload_coll.coll_addr,
                                bcast_data->root, bcast_data->fi_dt, 0,
                                (void *) &(MPIDI_OFI_REQUEST(req_ptr, context))), fi_broadcast);

    *done = 0;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int ibcast_check_completions(void *v, int *done)
{
    int mpi_errno = MPI_SUCCESS;
    OFI_bcast_offload_t *bcast_data = (OFI_bcast_offload_t *) v;

    if (MPIR_Request_is_complete(bcast_data->req))
        *done = 1;
    else
        *done = 0;

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int ibcast_free(void *v)
{
    int mpi_errno = MPI_SUCCESS;
    OFI_bcast_offload_t *bcast_data = (OFI_bcast_offload_t *) v;

    MPIR_Request_free(bcast_data->req);
    /* Free the struct */
    MPL_free(bcast_data);

    return mpi_errno;
}

/*
 * This is a function to create schedule for performing non-blocking
 * switch-based Bcast.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Ibcast_sched_intra_switch_offload(void *buffer, int count,
                                                                         MPI_Datatype datatype,
                                                                         int root,
                                                                         MPIR_Comm * comm_ptr,
                                                                         MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS, ret ATTRIBUTE((unused));
    int vtx_id, type_id;
    OFI_bcast_offload_t *bcast_data;
    enum fi_datatype fi_dt;

    /* Construct a generic vertex */
    type_id = MPIR_TSP_sched_new_type(sched, ibcast_issue, ibcast_check_completions, ibcast_free);

    bcast_data = (OFI_bcast_offload_t *) MPL_malloc(sizeof(OFI_bcast_offload_t), MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDSTMT(bcast_data == NULL, mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
    bcast_data->comm_ptr = comm_ptr;
    bcast_data->buffer = buffer;
    ret = MPIDI_mpi_to_ofi(datatype, &fi_dt, 0, NULL);
    MPIR_Assert(ret != -1);
    bcast_data->fi_dt = fi_dt;
    bcast_data->count = count;
    bcast_data->root = root;

    MPIR_TSP_sched_generic(type_id, bcast_data, sched, 0, NULL, &vtx_id);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif
