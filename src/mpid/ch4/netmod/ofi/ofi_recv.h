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

#define MPIDI_OFI_RECV_NEEDS_UNPACK (-1)
/*
  Tries to post receive for non-contiguous datatype using iovec.

  Return value:
  MPI_SUCCESS: Message was sent successfully using iovec
  MPIDI_OFI_RECV_NEEDS_UNPACK: There was no error but receive was not posted
      due to limitations with iovec. Needs to fall back to the unpack path.
  Other: An error occurred as indicated in the code.
*/
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_recv_iov(void *buf, MPI_Aint count, size_t data_sz,      /* data_sz passed in here for reusing */
                                                int rank, uint64_t match_bits, uint64_t mask_bits,
                                                MPIR_Comm * comm, MPIR_Context_id_t context_id,
                                                MPIDI_av_entry_t * addr, MPIR_Request * rreq,
                                                MPIR_Datatype * dt_ptr, uint64_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    struct iovec *originv = NULL, *originv_huge = NULL;
    size_t max_pipe = SIZE_MAX;
    size_t omax = MPIDI_OFI_global.rx_iov_limit;
    size_t countp =
        MPIDI_OFI_count_iov(count, MPIDI_OFI_REQUEST(rreq, datatype), data_sz, max_pipe);
    size_t o_size = sizeof(struct iovec);
    unsigned map_size;
    int num_contig, size, j = 0, k = 0, huge = 0, length = 0;
    size_t l = 0;
    size_t countp_huge = 0;
    size_t oout = 0;
    size_t cur_o = 0;
    struct fi_msg_tagged msg;
    size_t iov_align = MPL_MAX(MPIDI_OFI_IOVEC_ALIGN, sizeof(void *));
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_RECV_IOV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_RECV_IOV);

    /* If the number of iovecs is greater than the supported hardware limit (to transfer in a single send),
     *  fallback to the pack path */
    if (countp > omax) {
        goto unpack;
    }

    if (!flags) {
        flags = FI_COMPLETION | FI_REMOTE_CQ_DATA;
    }

    map_size = dt_ptr->max_contig_blocks * count + 1;
    num_contig = map_size;      /* map_size is the maximum number of iovecs that can be generated */

    size = o_size * num_contig + sizeof(*(MPIDI_OFI_REQUEST(rreq, noncontig.nopack)));

    MPIDI_OFI_REQUEST(rreq, noncontig.nopack) = MPL_aligned_alloc(iov_align, size, MPL_MEM_BUFFER);
    memset(MPIDI_OFI_REQUEST(rreq, noncontig.nopack), 0, size);

    int actual_iov_len;
    MPI_Aint actual_iov_bytes;
    MPIR_Typerep_to_iov(buf, count, MPIDI_OFI_REQUEST(rreq, datatype), 0,
                        MPIDI_OFI_REQUEST(rreq, noncontig.nopack), num_contig, dt_ptr->size * count,
                        &actual_iov_len, &actual_iov_bytes);
    num_contig = actual_iov_len;

    originv = &(MPIDI_OFI_REQUEST(rreq, noncontig.nopack[cur_o]));
    oout = num_contig;  /* num_contig is the actual number of iovecs returned by the Segment_pack_vector function */

    if (oout > omax) {
        MPL_free(MPIDI_OFI_REQUEST(rreq, noncontig.nopack));
        goto unpack;
    }

    /* check if the length of any iovec in the current iovec array exceeds the huge message threshold
     * and calculate the total number of iovecs */
    for (j = 0; j < num_contig; j++) {
        if (originv[j].iov_len > MPIDI_OFI_global.max_msg_size) {
            huge = 1;
            countp_huge += originv[j].iov_len / MPIDI_OFI_global.max_msg_size;
            if (originv[j].iov_len % MPIDI_OFI_global.max_msg_size) {
                countp_huge++;
            }
        } else {
            countp_huge++;
        }
    }

    if (countp_huge > omax && huge) {
        MPL_free(MPIDI_OFI_REQUEST(rreq, noncontig.nopack));
        goto unpack;
    }

    if (countp_huge >= 1 && huge) {
        originv_huge =
            MPL_aligned_alloc(iov_align, sizeof(struct iovec) * countp_huge, MPL_MEM_BUFFER);
        MPIR_Assert(originv_huge != NULL);
        for (j = 0; j < num_contig; j++) {
            l = 0;
            if (originv[j].iov_len > MPIDI_OFI_global.max_msg_size) {
                while (l < originv[j].iov_len) {
                    length = originv[j].iov_len - l;
                    if (length > MPIDI_OFI_global.max_msg_size)
                        length = MPIDI_OFI_global.max_msg_size;
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
        MPL_free(MPIDI_OFI_REQUEST(rreq, noncontig.nopack));
        MPL_free(originv_huge);
        goto unpack;
    }

    if (huge) {
        MPL_free(MPIDI_OFI_REQUEST(rreq, noncontig.nopack));
        MPIDI_OFI_REQUEST(rreq, noncontig.nopack) = originv_huge;
        originv = &(MPIDI_OFI_REQUEST(rreq, noncontig.nopack[cur_o]));
        oout = k;
    }

    MPIDI_OFI_REQUEST(rreq, util_comm) = comm;
    MPIDI_OFI_REQUEST(rreq, util_id) = context_id;

    MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV_NOPACK;

    MPIDI_OFI_ASSERT_IOVEC_ALIGN(originv);
    msg.msg_iov = originv;
    msg.desc = NULL;
    msg.iov_count = oout;
    msg.tag = match_bits;
    msg.ignore = mask_bits;
    msg.context = (void *) &(MPIDI_OFI_REQUEST(rreq, context));
    msg.data = 0;
    msg.addr = (MPI_ANY_SOURCE == rank) ? FI_ADDR_UNSPEC : MPIDI_OFI_av_to_phys(addr);

    MPIDI_OFI_CALL_RETRY(fi_trecvmsg(MPIDI_OFI_global.ctx[0].rx, &msg, flags), trecv, FALSE);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_RECV_IOV);
    return mpi_errno;

  unpack:
    mpi_errno = MPIDI_OFI_RECV_NEEDS_UNPACK;
    goto fn_exit;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_irecv(void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm,
                                                int context_offset,
                                                MPIDI_av_entry_t * addr,
                                                MPIR_Request ** request, int mode, uint64_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = *request;
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
        MPIDI_OFI_REQUEST_CREATE_CONDITIONAL(rreq, MPIR_REQUEST_KIND__RECV);
        /* Need to set the source to UNDEFINED for anysource matching */
        rreq->status.MPI_SOURCE = MPI_UNDEFINED;
    } else if (mode == MPIDI_OFI_USE_EXISTING) {
        rreq = *request;
    } else {
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nullptr");
        goto fn_fail;
    }

    *request = rreq;

    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, context_id, tag);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    MPIDI_OFI_REQUEST(rreq, datatype) = datatype;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);

    recv_buf = (char *) buf + dt_true_lb;

    if (!dt_contig) {
        if (MPIDI_OFI_ENABLE_PT2PT_NOPACK && data_sz <= MPIDI_OFI_global.max_msg_size) {
            mpi_errno =
                MPIDI_OFI_recv_iov(buf, count, data_sz, rank, match_bits, mask_bits, comm,
                                   context_id, addr, rreq, dt_ptr, flags);
            if (mpi_errno == MPI_SUCCESS)       /* Receive posted using iov */
                goto fn_exit;
            else if (mpi_errno != MPIDI_OFI_RECV_NEEDS_UNPACK)
                goto fn_fail;
            /* recv_iov returned MPIDI_OFI_RECV_NEEDS_UNPACK -- indicating
             * that there was no error but it couldn't post the receive
             * due to iov limitations. We need to fall back to the unpack
             * path below. Simply falling through. */
            mpi_errno = MPI_SUCCESS;    /* Reset error code */
        }
        /* Unpack */
        MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV_PACK;
        MPIDI_OFI_REQUEST(rreq, noncontig.pack) =
            (MPIDI_OFI_pack_t *) MPL_malloc(data_sz + sizeof(MPIDI_OFI_pack_t), MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP1(MPIDI_OFI_REQUEST(rreq, noncontig.pack->pack_buffer) == NULL,
                             mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "Recv Pack Buffer alloc");
        recv_buf = MPIDI_OFI_REQUEST(rreq, noncontig.pack->pack_buffer);
        MPIDI_OFI_REQUEST(rreq, noncontig.pack->buf) = buf;
        MPIDI_OFI_REQUEST(rreq, noncontig.pack->count) = count;
        MPIDI_OFI_REQUEST(rreq, noncontig.pack->datatype) = datatype;
    } else {
        MPIDI_OFI_REQUEST(rreq, noncontig.pack) = NULL;
        MPIDI_OFI_REQUEST(rreq, noncontig.nopack) = NULL;
    }

    MPIDI_OFI_REQUEST(rreq, util_comm) = comm;
    MPIDI_OFI_REQUEST(rreq, util_id) = context_id;

    if (unlikely(data_sz > MPIDI_OFI_global.max_msg_size)) {
        MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV_HUGE;
        data_sz = MPIDI_OFI_global.max_msg_size;
    } else if (MPIDI_OFI_REQUEST(rreq, event_id) != MPIDI_OFI_EVENT_RECV_PACK)
        MPIDI_OFI_REQUEST(rreq, event_id) = MPIDI_OFI_EVENT_RECV;

    if (!flags) /* Branch should compile out */
        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[0].rx,
                                      recv_buf,
                                      data_sz,
                                      NULL,
                                      (MPI_ANY_SOURCE ==
                                       rank) ? FI_ADDR_UNSPEC : MPIDI_OFI_av_to_phys(addr),
                                      match_bits, mask_bits,
                                      (void *) &(MPIDI_OFI_REQUEST(rreq, context))), trecv, FALSE);
    else {
        MPIDI_OFI_request_util_iov(rreq)->iov_base = recv_buf;
        MPIDI_OFI_request_util_iov(rreq)->iov_len = data_sz;

        MPIDI_OFI_ASSERT_IOVEC_ALIGN(MPIDI_OFI_request_util_iov(rreq));
        msg.msg_iov = MPIDI_OFI_request_util_iov(rreq);
        msg.desc = NULL;
        msg.iov_count = 1;
        msg.tag = match_bits;
        msg.ignore = mask_bits;
        msg.context = (void *) &(MPIDI_OFI_REQUEST(rreq, context));
        msg.data = 0;
        msg.addr = FI_ADDR_UNSPEC;

        MPIDI_OFI_CALL_RETRY(fi_trecvmsg(MPIDI_OFI_global.ctx[0].rx, &msg, flags), trecv, FALSE);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_IRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_recv(void *buf,
                                               MPI_Aint count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag,
                                               MPIR_Comm * comm,
                                               int context_offset, MPIDI_av_entry_t * addr,
                                               MPI_Status * status, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RECV);

#ifdef MPIDI_ENABLE_LEGACY_OFI
    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno =
            MPIDIG_mpi_recv(buf, count, datatype, rank, tag, comm, context_offset, status, request);
    } else
