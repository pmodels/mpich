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
                                                 MPI_Status * status,
                                                 MPIR_Request ** message, uint64_t peek_flags)
{
    int mpi_errno = MPI_SUCCESS;
    fi_addr_t remote_proc;
    uint64_t match_bits, mask_bits;
    MPIR_Request r, *rreq;      /* don't need to init request, output only */
    struct fi_msg_tagged msg;
    int ofi_err;
    int vni_local = vni_dst;
    int vni_remote = vni_src;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_IPROBE);

    if (unlikely(MPI_ANY_SOURCE == source))
        remote_proc = FI_ADDR_UNSPEC;
    else
        remote_proc = MPIDI_OFI_av_to_phys(addr, vni_local, vni_remote);

    if (message) {
        rreq = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__MPROBE, vni_dst);
        MPIR_ERR_CHKANDSTMT((rreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    } else {
        rreq = &r;
    }

    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, comm->recvcontext_id + context_offset, tag);

    MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_PEEK;
    MPIDI_OFI_REQUEST(rreq, util_id) = MPIDI_OFI_PEEK_START;

    msg.msg_iov = NULL;
    msg.desc = NULL;
    msg.iov_count = 0;
    msg.addr = remote_proc;
    msg.tag = match_bits;
    msg.ignore = mask_bits;
    msg.context = (void *) &(MPIDI_OFI_REQUEST(rreq, context));
    msg.data = 0;

    MPIDI_OFI_CALL_RETURN(fi_trecvmsg(MPIDI_OFI_global.ctx[vni_dst].rx, &msg,
                                      peek_flags | FI_PEEK | FI_COMPLETION | FI_REMOTE_CQ_DATA),
                          ofi_err);
    if (ofi_err == -FI_ENOMSG) {
        *flag = 0;
        if (message)
            MPIR_Request_free_unsafe(rreq);
        goto fn_exit;
    }

    MPIDI_OFI_CALL(ofi_err, trecvmsg);
    MPIDI_OFI_PROGRESS_WHILE(MPIDI_OFI_REQUEST(rreq, util_id) == MPIDI_OFI_PEEK_START, vni_dst);

    switch (MPIDI_OFI_REQUEST(rreq, util_id)) {
        case MPIDI_OFI_PEEK_NOT_FOUND:
            *flag = 0;

            if (message)
                MPIR_Request_free_unsafe(rreq);

            goto fn_exit;
            break;

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_IPROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Common macro used by all MPIDI_NM_mpi_probe routines to facilitate tuning */
#define MPIDI_OFI_PROBE_VNIS(vni_src_, vni_dst_) \
    do { \
        /* NOTE: hashing is based on target rank */ \
        vni_src_ = MPIDI_OFI_get_vni(SRC_VCI_FROM_RECVER, comm, source, comm->rank, tag); \
        vni_dst_ = MPIDI_OFI_get_vni(DST_VCI_FROM_RECVER, comm, source, comm->rank, tag); \
    } while (0)

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_improbe(int source,
                                                  int tag,
                                                  MPIR_Comm * comm,
                                                  int context_offset, MPIDI_av_entry_t * addr,
                                                  int *flag, MPIR_Request ** message,
                                                  MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IMPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IMPROBE);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_improbe(source, tag, comm, context_offset, flag, message, status);
        goto fn_exit;
    }

    int vni_src, vni_dst;
    MPIDI_OFI_PROBE_VNIS(vni_src, vni_dst);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni_dst).lock);
    /* Set flags for mprobe peek, when ready */
    mpi_errno = MPIDI_OFI_do_iprobe(source, tag, comm, context_offset, addr, vni_src, vni_dst,
                                    flag, status, message, FI_CLAIM | FI_COMPLETION);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni_dst).lock);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_exit;

    if (*flag && *message) {
        (*message)->comm = comm;
        MPIR_Object_add_ref(comm);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IMPROBE);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iprobe(int source,
                                                 int tag,
                                                 MPIR_Comm * comm,
                                                 int context_offset, MPIDI_av_entry_t * addr,
                                                 int *flag, MPI_Status * status)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IPROBE);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_iprobe(source, tag, comm, context_offset, flag, status);
    } else {
        int vni_src, vni_dst;
        MPIDI_OFI_PROBE_VNIS(vni_src, vni_dst);
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vni_dst).lock);
        mpi_errno = MPIDI_OFI_do_iprobe(source, tag, comm, context_offset, addr,
                                        vni_src, vni_dst, flag, status, NULL, 0ULL);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vni_dst).lock);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IPROBE);
    return mpi_errno;
}

#endif /* OFI_PROBE_H_INCLUDED */
