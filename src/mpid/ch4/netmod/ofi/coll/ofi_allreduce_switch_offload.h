/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OFI_ALLREDUCE_SWITCH_OFFLOAD_H_INCLUDED
#define OFI_ALLREDUCE_SWITCH_OFFLOAD_H_INCLUDED

#include "utarray.h"
#include <math.h>
#include "ofi_impl.h"
#include "ofi_coll_util.h"

/*
 * This function performs a switch-based allreduce
 * This is a blocking function and the arguments are same a in MPI_Allreduce();
 *
 * Input Parameters:
 * sendbuf - send buffer for Allreduce
 * recvbuf - receive buffer for Allreduce
 * count - number of elements in the buffer (non-negative integer)
 * datatype - datatype of each buffer element
 * op - the operation for Allreduce
 * comm_ptr - communicator
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Allreduce_intra_switch_offload(const void *sendbuf,
                                                                      void *recvbuf, int count,
                                                                      MPI_Datatype datatype,
                                                                      MPI_Op op,
                                                                      MPIR_Comm * comm_ptr,
                                                                      MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req_ptr;
    enum fi_datatype fi_dt;
    enum fi_op fi_op;

    req_ptr = MPIR_Request_create(MPIR_REQUEST_KIND__COLL);
    if (!req_ptr)
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIDI_OFI_REQUEST(req_ptr, event_id) = MPIDI_OFI_EVENT_COLL;
    MPIR_Request_add_ref(req_ptr);

    if (MPI_IN_PLACE == sendbuf)
        sendbuf = recvbuf;
    MPIDI_mpi_to_ofi(datatype, &fi_dt, op, &fi_op);
    MPIDI_OFI_CALL(fi_allreduce(MPIDI_OFI_global.ctx[0].ep,
                                sendbuf, count, NULL, recvbuf, NULL,
                                MPIDI_OFI_COMM(comm_ptr).offload_coll.coll_addr,
                                fi_dt, fi_op, 0,
                                (void *) &(MPIDI_OFI_REQUEST(req_ptr, context))), fi_allreduce);

    mpi_errno = MPIC_Wait(req_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Request_free(req_ptr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif
