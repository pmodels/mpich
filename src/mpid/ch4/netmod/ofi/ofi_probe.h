/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_PROBE_H_INCLUDED
#define OFI_PROBE_H_INCLUDED

#include "ofi_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_iprobe(int source,
                                                 int tag,
                                                 MPIR_Comm * comm,
                                                 int context_offset,
                                                 MPIDI_av_entry_t * addr, int vni_src, int vni_dst,
                                                 int *flag,
                                                 MPI_Status * status, MPIR_Request ** message)
{
    int mpi_errno = MPI_SUCCESS;
    fi_addr_t remote_proc;
    uint64_t match_bits, mask_bits;
    MPIR_Request r, *rreq;      /* don't need to init request, output only */
    struct fi_msg_tagged msg;
    int ofi_err;
    int vni_local = vni_dst;
    int vni_remote = vni_src;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(comm, vni_dst, nic);

    MPIR_FUNC_ENTER;

    if (unlikely(MPI_ANY_SOURCE == source))
        remote_proc = FI_ADDR_UNSPEC;
    else
        remote_proc = MPIDI_OFI_av_to_phys(addr, nic, vni_local, vni_remote);

    if (message) {
        MPIDI_CH4_REQUEST_CREATE(rreq, MPIR_REQUEST_KIND__MPROBE, vni_dst, 1);
        MPIR_ERR_CHKANDSTMT((rreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    } else {
        rreq = &r;
    }
    if (message) {
        MPIDI_OFI_REQUEST(rreq, kind) = MPIDI_OFI_req_kind__mprobe;
    } else {
        MPIDI_OFI_REQUEST(rreq, kind) = MPIDI_OFI_req_kind__probe;
    }
    MPIDI_OFI_REQUEST(rreq, huge.remote_info) = NULL;
    rreq->comm = comm;
    MPIR_Comm_add_ref(comm);

    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, comm->recvcontext_id + context_offset, tag);

    MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_PEEK;
    MPL_atomic_release_store_int(&(MPIDI_OFI_REQUEST(rreq, util_id)), MPIDI_OFI_PEEK_START);

    msg.msg_iov = NULL;
    msg.desc = NULL;
    msg.iov_count = 0;
    msg.addr = remote_proc;
    msg.tag = match_bits;
    msg.ignore = mask_bits;
    msg.context = (void *) &(MPIDI_OFI_REQUEST(rreq, context));
    msg.data = 0;

    uint64_t recv_flags = FI_PEEK | FI_COMPLETION;
    if (message) {
        recv_flags |= FI_CLAIM;
    }
    ofi_err = 0;
    MPIDI_OFI_CALL_RETRY_RETURN(fi_trecvmsg(MPIDI_OFI_global.ctx[ctx_idx].rx, &msg, recv_flags),
                                vni_dst, ofi_err);
    if (ofi_err == -FI_ENOMSG) {
        *flag = 0;
        if (message)
            MPIDI_CH4_REQUEST_FREE(rreq);
        goto fn_exit;
    }

    MPIDI_OFI_CALL(ofi_err, trecvmsg);
    MPIDI_OFI_PROGRESS_WHILE(MPL_atomic_acquire_load_int(&(MPIDI_OFI_REQUEST(rreq, util_id))) ==
                             MPIDI_OFI_PEEK_START, vni_dst);

    /* Ordering constraint for util_id is unnecessary after the thread unblocks */
    switch (MPL_atomic_relaxed_load_int(&(MPIDI_OFI_REQUEST(rreq, util_id)))) {
        case MPIDI_OFI_PEEK_NOT_FOUND:
            *flag = 0;

            if (message)
                MPIDI_CH4_REQUEST_FREE(rreq);

            goto fn_exit;

        case MPIDI_OFI_PEEK_FOUND:
            MPIR_Request_extract_status(rreq, status);
            *flag = 1;

            if (message) {
                MPIR_Request_add_ref(rreq);
                *message = rreq;
            }

            break;

        default:
            MPIR_Assert(0);
    }

  fn_exit:
    if (message == NULL) {
        MPIR_Comm_release(comm);
    }
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Common macro used by all MPIDI_NM_mpi_probe routines to facilitate tuning */
#define MPIDI_OFI_PROBE_VNIS(vni_src_, vni_dst_) \
    do { \
        /* NOTE: hashing is based on target rank */ \
        MPIDI_EXPLICIT_VCIS(comm, attr, source, comm->rank, vni_src_, vni_dst_); \
        if (vni_src_ == 0 && vni_dst_ == 0) { \
            vni_src_ = MPIDI_get_vci(SRC_VCI_FROM_RECVER, comm, source, comm->rank, tag); \
            vni_dst_ = MPIDI_get_vci(DST_VCI_FROM_RECVER, comm, source, comm->rank, tag); \
        } \
    } while (0)

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_improbe(int source,
                                                  int tag,
                                                  MPIR_Comm * comm,
                                                  int attr, MPIDI_av_entry_t * addr,
                                                  int *flag, MPIR_Request ** message,
                                                  MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    int vni_src, vni_dst;
    MPIDI_OFI_PROBE_VNIS(vni_src, vni_dst);

    MPIDI_OFI_THREAD_CS_ENTER_VCI_OPTIONAL(vni_dst);
    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_improbe(source, tag, comm, context_offset, vni_dst, flag, message,
                                       status);
    } else {
        /* Set flags for mprobe peek, when ready */
        mpi_errno = MPIDI_OFI_do_iprobe(source, tag, comm, context_offset, addr, vni_src, vni_dst,
                                        flag, status, message);
    }
    MPIDI_OFI_THREAD_CS_EXIT_VCI_OPTIONAL(vni_dst);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iprobe(int source,
                                                 int tag,
                                                 MPIR_Comm * comm,
                                                 int attr, MPIDI_av_entry_t * addr,
                                                 int *flag, MPI_Status * status)
{
    int mpi_errno;
    MPIR_FUNC_ENTER;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);
    int vni_src, vni_dst;
    MPIDI_OFI_PROBE_VNIS(vni_src, vni_dst);

    MPIDI_OFI_THREAD_CS_ENTER_VCI_OPTIONAL(vni_dst);
    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_iprobe(source, tag, comm, context_offset, vni_dst, flag, status);
    } else {
        mpi_errno = MPIDI_OFI_do_iprobe(source, tag, comm, context_offset, addr,
                                        vni_src, vni_dst, flag, status, NULL);
    }
    MPIDI_OFI_THREAD_CS_EXIT_VCI_OPTIONAL(vni_dst);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

#endif /* OFI_PROBE_H_INCLUDED */
