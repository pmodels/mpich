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

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_send_lightweight
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_lightweight(const void *buf,
                                                        size_t data_sz,
                                                        int rank,
                                                        int tag, MPIR_Comm * comm,
                                                        int context_offset, MPIDI_av_entry_t * addr)
{
    int mpi_errno = MPI_SUCCESS;
    uint64_t match_bits;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_SEND_LIGHTWEIGHT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_SEND_LIGHTWEIGHT);
    match_bits = MPIDI_OFI_init_sendtag(comm->context_id + context_offset, comm->rank, tag, 0);
    mpi_errno =
        MPIDI_OFI_send_handler(MPIDI_Global.ctx[0].tx, buf, data_sz, NULL, comm->rank,
                               MPIDI_OFI_av_to_phys(addr), match_bits,
                               NULL, MPIDI_OFI_DO_INJECT, MPIDI_OFI_CALL_LOCK,
                               MPIDI_OFI_COMM(comm).eagain);
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
                                                                int context_offset,
                                                                MPIDI_av_entry_t * addr,
                                                                MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    uint64_t match_bits;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_SEND_LIGHTWEIGHT_REQUEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_SEND_LIGHTWEIGHT_REQUEST);
    MPIR_Request *r;
    MPIDI_OFI_SEND_REQUEST_CREATE_LW(r);
    *request = r;
    match_bits = MPIDI_OFI_init_sendtag(comm->context_id + context_offset, comm->rank, tag, 0);
    mpi_errno =
        MPIDI_OFI_send_handler(MPIDI_Global.ctx[0].tx, buf, data_sz, NULL, comm->rank,
                               MPIDI_OFI_av_to_phys(addr), match_bits,
                               NULL, MPIDI_OFI_DO_INJECT, MPIDI_OFI_CALL_LOCK,
                               MPIDI_OFI_COMM(comm).eagain);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_SEND_LIGHTWEIGHT_REQUEST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#define MPIDI_OFI_SEND_NEEDS_PACK (-1)  /* Could not use iov; needs packing */

