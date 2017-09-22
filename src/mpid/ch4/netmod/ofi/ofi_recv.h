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
#ifndef OFI_RECV_H_INCLUDED
#define OFI_RECV_H_INCLUDED

#include "ofi_impl.h"

#define MPIDI_OFI_ON_HEAP      0
#define MPIDI_OFI_USE_EXISTING 1

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_do_irecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_irecv(void *buf,
                                                int count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm,
                                                int context_offset,
                                                MPIDI_av_entry_t *addr,
                                                MPIR_Request ** request, int mode, uint64_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    uint64_t match_bits, mask_bits;
    MPIR_Context_id_t context_id = comm->recvcontext_id + context_offset;
    size_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    struct fi_msg_tagged msg;
    char *recv_buf;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_IRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_IRECV);

    if (mode == MPIDI_OFI_ON_HEAP) {    /* Branch should compile out */
        MPIDI_OFI_REQUEST_CREATE(rreq, MPIR_REQUEST_KIND__RECV);
        /* Need to set the source to UNDEFINED for anysource matching */
        rreq->status.MPI_SOURCE = MPI_UNDEFINED;
    }
    else if (mode == MPIDI_OFI_USE_EXISTING) {
        rreq = *request;
        rreq->kind = MPIR_REQUEST_KIND__RECV;
    } else {
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nullptr");
        goto fn_fail;
    }

    *request = rreq;

    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, context_id, rank, tag);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    MPIDI_OFI_REQUEST(rreq, datatype) = datatype;
    dtype_add_ref_if_not_builtin(datatype);

    recv_buf = (char *) buf + dt_true_lb;

    if (!dt_contig) {
        MPIDI_OFI_REQUEST(rreq, noncontig) =
            (MPIDI_OFI_noncontig_t *) MPL_malloc(data_sz + sizeof(MPIR_Segment));
        MPIR_ERR_CHKANDJUMP1(MPIDI_OFI_REQUEST(rreq, noncontig->pack_buffer) == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Recv Pack Buffer alloc");
        recv_buf = MPIDI_OFI_REQUEST(rreq, noncontig->pack_buffer);
        MPIR_Segment_init(buf, count, datatype, &MPIDI_OFI_REQUEST(rreq, noncontig->segment), 0);
    }
    else
        MPIDI_OFI_REQUEST(rreq, noncontig) = NULL;

    MPIDI_OFI_REQUEST(rreq, util_comm) = comm;
    MPIDI_OFI_REQUEST(rreq, util_id) = context_id;

    if (unlikely(data_sz > MPIDI_Global.max_send)) {
        MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV_HUGE;
        data_sz = MPIDI_Global.max_send;
    }
    else
        MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV;

    if (!flags) /* Branch should compile out */
        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_Global.ctx[0].rx,
                                      recv_buf,
                                      data_sz,
                                      NULL,
                                      (MPI_ANY_SOURCE ==
                                       rank) ? FI_ADDR_UNSPEC : MPIDI_OFI_av_to_phys(addr),
                                      match_bits, mask_bits,
                                      (void *) &(MPIDI_OFI_REQUEST(rreq, context))), trecv,
                             MPIDI_OFI_CALL_LOCK);
    else {
        MPIDI_OFI_REQUEST(rreq, util.iov).iov_base = recv_buf;
        MPIDI_OFI_REQUEST(rreq, util.iov).iov_len = data_sz;

        msg.msg_iov = &MPIDI_OFI_REQUEST(rreq, util.iov);
        msg.desc = NULL;
        msg.iov_count = 1;
        msg.tag = match_bits;
        msg.ignore = mask_bits;
        msg.context = (void *) &(MPIDI_OFI_REQUEST(rreq, context));
        msg.data = 0;
        msg.addr = FI_ADDR_UNSPEC;

        MPIDI_OFI_CALL_RETRY(fi_trecvmsg(MPIDI_Global.ctx[0].rx, &msg, flags), trecv,
                             MPIDI_OFI_CALL_LOCK);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_IRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_recv(void *buf,
                                               int count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag,
                                               MPIR_Comm * comm,
                                               int context_offset, MPIDI_av_entry_t *addr,
                                               MPI_Status * status, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RECV);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_recv(buf, count, datatype, rank, tag, comm, context_offset, status, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_irecv(buf, count, datatype, rank, tag, comm,
                                   context_offset, addr, request, MPIDI_OFI_ON_HEAP, 0ULL);

fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RECV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_recv_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_recv_init(void *buf,
                                                    int count,
                                                    MPI_Datatype datatype,
                                                    int rank,
                                                    int tag,
                                                    MPIR_Comm * comm,
                                                    int context_offset, MPIDI_av_entry_t *addr, MPIR_Request ** request)
{
    MPIR_Request *rreq;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RECV_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RECV_INIT);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_recv_init(buf, count, datatype, rank, tag, comm, context_offset, request);
        goto fn_exit;
    }

    MPIDI_OFI_REQUEST_CREATE((rreq), MPIR_REQUEST_KIND__PREQUEST_RECV);

    *request = rreq;
    rreq->comm = comm;
    MPIR_Comm_add_ref(comm);

    MPIDI_OFI_REQUEST(rreq, util.persist.buf) = (void *) buf;
    MPIDI_OFI_REQUEST(rreq, util.persist.count) = count;
    MPIDI_OFI_REQUEST(rreq, datatype) = datatype;
    MPIDI_OFI_REQUEST(rreq, util.persist.rank) = rank;
    MPIDI_OFI_REQUEST(rreq, util.persist.tag) = tag;
    MPIDI_OFI_REQUEST(rreq, util_comm) = comm;
    MPIDI_OFI_REQUEST(rreq, util_id) = comm->context_id + context_offset;
    rreq->u.persist.real_request = NULL;

    MPIDI_CH4U_request_complete(rreq);

    MPIDI_OFI_REQUEST(rreq, util.persist.type) = MPIDI_PTYPE_RECV;

    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
        MPIR_Datatype *dt_ptr;
        MPIR_Datatype_get_ptr(datatype, dt_ptr);
        MPIR_Datatype_add_ref(dt_ptr);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RECV_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_imrecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_imrecv(void *buf,
                                                 int count,
                                                 MPI_Datatype datatype,
                                                 MPIR_Request * message, MPIR_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDI_av_entry_t *av;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IMRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IMRECV);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_imrecv(buf, count, datatype, message, rreqp);
        goto fn_exit;
    }

    MPIR_Assert(message->kind == MPIR_REQUEST_KIND__MPROBE);

    *rreqp = rreq = message;

    av = MPIDIU_comm_rank_to_av(rreq->comm, message->status.MPI_SOURCE);
    mpi_errno = MPIDI_OFI_do_irecv(buf, count, datatype, message->status.MPI_SOURCE,
                                   message->status.MPI_TAG, rreq->comm, 0, av,
                                   &rreq, MPIDI_OFI_USE_EXISTING, FI_CLAIM | FI_COMPLETION);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IMRECV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_irecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_irecv(void *buf,
                                                int count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int context_offset, MPIDI_av_entry_t *addr,
                                                MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IRECV);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_do_irecv(buf, count, datatype, rank, tag, comm,
                                   context_offset, addr, request, MPIDI_OFI_ON_HEAP, 0ULL);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IRECV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_cancel_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_recv(MPIR_Request * rreq)
{

    int mpi_errno = MPI_SUCCESS;
    ssize_t ret;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_CANCEL_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_CANCEL_RECV);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_cancel_recv(rreq);
        goto fn_exit;
    }

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_OFI_THREAD_FI_MUTEX);
    ret = fi_cancel((fid_t) MPIDI_Global.ctx[0].rx, &(MPIDI_OFI_REQUEST(rreq, context)));
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_OFI_THREAD_FI_MUTEX);

    if (ret == 0) {
        while ((!MPIR_STATUS_GET_CANCEL_BIT(rreq->status)) && (!MPIR_cc_is_complete(&rreq->cc))) {
            /* The cancel is local and must complete, so only poll this device (not global progress) */
            if ((mpi_errno =
                 MPIDI_NM_progress(0, 0)) != MPI_SUCCESS)
                goto fn_exit;
        }

        if (MPIR_STATUS_GET_CANCEL_BIT(rreq->status)) {
            MPIR_STATUS_SET_CANCEL_BIT(rreq->status, TRUE);
            MPIR_STATUS_SET_COUNT(rreq->status, 0);
            MPIDI_CH4U_request_complete(rreq);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_CANCEL_RECV);
    return mpi_errno;

#ifndef MPIDI_BUILD_CH4_SHM
  fn_fail:
    goto fn_exit;
#endif
}

#endif /* OFI_RECV_H_INCLUDED */