#endif
    {
        mpi_errno = MPIDI_OFI_do_irecv(buf, count, datatype, rank, tag, comm,
                                       context_offset, addr, request, MPIDI_OFI_ON_HEAP, 0ULL);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RECV);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_imrecv(void *buf,
                                                 MPI_Aint count,
                                                 MPI_Datatype datatype, MPIR_Request * message)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDI_av_entry_t *av;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IMRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IMRECV);

#ifdef MPIDI_ENABLE_LEGACY_OFI
    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_imrecv(buf, count, datatype, message);
    } else
#endif
    {
        rreq = message;

        av = MPIDIU_comm_rank_to_av(rreq->comm, message->status.MPI_SOURCE);
        mpi_errno = MPIDI_OFI_do_irecv(buf, count, datatype, message->status.MPI_SOURCE,
                                       message->status.MPI_TAG, rreq->comm, 0, av,
                                       &rreq, MPIDI_OFI_USE_EXISTING, FI_CLAIM | FI_COMPLETION);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IMRECV);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_irecv(void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IRECV);

#ifdef MPIDI_ENABLE_LEGACY_OFI
    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno =
            MPIDIG_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, request);
    } else
#endif
    {
        mpi_errno = MPIDI_OFI_do_irecv(buf, count, datatype, rank, tag, comm,
                                       context_offset, addr, request, MPIDI_OFI_ON_HEAP, 0ULL);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IRECV);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_recv(MPIR_Request * rreq)
{

    int mpi_errno = MPI_SUCCESS;
    ssize_t ret;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_CANCEL_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_CANCEL_RECV);

#ifdef MPIDI_ENABLE_LEGACY_OFI
    if (!MPIDI_OFI_ENABLE_TAGGED) {
        mpi_errno = MPIDIG_mpi_cancel_recv(rreq);
        goto fn_exit;
    }
#endif

    ret = fi_cancel((fid_t) MPIDI_OFI_global.ctx[0].rx, &(MPIDI_OFI_REQUEST(rreq, context)));

    if (ret == 0) {
        while ((!MPIR_STATUS_GET_CANCEL_BIT(rreq->status)) && (!MPIR_cc_is_complete(&rreq->cc))) {
            /* The cancel is local and must complete, so only poll this device (not global progress) */
            if ((mpi_errno = MPIDI_OFI_progress(0, 0)) != MPI_SUCCESS)
                goto fn_exit;
        }

        if (MPIR_STATUS_GET_CANCEL_BIT(rreq->status)) {
            MPIR_STATUS_SET_CANCEL_BIT(rreq->status, TRUE);
            MPIR_STATUS_SET_COUNT(rreq->status, 0);
            MPIDIU_request_complete(rreq);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_CANCEL_RECV);
    return mpi_errno;
}

#endif /* OFI_RECV_H_INCLUDED */
