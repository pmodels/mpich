/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OFI_IALLREDUCE_SWITCH_OFFLOAD_H_INCLUDED
#define OFI_IALLREDUCE_SWITCH_OFFLOAD_H_INCLUDED

#include <math.h>
#include "ofi_impl.h"

typedef struct {
    MPIR_Comm *comm_ptr;
    MPIR_Request *req;
    const void *sendbuf;
    void *recvbuf;
    int count;
    enum fi_datatype fi_dt;
    enum fi_op fi_op;
} OFI_allreduce_offload_t;

/* The following three functions provide the needed hook functions to
 * create a new gentran vertex type. The vertex actions defined here
 * include starting a collective offloading to switch, and waiting for
 * it to complete.
 */
MPL_STATIC_INLINE_PREFIX int iallreduce_issue(void *v, int *done)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req_ptr;
    OFI_allreduce_offload_t *allreduce_data = (OFI_allreduce_offload_t *) v;

    req_ptr = MPIR_Request_create(MPIR_REQUEST_KIND__COLL);
    if (!req_ptr)
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIDI_OFI_REQUEST(req_ptr, event_id) = MPIDI_OFI_EVENT_COLL;
    MPIR_Request_add_ref(req_ptr);
    allreduce_data->req = req_ptr;

    MPIDI_OFI_CALL(fi_allreduce(MPIDI_OFI_global.ctx[0].ep, allreduce_data->sendbuf,
                                allreduce_data->count, NULL,
                                allreduce_data->recvbuf, NULL,
                                MPIDI_OFI_COMM(allreduce_data->comm_ptr).offload_coll.coll_addr,
                                allreduce_data->fi_dt, allreduce_data->fi_op, 0,
                                (void *) &(MPIDI_OFI_REQUEST(req_ptr, context))), fi_allreduce);

    *done = 0;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int iallreduce_check_completions(void *v, int *done)
{
    int mpi_errno = MPI_SUCCESS;
    OFI_allreduce_offload_t *allreduce_data = (OFI_allreduce_offload_t *) v;

    if (MPIR_Request_is_complete(allreduce_data->req))
        *done = 1;
    else
        *done = 0;

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int iallreduce_free(void *v)
{
    int mpi_errno = MPI_SUCCESS;
    OFI_allreduce_offload_t *allreduce_data = (OFI_allreduce_offload_t *) v;

    MPIR_Request_free(allreduce_data->req);
    /* Free the struct */
    MPL_free(allreduce_data);

    return mpi_errno;
}

/*
 * This is a function to create schedule for performing non-blocking
 * switch-based Allreduce.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Iallreduce_sched_intra_switch_offload(const void *sendbuf,
                                                                             void *recvbuf,
                                                                             int count,
                                                                             MPI_Datatype
                                                                             datatype, MPI_Op op,
                                                                             MPIR_Comm *
                                                                             comm_ptr,
                                                                             MPIR_TSP_sched_t *
                                                                             sched)
{
    int mpi_errno = MPI_SUCCESS, ret ATTRIBUTE((unused));
    int vtx_id, type_id;
    OFI_allreduce_offload_t *allreduce_data;
    enum fi_datatype fi_dt;
    enum fi_op fi_op;

    /* Construct a generic vertex */
    type_id =
        MPIR_TSP_sched_new_type(sched, iallreduce_issue,
                                iallreduce_check_completions, iallreduce_free);

    allreduce_data =
        (OFI_allreduce_offload_t *) MPL_malloc(sizeof(OFI_allreduce_offload_t), MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDSTMT(allreduce_data == NULL, mpi_errno, MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
    allreduce_data->comm_ptr = comm_ptr;
    allreduce_data->sendbuf = sendbuf;
    allreduce_data->recvbuf = recvbuf;
    ret = MPIDI_mpi_to_ofi(datatype, &fi_dt, op, &fi_op);
    MPIR_Assert(ret != -1);
    allreduce_data->fi_dt = fi_dt;
    allreduce_data->fi_op = fi_op;
    allreduce_data->count = count;

    MPIR_TSP_sched_generic(type_id, allreduce_data, sched, 0, NULL, &vtx_id);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif
