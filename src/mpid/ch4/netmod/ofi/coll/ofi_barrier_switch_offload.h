/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OFI_BARRIER_SWITCH_OFFLOAD_H_INCLUDED
#define OFI_BARRIER_SWITCH_OFFLOAD_H_INCLUDED

#include "ofi_triggered.h"
#include "utarray.h"
#include <math.h>
#include "ofi_impl.h"
#include "ofi_coll_util.h"
/*@
 * This function offload barrier to libfabric (switch-based collective)
 *
 * Input Parameters:
 * comm_ptr - communicator
 * This is a blocking barrier function.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Barrier_intra_switch_offloading(MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Request *req_ptr;

    req_ptr = MPIR_Request_create(MPIR_REQUEST_KIND__COLL);
    if (!req_ptr)
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIDI_OFI_REQUEST(req_ptr, event_id) = MPIDI_OFI_EVENT_COLL;
    MPIR_Request_add_ref(req_ptr);

    MPIDI_OFI_CALL(fi_barrier(MPIDI_OFI_global.ctx[0].ep,
                              MPIDI_OFI_COMM(comm_ptr).offload_coll.coll_addr,
                              (void *) &(MPIDI_OFI_REQUEST(req_ptr, context))), fi_barrier);

    mpi_errno = MPIC_Wait(req_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Request_free(req_ptr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif
