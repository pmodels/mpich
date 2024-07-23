/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef NETMOD_AM_FALLBACK_SEND_H_INCLUDED
#define NETMOD_AM_FALLBACK_SEND_H_INCLUDED

/* parent_cc_ptr: allows the created request to decrement the counter of a parent request when completed. use NULL to not use this mechanism */
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_isend(const void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int attr,
                                                MPIDI_av_entry_t * addr,
                                                MPIR_cc_t * parent_cc_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    MPIR_Errflag_t errflag = MPIR_PT2PT_ATTR_GET_ERRFLAG(attr);
    bool syncflag = MPIR_PT2PT_ATTR_GET_SYNCFLAG(attr);

    int vci_src, vci_dst;
    vci_src = 0;
    vci_dst = 0;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci_src).lock);
    mpi_errno = MPIDIG_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                 vci_src, vci_dst, request, syncflag, errflag);
    /* if the parent_cc_ptr exists */
    if (parent_cc_ptr) {
        if (MPIR_Request_is_complete(*request)) {
            /* if the request is already completed, decrement the parent counter */
            MPIR_cc_dec(parent_cc_ptr);
        } else {
            /* if the request is not done yet, assign the completion pointer to the parent one and it will be decremented later */
            (*request)->dev.completion_notification = parent_cc_ptr;
        }
    }
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci_src).lock);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_send(MPIR_Request * sreq)
{
    return MPIDIG_mpi_cancel_send(sreq);
}

#endif /* NETMOD_AM_FALLBACK_SEND_H_INCLUDED */