/*
  Tries to send non-contiguous datatype using iovec.

  Return value:
  MPI_SUCCESS: Message was sent successfully using iovec
  MPIDI_OFI_SEND_NEEDS_PACK: There was no error but send was not initiated
      due to limitations with iovec. Needs to fall back to the pack path.
  Other: An error occurred as indicated in the code.
*/
#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_send_iov
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_iov(const void *buf, MPI_Aint count,
                                                int rank, uint64_t match_bits, MPIR_Comm * comm,
                                                MPIR_Request * sreq, MPIR_Datatype * dt_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    struct iovec *originv = NULL, *originv_huge = NULL;
    size_t countp = MPIDI_OFI_count_iov(count, MPIDI_OFI_REQUEST(sreq, datatype), INT64_MAX);
    size_t omax = MPIDI_Global.tx_iov_limit;
    size_t o_size = sizeof(struct iovec);
    size_t cur_o = 0;
    struct fi_msg_tagged msg;
    uint64_t flags;
    unsigned map_size;
    int num_contig, size, j = 0, k = 0, huge = 0, length = 0;
    size_t oout = 0;
    size_t l = 0;
    size_t countp_huge = 0;
    MPIR_Segment seg;
    DLOOP_Offset last_byte = dt_ptr->size * count;
    size_t iov_align = MPL_MAX(MPIDI_OFI_IOVEC_ALIGN, sizeof(void *));

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_SEND_IOV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_SEND_IOV);

    /* If the number of iovecs is greater than the supported hardware limit (to transfer in a single send),
     *  fallback to the pack path */
    if (countp > omax) {
        goto pack;
    }

    flags = FI_COMPLETION | (MPIDI_OFI_ENABLE_DATA ? FI_REMOTE_CQ_DATA : 0);
    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND_NOPACK;

    map_size = dt_ptr->max_contig_blocks * count + 1;
    num_contig = map_size;      /* map_size is the maximum number of iovecs that can be generated */

    size = o_size * num_contig + sizeof(*(MPIDI_OFI_REQUEST(sreq, noncontig.nopack)));

    MPIDI_OFI_REQUEST(sreq, noncontig.nopack) = MPL_aligned_alloc(iov_align, size, MPL_MEM_BUFFER);
    memset(MPIDI_OFI_REQUEST(sreq, noncontig.nopack), 0, size);

    MPIR_Segment_init(buf, count, MPIDI_OFI_REQUEST(sreq, datatype), &seg);
    MPIR_Segment_pack_vector(&seg, 0, &last_byte, MPIDI_OFI_REQUEST(sreq, noncontig.nopack),
                             &num_contig);

    originv = &(MPIDI_OFI_REQUEST(sreq, noncontig.nopack[cur_o]));
    oout = num_contig;  /* num_contig is the actual number of iovecs returned by the Segment_pack_vector function */

    if (oout > omax) {
        MPL_free(MPIDI_OFI_REQUEST(sreq, noncontig.nopack));
        goto pack;
    }

    /* check if the length of any iovec in the current iovec array exceeds the huge message threshold
     * and calculate the total number of iovecs */
    for (j = 0; j < num_contig; j++) {
        if (originv[j].iov_len > MPIDI_Global.max_send) {
            huge = 1;
            countp_huge += originv[j].iov_len / MPIDI_Global.max_send;
            if (originv[j].iov_len % MPIDI_Global.max_send) {
                countp_huge++;
            }
        } else {
            countp_huge++;
        }
    }

    if (countp_huge > omax && huge) {
        MPL_free(MPIDI_OFI_REQUEST(sreq, noncontig.nopack));
        goto pack;
    }

    if (countp_huge >= 1 && huge) {
        originv_huge =
            MPL_aligned_alloc(iov_align, sizeof(struct iovec) * countp_huge, MPL_MEM_BUFFER);

        for (j = 0; j < num_contig; j++) {
            l = 0;
            if (originv[j].iov_len > MPIDI_Global.max_send) {
                while (l < originv[j].iov_len) {
                    length = originv[j].iov_len - l;
                    if (length > MPIDI_Global.max_send)
                        length = MPIDI_Global.max_send;
                    originv_huge[k].iov_base = (char *) originv[j].iov_base + l;
                    originv_huge[k].iov_len = length;
                    k++;
                    l += length;
                }

            } else {
                originv_huge[k].iov_base = originv[j].iov_base;
                originv_huge[k].iov_len = originv[j].iov_len;
                k++;
            }
        }
    }

    if (huge && k > omax) {
        MPL_free(MPIDI_OFI_REQUEST(sreq, noncontig.nopack));
        MPL_free(originv_huge);
        goto pack;
    }

    if (huge) {
        MPL_free(MPIDI_OFI_REQUEST(sreq, noncontig.nopack));
        MPIDI_OFI_REQUEST(sreq, noncontig.nopack) = originv_huge;
        originv = &(MPIDI_OFI_REQUEST(sreq, noncontig.nopack[cur_o]));
        oout = k;
    }

    MPIDI_OFI_ASSERT_IOVEC_ALIGN(originv);
    msg.msg_iov = originv;
    msg.desc = NULL;
    msg.iov_count = oout;
    msg.tag = match_bits;
    msg.ignore = 0ULL;
    msg.context = (void *) &(MPIDI_OFI_REQUEST(sreq, context));
    msg.data = MPIDI_OFI_ENABLE_DATA ? comm->rank : 0;
    msg.addr = MPIDI_OFI_comm_to_phys(comm, rank);

    MPIDI_OFI_CALL_RETRY(fi_tsendmsg(MPIDI_Global.ctx[0].tx, &msg, flags), tsendv,
                         MPIDI_OFI_CALL_LOCK, FALSE);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_SEND_IOV);
    return mpi_errno;

  pack:
    mpi_errno = MPIDI_OFI_SEND_NEEDS_PACK;
    goto fn_exit;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_send_normal
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_normal(const void *buf, MPI_Aint count,
                                                   MPI_Datatype datatype, int rank, int tag,
                                                   MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                   int dt_contig, size_t data_sz,
                                                   MPIR_Datatype * dt_ptr, MPI_Aint dt_true_lb,
                                                   uint64_t type)
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
    match_bits = MPIDI_OFI_init_sendtag(comm->context_id + context_offset, comm->rank, tag, type);
    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND;
    MPIDI_OFI_REQUEST(sreq, datatype) = datatype;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);

    if (type == MPIDI_OFI_SYNC_SEND) {  /* Branch should compile out */
        int c = 1;
        uint64_t ssend_match, ssend_mask;
        MPIDI_OFI_ssendack_request_t *ackreq;
        ackreq = MPL_malloc(sizeof(MPIDI_OFI_ssendack_request_t), MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP1(ackreq == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem",
                             "**nomem %s", "Ssend ack request alloc");
        ackreq->event_id = MPIDI_OFI_EVENT_SSEND_ACK;
        ackreq->signal_req = sreq;
        MPIR_cc_incr(sreq->cc_ptr, &c);
        ssend_match =
            MPIDI_OFI_init_recvtag(&ssend_mask, comm->context_id + context_offset, rank, tag);
        ssend_match |= MPIDI_OFI_SYNC_SEND_ACK;
        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_Global.ctx[0].rx,   /* endpoint    */
                                      NULL,     /* recvbuf     */
                                      0,        /* data sz     */
                                      NULL,     /* memregion descr  */
                                      MPIDI_OFI_av_to_phys(addr),       /* remote proc */
                                      ssend_match,      /* match bits  */
                                      0ULL,     /* mask bits   */
                                      (void *) &(ackreq->context)), trecvsync, MPIDI_OFI_CALL_LOCK,
                             FALSE);
    }

    send_buf = (char *) buf + dt_true_lb;

    if (!dt_contig) {
        if (MPIDI_OFI_ENABLE_PT2PT_NOPACK && data_sz <= MPIDI_Global.max_send) {
            mpi_errno = MPIDI_OFI_send_iov(buf, count, rank, match_bits, comm, sreq, dt_ptr);
            if (mpi_errno == MPI_SUCCESS)       /* Send posted using iov */
                goto fn_exit;
            else if (mpi_errno != MPIDI_OFI_SEND_NEEDS_PACK)
                goto fn_fail;
            /* send_iov returned MPIDI_OFI_SEND_NEEDS_PACK -- indicating
             * that there was no error but it couldn't initiate the send
             * due to iov limitations. We need to fall back to the pack
             * path below. Simply falling through. */
            mpi_errno = MPI_SUCCESS;    /* Reset error code */
        }
        /* Pack */
        MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND_PACK;

        MPIDI_OFI_REQUEST(sreq, noncontig.pack) =
            (MPIDI_OFI_pack_t *) MPL_malloc(data_sz + sizeof(MPIR_Segment), MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP1(MPIDI_OFI_REQUEST(sreq, noncontig.pack) == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send Pack buffer alloc");
        size_t segment_first;
        segment_first = 0;
        last = data_sz;

        MPIR_Segment_init(buf, count, datatype, &MPIDI_OFI_REQUEST(sreq, noncontig.pack->segment));
        MPIR_Segment_pack(&MPIDI_OFI_REQUEST(sreq, noncontig.pack->segment), segment_first, &last,
                          MPIDI_OFI_REQUEST(sreq, noncontig.pack->pack_buffer));
        send_buf = MPIDI_OFI_REQUEST(sreq, noncontig.pack->pack_buffer);
    } else {
        MPIDI_OFI_REQUEST(sreq, noncontig.pack) = NULL;
        MPIDI_OFI_REQUEST(sreq, noncontig.nopack) = NULL;
    }

    if (data_sz <= MPIDI_Global.max_buffered_send) {
        mpi_errno =
            MPIDI_OFI_send_handler(MPIDI_Global.ctx[0].tx, send_buf, data_sz, NULL, comm->rank,
                                   MPIDI_OFI_av_to_phys(addr),
                                   match_bits, NULL, MPIDI_OFI_DO_INJECT, MPIDI_OFI_CALL_LOCK,
                                   FALSE);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIDI_OFI_send_event(NULL, sreq, MPIDI_OFI_REQUEST(sreq, event_id));
    } else if (data_sz <= MPIDI_Global.max_send) {
        mpi_errno =
            MPIDI_OFI_send_handler(MPIDI_Global.ctx[0].tx, send_buf, data_sz, NULL, comm->rank,
                                   MPIDI_OFI_av_to_phys(addr),
                                   match_bits, (void *) &(MPIDI_OFI_REQUEST(sreq, context)),
                                   MPIDI_OFI_DO_SEND, MPIDI_OFI_CALL_LOCK, FALSE);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else if (unlikely(1)) {
        MPIDI_OFI_send_control_t ctrl;
        int c;
        uint64_t rma_key = 0;
        struct fid_mr *huge_send_mr;

        c = 1;
        MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_SEND_HUGE;
        MPIR_cc_incr(sreq->cc_ptr, &c);

        MPID_THREAD_CS_ENTER(POBJ, MPIDI_OFI_THREAD_FI_MUTEX);

        if (MPIDI_OFI_ENABLE_MR_SCALABLE) {
            /* Set up a memory region for the lmt data transfer */
            ctrl.rma_key =
                MPIDI_OFI_index_allocator_alloc(MPIDI_OFI_COMM(comm).rma_id_allocator, MPL_MEM_RMA);
            MPIR_Assert(ctrl.rma_key < MPIDI_Global.max_huge_rmas);
            rma_key = MPIDI_OFI_rma_key_pack(comm->context_id, MPIDI_OFI_KEY_TYPE_HUGE_RMA,
                                             ctrl.rma_key);
            ctrl.rma_key = rma_key;
        }

        MPIDI_OFI_CALL_NOLOCK(fi_mr_reg(MPIDI_Global.domain,    /* In:  Domain Object       */
                                        send_buf,       /* In:  Lower memory address */
                                        data_sz,        /* In:  Length              */
                                        FI_REMOTE_READ, /* In:  Expose MR for read  */
                                        0ULL,   /* In:  offset(not used)    */
                                        rma_key,        /* In:  requested key       */
                                        0ULL,   /* In:  flags               */
                                        &huge_send_mr,  /* Out: memregion object    */
                                        NULL), mr_reg); /* In:  context             */

        /* Create map to the memory region */
        MPIDI_CH4U_map_set(MPIDI_OFI_COMM(comm).huge_send_counters, sreq->handle, huge_send_mr,
                           MPL_MEM_BUFFER);

        if (!MPIDI_OFI_ENABLE_MR_SCALABLE) {
            /* MR_BASIC */
            ctrl.rma_key = fi_mr_key(huge_send_mr);
        }

        /* Send the maximum amount of data that we can here to get things
         * started, then do the rest using the MR below. This can be confirmed
         * in the MPIDI_OFI_get_huge code where we start the offset at
         * MPIDI_Global.max_send */
        MPIDI_OFI_REQUEST(sreq, util_comm) = comm;
        MPIDI_OFI_REQUEST(sreq, util_id) = rank;
        mpi_errno = MPIDI_OFI_send_handler(MPIDI_Global.ctx[0].tx, send_buf,
                                           MPIDI_Global.max_send,
                                           NULL,
                                           comm->rank,
                                           MPIDI_OFI_av_to_phys(addr),
                                           match_bits,
                                           (void *) &(MPIDI_OFI_REQUEST(sreq, context)),
                                           MPIDI_OFI_DO_SEND, MPIDI_OFI_CALL_NO_LOCK, FALSE);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        ctrl.type = MPIDI_OFI_CTRL_HUGE;
        ctrl.seqno = 0;
        ctrl.tag = tag;

        /* Send information about the memory region here to get the lmt going. */
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
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                            int rank, int tag, MPIR_Comm * comm, int context_offset,
                                            MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                            int noreq, uint64_t syncflag)
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
                                                   rank, tag, comm, context_offset, addr);
        else
            mpi_errno = MPIDI_OFI_send_lightweight_request((char *) buf + dt_true_lb, data_sz,
                                                           rank, tag, comm, context_offset,
                                                           addr, request);
    else
        mpi_errno = MPIDI_OFI_send_normal(buf, count, datatype, rank, tag, comm,
                                          context_offset, addr, request, dt_contig,
                                          data_sz, dt_ptr, dt_true_lb, syncflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_SEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_send(const void *buf, MPI_Aint count,
                                               MPI_Datatype datatype, int rank, int tag,
                                               MPIR_Comm * comm, int context_offset,
                                               MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SEND);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno =
            MPIDIG_mpi_send(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_send(buf, count, datatype, rank, tag, comm,
                               context_offset, addr, request, 1, 0ULL);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ssend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ssend(const void *buf, MPI_Aint count,
                                                MPI_Datatype datatype, int rank, int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SSEND);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno =
            MPIDIG_mpi_ssend(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_send(buf, count, datatype, rank, tag, comm,
                               context_offset, addr, request, 0, MPIDI_OFI_SYNC_SEND);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SSEND);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_isend(const void *buf, MPI_Aint count,
                                                MPI_Datatype datatype, int rank, int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISEND);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno =
            MPIDIG_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_send(buf, count, datatype, rank, tag, comm,
                               context_offset, addr, request, 0, 0ULL);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_issend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_issend(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISSEND);

    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno =
            MPIDIG_mpi_issend(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
        goto fn_exit;
    }

    mpi_errno = MPIDI_OFI_send(buf, count, datatype, rank, tag, comm,
                               context_offset, addr, request, 0, MPIDI_OFI_SYNC_SEND);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISSEND);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_send_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_send_init(const void *buf, int count,
                                                    MPI_Datatype datatype, int rank, int tag,
                                                    MPIR_Comm * comm, int context_offset,
                                                    MPIDI_av_entry_t * addr,
                                                    MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SEND_INIT);

    mpi_errno =
        MPIDIG_mpi_send_init(buf, count, datatype, rank, tag, comm, context_offset, addr, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SEND_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ssend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ssend_init(const void *buf, int count,
                                                     MPI_Datatype datatype, int rank, int tag,
                                                     MPIR_Comm * comm, int context_offset,
                                                     MPIDI_av_entry_t * addr,
                                                     MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SSEND_INIT);

    mpi_errno =
        MPIDIG_mpi_ssend_init(buf, count, datatype, rank, tag, comm, context_offset, addr, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SSEND_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_bsend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_bsend_init(const void *buf, int count,
                                                     MPI_Datatype datatype, int rank, int tag,
                                                     MPIR_Comm * comm, int context_offset,
                                                     MPIDI_av_entry_t * addr,
                                                     MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_BSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_BSEND_INIT);

    mpi_errno =
        MPIDIG_mpi_bsend_init(buf, count, datatype, rank, tag, comm, context_offset, addr, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_BSEND_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_rsend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rsend_init(const void *buf, int count,
                                                     MPI_Datatype datatype, int rank, int tag,
                                                     MPIR_Comm * comm, int context_offset,
                                                     MPIDI_av_entry_t * addr,
                                                     MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RSEND_INIT);

    mpi_errno =
        MPIDIG_mpi_rsend_init(buf, count, datatype, rank, tag, comm, context_offset, addr, request);
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
