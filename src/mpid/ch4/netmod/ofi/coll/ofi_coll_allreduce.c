/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_coll_types.h"


MPIDI_OFI_coll_retval_t MPIDI_OFI_coll_allreduce(const void *sendbuf, void *recvbuf,
                                                 MPI_Aint count,
                                                 MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm)
{
    enum fi_datatype fi_datatype;
    enum fi_op fi_op;
    const void *input = sendbuf;
    MPIDI_OFI_coll_request_t request = { 0 };
    ssize_t ret;

    if (!MPIDI_OFI_COMM(comm).coll.initialized ||
        !HANDLE_IS_BUILTIN(datatype) || !HANDLE_IS_BUILTIN(op) ||
        MPIR_GPU_query_pointer_is_dev(recvbuf) ||
        (sendbuf != MPI_IN_PLACE && MPIR_GPU_query_pointer_is_dev(sendbuf)) ||
        MPIDI_OFI_datatype_to_ofi(datatype, &fi_datatype) != MPI_SUCCESS ||
        MPIDI_OFI_op_to_ofi(op, &fi_op) != MPI_SUCCESS ||
        !MPIDI_OFI_coll_datatype_supported(fi_datatype) || !MPIDI_OFI_coll_op_supported(fi_op)) {
        return MPIDI_OFI_COLL_RETVAL_FALLBACK;
    }
    if (MPIDI_OFI_coll_query(comm, FI_ALLREDUCE, fi_datatype, fi_op) !=
        MPIDI_OFI_COLL_RETVAL_SUCCESS) {
        return MPIDI_OFI_COLL_RETVAL_FALLBACK;
    }

    if (sendbuf == MPI_IN_PLACE) {
        input = recvbuf;
    }
    request.event_id = MPIDI_OFI_EVENT_COLL_DONE;
    ret = fi_allreduce(MPIDI_OFI_COLL_EP, input, count, NULL, recvbuf, NULL,
                       MPIDI_OFI_COMM(comm).coll.coll_addr, fi_datatype, fi_op, 0,
                       &request.context);

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
