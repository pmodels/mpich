/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_coll_types.h"


MPIDI_OFI_coll_retval_t MPIDI_OFI_coll_bcast(void *buffer, MPI_Aint count,
                                             MPI_Datatype datatype, int root, MPIR_Comm * comm)
{
    enum fi_datatype fi_datatype;
    MPIDI_OFI_coll_request_t request = { 0 };
    ssize_t ret;

    if (!MPIDI_OFI_COMM(comm).coll.initialized ||
        !HANDLE_IS_BUILTIN(datatype) ||
        MPIR_GPU_query_pointer_is_dev(buffer) ||
        MPIDI_OFI_datatype_to_ofi(datatype, &fi_datatype) != MPI_SUCCESS ||
        !MPIDI_OFI_coll_datatype_supported(fi_datatype)) {
        return MPIDI_OFI_COLL_RETVAL_FALLBACK;
    }
    if (MPIDI_OFI_coll_query(comm, FI_BROADCAST, fi_datatype, FI_NOOP) !=
        MPIDI_OFI_COLL_RETVAL_SUCCESS) {
        return MPIDI_OFI_COLL_RETVAL_FALLBACK;
    }

    request.event_id = MPIDI_OFI_EVENT_COLL_DONE;
    ret = fi_broadcast(MPIDI_OFI_COLL_EP, buffer, count, NULL, MPIDI_OFI_COMM(comm).coll.coll_addr,
                       /* libfabric coll treats root_addr as the root's rank in the av_set */
                       (fi_addr_t) root, fi_datatype, 0, &request.context);

    if (ret == -FI_EOPNOTSUPP || ret == -FI_ENOSYS) {
        return MPIDI_OFI_COLL_RETVAL_FALLBACK;
    }
    if (ret) {
        return MPIDI_OFI_COLL_RETVAL_ERROR;
    }
    if (MPIDI_OFI_coll_wait(&request) != MPI_SUCCESS) {
        return MPIDI_OFI_COLL_RETVAL_ERROR;
    }

    return MPIDI_OFI_COLL_RETVAL_SUCCESS;
}
