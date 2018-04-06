/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_PROBE_H_INCLUDED
#define OFI_PROBE_H_INCLUDED

#include "ofi_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_do_iprobe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_do_iprobe(int source,
                                      int tag,
                                      MPIR_Comm * comm,
                                      int context_offset,
                                      MPIDI_av_entry_t * addr,
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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_IPROBE);

    if (unlikely(MPI_ANY_SOURCE == source))
        remote_proc = FI_ADDR_UNSPEC;
    else
        remote_proc = MPIDI_OFI_av_to_phys(addr);

    if (message) {
        rreq = MPIR_Request_create(MPIR_REQUEST_KIND__MPROBE);
        MPIR_ERR_CHKANDSTMT((rreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    } else {
        rreq = &r;
    }

    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, comm->context_id + context_offset, source, tag);

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

    MPIDI_OFI_CALL_RETURN(fi_trecvmsg(MPIDI_Global.ctx[0].rx, &msg,
                                      peek_flags | FI_PEEK | FI_COMPLETION | (MPIDI_OFI_ENABLE_DATA
                                                                              ? FI_REMOTE_CQ_DATA :
                                                                              0)), ofi_err);
    if (ofi_err == -FI_ENOMSG) {
        *flag = 0;
        if (message)
            MPIR_Request_free(rreq);
        goto fn_exit;
    }

    MPIDI_OFI_CALL(ofi_err, trecvmsg);
    MPIDI_OFI_PROGRESS_WHILE(MPIDI_OFI_REQUEST(rreq, util_id) == MPIDI_OFI_PEEK_START);

    switch (MPIDI_OFI_REQUEST(rreq, util_id)) {
        case MPIDI_OFI_PEEK_NOT_FOUND:
            *flag = 0;

            if (message)
                MPIR_Request_free(rreq);

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

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_improbe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_improbe(int source,
                                       int tag,
                                       MPIR_Comm * comm,
                                       int context_offset, MPIDI_av_entry_t * addr,
                                       int *flag, MPIR_Request ** message, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IMPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IMPROBE);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno =
            MPIDI_CH4U_mpi_improbe(source, tag, comm, context_offset, flag, message, status);
        goto fn_exit;
    }

    /* Set flags for mprobe peek, when ready */
    mpi_errno = MPIDI_OFI_do_iprobe(source, tag, comm, context_offset, addr,
                                    flag, status, message, FI_CLAIM | FI_COMPLETION);
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

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iprobe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iprobe(int source,
                                      int tag,
                                      MPIR_Comm * comm,
                                      int context_offset, MPIDI_av_entry_t * addr, int *flag,
                                      MPI_Status * status)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IPROBE);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDI_CH4U_mpi_iprobe(source, tag, comm, context_offset, flag, status);
        goto fn_exit;
    }

    mpi_errno =
        MPIDI_OFI_do_iprobe(source, tag, comm, context_offset, addr, flag, status, NULL, 0ULL);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IPROBE);
    return mpi_errno;
}

#endif /* OFI_PROBE_H_INCLUDED */
