/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OFI_BCAST_SWITCH_OFFLOAD_H_INCLUDED
#define OFI_BCAST_SWITCH_OFFLOAD_H_INCLUDED

#include "ofi_triggered.h"
#include "utarray.h"
#include <math.h>
#include "ofi_impl.h"
#include "ofi_coll_util.h"
/*@
 * This function offload bcast to libfabric (switch-based collective)
 *
 * Input Parameters:
 * buffer - buffer for bcast
 * count - number of elements in the buffer (non-negative integer)
 * datatype - datatype of each buffer element
 * root - the root node of bcast
 * comm_ptr - communicator
 * This is a blocking bcast function.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Bcast_intra_switch_offload(void *buffer, int count,
                                                                  MPI_Datatype datatype, int root,
                                                                  MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Request *req_ptr;
    enum fi_datatype fi_dt;

    MPIDI_mpi_to_ofi(datatype, &fi_dt, 0, NULL);

    req_ptr = MPIR_Request_create(MPIR_REQUEST_KIND__COLL);
    if (!req_ptr)
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIDI_OFI_REQUEST(req_ptr, event_id) = MPIDI_OFI_EVENT_COLL;
    MPIR_Request_add_ref(req_ptr);

    MPIDI_OFI_CALL(fi_broadcast(MPIDI_OFI_global.ctx[0].ep, buffer,
                                count, NULL,
                                MPIDI_OFI_COMM(comm_ptr).offload_coll.coll_addr,
                                root, fi_dt, 0, (void *) &(MPIDI_OFI_REQUEST(req_ptr, context))),
                   fi_broadcast);

    mpi_errno = MPIC_Wait(req_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Request_free(req_ptr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif
