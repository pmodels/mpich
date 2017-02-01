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
#ifndef OFI_SEND_H_INCLUDED
#define OFI_SEND_H_INCLUDED

#include "ofi_impl.h"
#include <../mpi/pt2pt/bsendutil.h>

#define MPIDI_OFI_SENDPARAMS const void *buf,int count,MPI_Datatype datatype, \
    int rank,int tag,MPIR_Comm *comm,                               \
    int context_offset,int ep_idx,MPIR_Request **request

#define MPIDI_OFI_SENDARGS buf,count,datatype,rank,tag, \
        comm,context_offset,ep_idx,request

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_send_lightweight
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_lightweight(const void *buf,
                                                        size_t data_sz,
                                                        int rank,
                                                        int tag, MPIR_Comm * comm,
                                                        int context_offset, int ep_idx)
{
    int mpi_errno = MPI_SUCCESS;
    uint64_t match_bits;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_SEND_LIGHTWEIGHT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_SEND_LIGHTWEIGHT);
    match_bits =
        MPIDI_OFI_init_sendtag(comm->context_id + context_offset, comm->rank, tag, 0);
    mpi_errno =
        MPIDI_OFI_send_handler(MPIDI_OFI_EP_TX_TAG(ep_idx), buf, data_sz, NULL, comm->rank,
                               MPIDI_OFI_comm_to_phys(comm, rank,
                                   MPIDI_CH4_ep_rx_tag(comm, comm->rank, rank, tag),
                                   MPIDI_OFI_API_TAG), match_bits,
                               NULL, MPIDI_OFI_DO_INJECT, MPIDI_OFI_CALL_LOCK);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_SEND_LIGHTWEIGHT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_send_lightweight_request
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_lightweight_request(const void *buf,
                                                                size_t data_sz,
                                                                int rank,
                                                                int tag,
                                                                MPIR_Comm * comm,
                                                                int context_offset, int ep_idx,
                                                                MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    uint64_t match_bits;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_SEND_LIGHTWEIGHT_REQUEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_SEND_LIGHTWEIGHT_REQUEST);
    MPIR_Request *r;
    MPIDI_OFI_SEND_REQUEST_CREATE_LW(r);
    *request = r;
    match_bits =
        MPIDI_OFI_init_sendtag(comm->context_id + context_offset, comm->rank, tag, 0);
    mpi_errno =
        MPIDI_OFI_send_handler(MPIDI_OFI_EP_TX_TAG(ep_idx), buf, data_sz, NULL, comm->rank,
                               MPIDI_OFI_comm_to_phys(comm, rank,
                                   MPIDI_CH4_ep_rx_tag(comm, comm->rank, rank, tag),
                                   MPIDI_OFI_API_TAG), match_bits,
                               NULL, MPIDI_OFI_DO_INJECT, MPIDI_OFI_CALL_LOCK);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_SEND_LIGHTWEIGHT_REQUEST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_send_normal
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_normal(MPIDI_OFI_SENDPARAMS,
                                                   int dt_contig,
                                                   size_t data_sz,
                                                   MPIR_Datatype * dt_ptr,
                                                   MPI_Aint dt_true_lb, uint64_t type)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPI_Aint last;
    char *send_buf;
    uint64_t match_bits;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_SEND_NORMAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_SEND_NORMAL);

    MPIDI_OFI_REQUEST_CREATE(sreq, MPIR_REQUEST_KIND__SEND);
    *request = sreq;
    match_bits =
        MPIDI_OFI_init_sendtag(comm->context_id + context_offset, comm->rank, tag, type);
    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND;
    MPIDI_OFI_REQUEST(sreq, datatype) = datatype;
    dtype_add_ref_if_not_builtin(datatype);

    if (type == MPIDI_OFI_SYNC_SEND) {  /* Branch should compile out */
        int c = 1;
        uint64_t ssend_match, ssend_mask;
        MPIDI_OFI_ssendack_request_t *ackreq;
        MPIDI_OFI_SSEND_ACKREQUEST_CREATE(ackreq);
        ackreq->event_id = MPIDI_OFI_EVENT_SSEND_ACK;
        ackreq->signal_req = sreq;
        MPIR_cc_incr(sreq->cc_ptr, &c);
        ssend_match =
            MPIDI_OFI_init_recvtag(&ssend_mask, comm->context_id + context_offset, rank, tag);
        ssend_match |= MPIDI_OFI_SYNC_SEND_ACK;
        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_EP_RX_TAG(0),   /* endpoint    */
                                      NULL,     /* recvbuf     */
                                      0,        /* data sz     */
                                      NULL,     /* memregion descr  */
                                      MPIDI_OFI_comm_to_phys(comm, rank,
                                           MPIDI_CH4_ep_rx_tag(comm, comm->rank, rank, tag),
                                           MPIDI_OFI_API_TAG),     /* remote proc */
                                      ssend_match,      /* match bits  */
                                      0ULL,     /* mask bits   */
                                      (void *) &(ackreq->context)), trecvsync, MPIDI_OFI_CALL_LOCK);
    }

    send_buf = (char *) buf + dt_true_lb;

    if (!dt_contig) {
        size_t segment_first;
        segment_first = 0;
        last = data_sz;
        MPIDI_OFI_REQUEST(sreq, noncontig) =
            (MPIDI_OFI_noncontig_t *) MPL_malloc(data_sz + sizeof(MPID_Segment));
        MPIR_ERR_CHKANDJUMP1(MPIDI_OFI_REQUEST(sreq, noncontig) == NULL, mpi_errno, MPI_ERR_OTHER,
                             "**nomem", "**nomem %s", "Send Pack buffer alloc");
        MPID_Segment_init(buf, count, datatype, &MPIDI_OFI_REQUEST(sreq, noncontig->segment), 0);
        MPID_Segment_pack(&MPIDI_OFI_REQUEST(sreq, noncontig->segment), segment_first, &last,
                          MPIDI_OFI_REQUEST(sreq, noncontig->pack_buffer));
        send_buf = MPIDI_OFI_REQUEST(sreq, noncontig->pack_buffer);
    }
    else
        MPIDI_OFI_REQUEST(sreq, noncontig) = NULL;

    if (data_sz <= MPIDI_Global.max_buffered_send) {
        mpi_errno =
            MPIDI_OFI_send_handler(MPIDI_OFI_EP_TX_TAG(ep_idx), send_buf, data_sz, NULL, comm->rank,
                                   MPIDI_OFI_comm_to_phys(comm, rank,
                                       MPIDI_CH4_ep_rx_tag(comm, comm->rank, rank, tag),
                                       MPIDI_OFI_API_TAG),
                                   match_bits, NULL, MPIDI_OFI_DO_INJECT,
                                   MPIDI_OFI_CALL_LOCK);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIDI_OFI_send_event(NULL, sreq);
    }
    else if (data_sz <= MPIDI_Global.max_send) {
        mpi_errno =
            MPIDI_OFI_send_handler(MPIDI_OFI_EP_TX_TAG(ep_idx), send_buf, data_sz, NULL, comm->rank,
                                   MPIDI_OFI_comm_to_phys(comm, rank,
                                       MPIDI_CH4_ep_rx_tag(comm, comm->rank, rank, tag),
                                       MPIDI_OFI_API_TAG),
                                   match_bits, (void *) &(MPIDI_OFI_REQUEST(sreq, context)),
                                   MPIDI_OFI_DO_SEND, MPIDI_OFI_CALL_LOCK);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    else if (unlikely(1)) {
        MPIDI_OFI_send_control_t ctrl;
        int c;
        uint64_t rma_key = 0;
        MPIDI_OFI_huge_counter_t *cntr;
        void *ptr;
        c = 1;
        MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND_HUGE;
        MPIR_cc_incr(sreq->cc_ptr, &c);
        ptr = MPIDI_OFI_map_lookup(MPIDI_OFI_COMM(comm).huge_send_counters, rank);

        MPID_THREAD_CS_ENTER(POBJ, MPIDI_OFI_THREAD_FI_MUTEX);

        if (ptr == MPIDI_OFI_MAP_NOT_FOUND) {
            ptr = MPL_malloc(sizeof(MPIDI_OFI_huge_counter_t));
            cntr = (MPIDI_OFI_huge_counter_t *) ptr;
            cntr->outstanding = 0;
            cntr->counter = 0;
            MPIDI_OFI_map_set(MPIDI_OFI_COMM(comm).huge_send_counters, rank, ptr);
        }

        cntr = (MPIDI_OFI_huge_counter_t *) ptr;

        ctrl.rma_key = MPIDI_OFI_index_allocator_alloc(MPIDI_OFI_COMM(comm).rma_id_allocator);
        MPIR_Assert(ctrl.rma_key < MPIDI_Global.max_huge_rmas);
        if (MPIDI_OFI_ENABLE_MR_SCALABLE)
            rma_key = ctrl.rma_key << MPIDI_Global.huge_rma_shift;
        MPIDI_OFI_CALL_NOLOCK(fi_mr_reg(MPIDI_Global.domain,    /* In:  Domain Object       */
                                        send_buf,       /* In:  Lower memory address */
                                        data_sz,        /* In:  Length              */
                                        FI_REMOTE_READ, /* In:  Expose MR for read  */
                                        0ULL,   /* In:  offset(not used)    */
                                        rma_key,        /* In:  requested key       */
                                        0ULL,   /* In:  flags               */
                                        &cntr->mr,      /* Out: memregion object    */
                                        NULL), mr_reg); /* In:  context             */

        if (!MPIDI_OFI_ENABLE_MR_SCALABLE) {
            /* MR_BASIC */
            ctrl.rma_key = fi_mr_key(cntr->mr);
        }

        cntr->outstanding++;
        cntr->counter++;
        MPIR_Assert(cntr->outstanding != USHRT_MAX);
        MPIR_Assert(cntr->counter != USHRT_MAX);
        MPIDI_OFI_REQUEST(sreq, util_comm) = comm;
        MPIDI_OFI_REQUEST(sreq, util_id) = rank;
        mpi_errno = MPIDI_OFI_send_handler(MPIDI_OFI_EP_TX_TAG(ep_idx), send_buf,
                                           MPIDI_Global.max_send,
                                           NULL,
                                           comm->rank,
                                           MPIDI_OFI_comm_to_phys(comm, rank,
                                               MPIDI_CH4_ep_rx_tag(comm, comm->rank, rank, tag),
                                               MPIDI_OFI_API_TAG),
                                           match_bits,
                                           (void *) &(MPIDI_OFI_REQUEST(sreq, context)),
                                           MPIDI_OFI_DO_SEND,
                                           MPIDI_OFI_CALL_NO_LOCK);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        ctrl.type = MPIDI_OFI_CTRL_HUGE;
        ctrl.seqno = cntr->counter - 1;
        MPIDI_OFI_MPI_CALL_POP(MPIDI_OFI_do_control_send
                               (&ctrl, send_buf, data_sz, rank, comm, sreq, FALSE));
        MPID_THREAD_CS_EXIT(POBJ, MPIDI_OFI_THREAD_FI_MUTEX);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_SEND_NORMAL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send(MPIDI_OFI_SENDPARAMS, int noreq, uint64_t syncflag)
{
    int dt_contig, mpi_errno;
    size_t data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_SEND);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (likely(!syncflag && dt_contig && (data_sz <= MPIDI_Global.max_buffered_send)))
        if (noreq)
            mpi_errno = MPIDI_OFI_send_lightweight((char *) buf + dt_true_lb, data_sz,
                                                   rank, tag, comm, context_offset, ep_idx);
        else
            mpi_errno = MPIDI_OFI_send_lightweight_request((char *) buf + dt_true_lb, data_sz,
                                                           rank, tag, comm, context_offset, ep_idx,
                                                           request);
    else
        mpi_errno = MPIDI_OFI_send_normal(buf, count, datatype, rank, tag, comm,
                                          context_offset, ep_idx, request, dt_contig,
                                          data_sz, dt_ptr, dt_true_lb, syncflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_SEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_persistent_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_persistent_send(MPIDI_OFI_SENDPARAMS)
{
    MPIR_Request *sreq;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_PERSISTENT_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_PERSISTENT_SEND);

    MPIDI_OFI_REQUEST_CREATE(sreq, MPIR_REQUEST_KIND__PREQUEST_SEND);
    *request = sreq;

    MPIR_Comm_add_ref(comm);
    sreq->comm = comm;
    MPIDI_OFI_REQUEST(sreq, util.persist.buf) = (void *) buf;
    MPIDI_OFI_REQUEST(sreq, util.persist.count) = count;
    MPIDI_OFI_REQUEST(sreq, datatype) = datatype;
    MPIDI_OFI_REQUEST(sreq, util.persist.rank) = rank;
    MPIDI_OFI_REQUEST(sreq, util.persist.tag) = tag;
    MPIDI_OFI_REQUEST(sreq, util_comm) = comm;
    MPIDI_OFI_REQUEST(sreq, util_id) = comm->context_id + context_offset;
    sreq->u.persist.real_request = NULL;
    MPIDI_CH4U_request_complete(sreq);

    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
        MPIR_Datatype *dt_ptr;
        MPID_Datatype_get_ptr(datatype, dt_ptr);
        MPID_Datatype_add_ref(dt_ptr);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_PERSISTENT_SEND);
    return MPI_SUCCESS;
}

#define STARTALL_CASE(CASELABEL,FUNC,CONTEXTID) \
  case CASELABEL:                               \
  {                                             \
    mpi_errno = FUNC(MPIDI_OFI_REQUEST(preq,util.persist.buf),              \
                     MPIDI_OFI_REQUEST(preq,util.persist.count),            \
                     MPIDI_OFI_REQUEST(preq,datatype),           \
                     MPIDI_OFI_REQUEST(preq,util.persist.rank),             \
                     MPIDI_OFI_REQUEST(preq,util.persist.tag),              \
                     preq->comm,                       \
                     MPIDI_OFI_REQUEST(preq,util_id) -           \
                     CONTEXTID,                        \
                     &preq->u.persist.real_request);          \
    break;                                      \
  }

#define STARTALL_CASE_EP(CASELABEL,FUNC,CONTEXTID)                      \
  case CASELABEL:                                                       \
  {                                                                     \
      mpi_errno = FUNC(MPIDI_OFI_REQUEST(preq,util.persist.buf),        \
                       MPIDI_OFI_REQUEST(preq,util.persist.count),      \
                       MPIDI_OFI_REQUEST(preq,datatype),                \
                       MPIDI_OFI_REQUEST(preq,util.persist.rank),       \
                       MPIDI_OFI_REQUEST(preq,util.persist.tag),        \
                       preq->comm,                                      \
                       MPIDI_OFI_REQUEST(preq,util_id) -                \
                       CONTEXTID,                                       \
                       0,  /* FIXME: ep_idx */                          \
                       &preq->u.persist.real_request);                  \
      break;                                                            \
  }

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_send(MPIDI_OFI_SENDPARAMS)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SEND);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_send(buf, count, datatype, rank, tag, comm, context_offset, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_send(MPIDI_OFI_SENDARGS, 1, 0ULL);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ssend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ssend(MPIDI_OFI_SENDPARAMS)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SSEND);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_ssend(buf, count, datatype, rank, tag, comm, context_offset, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_send(MPIDI_OFI_SENDARGS, 0, MPIDI_OFI_SYNC_SEND);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SSEND);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_isend(MPIDI_OFI_SENDPARAMS)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISEND);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_send(MPIDI_OFI_SENDARGS, 0, 0ULL);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_issend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_issend(MPIDI_OFI_SENDPARAMS)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISSEND);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_issend(buf, count, datatype, rank, tag, comm, context_offset, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_send(MPIDI_OFI_SENDARGS, 0, MPIDI_OFI_SYNC_SEND);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISSEND);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_startall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_startall(int count, MPIR_Request * requests[])
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_STARTALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_STARTALL);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_startall(count, requests);
        goto fn_exit;
    }

    for (i = 0; i < count; i++) {
        MPIR_Request *const preq = requests[i];

        switch (MPIDI_OFI_REQUEST(preq, util.persist.type)) {
#ifdef MPIDI_BUILD_CH4_SHM
            STARTALL_CASE_EP(MPIDI_PTYPE_RECV, MPIDI_NM_mpi_irecv, preq->comm->recvcontext_id);
#else
            STARTALL_CASE(MPIDI_PTYPE_RECV, MPID_Irecv, preq->comm->recvcontext_id);
#endif

#ifdef MPIDI_BUILD_CH4_SHM
            STARTALL_CASE_EP(MPIDI_PTYPE_SEND, MPIDI_NM_mpi_isend, preq->comm->context_id);
#else
            STARTALL_CASE(MPIDI_PTYPE_SEND, MPID_Isend, preq->comm->context_id);
#endif
            STARTALL_CASE(MPIDI_PTYPE_SSEND, MPID_Issend, preq->comm->context_id);

        case MPIDI_PTYPE_BSEND:{
                MPI_Request sreq_handle;
                mpi_errno = MPIR_Ibsend_impl(MPIDI_OFI_REQUEST(preq, util.persist.buf),
                                      MPIDI_OFI_REQUEST(preq, util.persist.count),
                                      MPIDI_OFI_REQUEST(preq, datatype),
                                      MPIDI_OFI_REQUEST(preq, util.persist.rank),
                                      MPIDI_OFI_REQUEST(preq, util.persist.tag),
                                      preq->comm, &sreq_handle);

                if (mpi_errno == MPI_SUCCESS)
                    MPIR_Request_get_ptr(sreq_handle, preq->u.persist.real_request);

                break;
            }

        default:
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, __FUNCTION__,
                                      __LINE__, MPI_ERR_INTERN, "**ch3|badreqtype",
                                      "**ch3|badreqtype %d", MPIDI_OFI_REQUEST(preq,
                                                                               util.persist.type));
        }

        if (mpi_errno == MPI_SUCCESS) {
            preq->status.MPI_ERROR = MPI_SUCCESS;

            if (MPIDI_OFI_REQUEST(preq, util.persist.type) == MPIDI_PTYPE_BSEND) {
                preq->cc_ptr = &preq->cc;
                MPIR_cc_set(&preq->cc, 0);
            }
            else
                preq->cc_ptr = &preq->u.persist.real_request->cc;
        }
        else {
            preq->u.persist.real_request = NULL;
            preq->status.MPI_ERROR = mpi_errno;
            preq->cc_ptr = &preq->cc;
            MPIR_cc_set(&preq->cc, 0);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_STARTALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_send_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_send_init(MPIDI_OFI_SENDPARAMS)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SEND_INIT);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_send_init(buf, count, datatype, rank, tag, comm, context_offset, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_persistent_send(MPIDI_OFI_SENDARGS);
    MPIDI_OFI_REQUEST((*request), util.persist.type) = MPIDI_PTYPE_SEND;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SEND_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ssend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ssend_init(MPIDI_OFI_SENDPARAMS)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SSEND_INIT);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_ssend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_persistent_send(MPIDI_OFI_SENDARGS);
    MPIDI_OFI_REQUEST((*request), util.persist.type) = MPIDI_PTYPE_SSEND;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SSEND_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_bsend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_bsend_init(MPIDI_OFI_SENDPARAMS)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_BSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_BSEND_INIT);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_bsend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_persistent_send(MPIDI_OFI_SENDARGS);
    MPIDI_OFI_REQUEST((*request), util.persist.type) = MPIDI_PTYPE_BSEND;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_BSEND_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_rsend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rsend_init(MPIDI_OFI_SENDPARAMS)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RSEND_INIT);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_rsend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_persistent_send(MPIDI_OFI_SENDARGS);
    MPIDI_OFI_REQUEST((*request), util.persist.type) = MPIDI_PTYPE_SEND;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RSEND_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_cancel_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_send(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_CANCEL_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_CANCEL_SEND);
    /* Sends cannot be cancelled */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_CANCEL_SEND);
    return mpi_errno;
}

#endif /* OFI_SEND_H_INCLUDED */
