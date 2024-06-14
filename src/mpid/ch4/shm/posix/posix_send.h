/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * The POSIX shared memory module uses the CH4-fallback active message implementation for tracking
 * requests and performing message matching. This is because these functions are always implemented
 * in software, therefore duplicating the code to perform the matching is a bad idea. In all of
 * these functions, we call back up to the fallback code to start the process of sending the
 * message.
 */

#ifndef POSIX_SEND_H_INCLUDED
#define POSIX_SEND_H_INCLUDED

#include "posix_impl.h"

#define MPIDI_POSIX_SEND_VSIS(vci_src_, vci_dst_) \
    do { \
        MPIDI_EXPLICIT_VCIS(comm, attr, comm->rank, rank, vci_src_, vci_dst_); \
        if (vci_src_ == 0 && vci_dst_ == 0) { \
            vci_src_ = MPIDI_get_vci(SRC_VCI_FROM_SENDER, comm, comm->rank, rank, tag); \
            vci_dst_ = MPIDI_get_vci(DST_VCI_FROM_SENDER, comm, comm->rank, rank, tag); \
        } \
    } while (0)

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_isend(const void *buf, MPI_Aint count,
                                                   MPI_Datatype datatype, int rank, int tag,
                                                   MPIR_Comm * comm, int attr,
                                                   MPIDI_av_entry_t * addr,
                                                   MPIR_cc_t * parent_cc_ptr,
                                                   MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    MPIR_Errflag_t errflag = MPIR_PT2PT_ATTR_GET_ERRFLAG(attr);
    bool syncflag = MPIR_PT2PT_ATTR_GET_SYNCFLAG(attr);

    int vci_src, vci_dst;
    MPIDI_POSIX_SEND_VSIS(vci_src, vci_dst);

    MPIDI_POSIX_THREAD_CS_ENTER_VCI(vci_src);
    bool done = false;
    if (*request == NULL && !syncflag && !MPIDI_POSIX_global.per_vci[vci_src].postponed_queue) {
        /* try eager mode to avoid heavy request.
         * THIS IS A HACK: code need match the am MPIDIG_SEND protocol and posix SHORT protocol
         */
        MPI_Aint data_sz;
        MPIDI_Datatype_check_size(datatype, count, data_sz);
        if ((sizeof(MPIDIG_hdr_t) + data_sz) <= MPIDI_POSIX_am_eager_limit()) {
            /* try eager send, we can use lightweight request if successful */
            MPIDI_POSIX_am_header_t msg_hdr;
            msg_hdr.handler_id = MPIDIG_SEND;
            msg_hdr.am_hdr_sz = sizeof(MPIDIG_hdr_t);
            msg_hdr.am_type = MPIDI_POSIX_AM_TYPE__SHORT;

            MPIDIG_hdr_t am_hdr;
            am_hdr.src_rank = comm->rank;
            am_hdr.tag = tag;
            am_hdr.context_id = comm->context_id + context_offset;
            am_hdr.error_bits = errflag;
            am_hdr.sreq_ptr = NULL;
            am_hdr.flags = MPIDIG_AM_SEND_FLAGS_NONE;
            am_hdr.data_sz = data_sz;
            am_hdr.rndv_hdr_sz = 0;

            int grank = MPIDIU_rank_to_lpid(rank, comm);
            MPI_Aint bytes_sent;
            int rc = MPIDI_POSIX_eager_send(grank, &msg_hdr, &am_hdr, sizeof(am_hdr),
                                            buf, count, datatype, 0, vci_src, vci_dst, &bytes_sent);
            if (rc == MPIDI_POSIX_OK) {
                done = true;
                *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
            }
        }
    }
    if (!done) {
        mpi_errno = MPIDIG_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                     vci_src, vci_dst, request, syncflag, errflag);
    }
    /* if the parent_cc_ptr exists */
    if (parent_cc_ptr) {
        if (MPIR_Request_is_complete(*request)) {
            /* if the request is already completed, decrement the parent counter */
            MPIR_cc_dec(parent_cc_ptr);
        } else {
            /* if the request is not done yet, assign the completion pointer to the parent one and
             * it will be decremented later */
            (*request)->dev.completion_notification = parent_cc_ptr;
        }
    }

    MPIDI_POSIX_THREAD_CS_EXIT_VCI(vci_src);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_cancel_send(MPIR_Request * sreq)
{
    return MPIDIG_mpi_cancel_send(sreq);
}

#endif /* POSIX_SEND_H_INCLUDED */
