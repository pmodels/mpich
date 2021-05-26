/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPID_RMA_ISSUE_H_INCLUDED
#define MPID_RMA_ISSUE_H_INCLUDED

#include "utlist.h"
#include "mpid_rma_types.h"

/* =========================================================== */
/*                    auxiliary functions                      */
/* =========================================================== */

/* immed_copy() copies data from origin buffer to
   IMMED packet header. */
static inline int immed_copy(void *src, void *dest, size_t len)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_IMMED_COPY);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_IMMED_COPY);

    if (src == NULL || dest == NULL || len == 0)
        goto fn_exit;

    switch (len) {
    case 1:
        *(uint8_t *) dest = *(uint8_t *) src;
        break;
#ifndef NEEDS_STRICT_ALIGNMENT
        /* Following copy is unsafe on platforms that require strict
         * alignment (e.g., SPARC). Because the buffers may not be aligned
         * for data type access except char. */
    case 2:
        *(uint16_t *) dest = *(uint16_t *) src;
        break;
    case 4:
        *(uint32_t *) dest = *(uint32_t *) src;
        break;
    case 8:
        *(uint64_t *) dest = *(uint64_t *) src;
        break;
#endif
    default:
        MPIR_Memcpy(dest, (void *) src, len);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_IMMED_COPY);
    return mpi_errno;
}

/* =========================================================== */
/*                  extended packet functions                  */
/* =========================================================== */

/* Set extended header for ACC operation and return its real size. */
static int init_stream_dtype_ext_pkt(int pkt_flags,
                              MPIR_Datatype* target_dtp, intptr_t stream_offset,
                              void **ext_hdr_ptr, MPI_Aint * ext_hdr_sz, int *flattened_type_size)
{
    MPI_Aint _total_sz = 0, stream_hdr_sz = 0;
    void *flattened_type, *total_hdr;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_INIT_ACCUM_EXT_PKT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_INIT_ACCUM_EXT_PKT);

    /*
     * The extended header consists of two parts:
     *
     *  1. Stream header: if the size of the data is large and needs
     *  to be chunked into multiple pieces.
     *
     *  2. Flattened datatype: if the target is a derived datatype.
     */

    if ((pkt_flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM))
        stream_hdr_sz = sizeof(MPIDI_CH3_Ext_pkt_stream_t);
    else
        stream_hdr_sz = 0;

    if (target_dtp != NULL)
        MPIR_Typerep_flatten_size(target_dtp, flattened_type_size);
    else
        *flattened_type_size = 0;

    _total_sz = stream_hdr_sz + *flattened_type_size;
    if (_total_sz) {
        total_hdr = MPL_malloc(_total_sz, MPL_MEM_RMA);
        if (total_hdr == NULL) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem",
                                 "**nomem %d", _total_sz);
        }
        MPL_VG_MEM_INIT(total_hdr, _total_sz);
    }
    else {
        total_hdr = NULL;
    }

    if ((pkt_flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM)) {
        ((MPIDI_CH3_Ext_pkt_stream_t *) total_hdr)->stream_offset = stream_offset;
    }
    if (target_dtp != NULL) {
        flattened_type = (void *) ((char *) total_hdr + stream_hdr_sz);
        MPIR_Typerep_flatten(target_dtp, flattened_type);
    }

    (*ext_hdr_ptr) = total_hdr;
    (*ext_hdr_sz) = _total_sz;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_INIT_ACCUM_EXT_PKT);
    return mpi_errno;
  fn_fail:
    MPL_free((*ext_hdr_ptr));
    (*ext_hdr_ptr) = NULL;
    (*ext_hdr_sz) = 0;
    goto fn_exit;
}

/* =========================================================== */
/*                      issuinng functions                     */
/* =========================================================== */

/* issue_from_origin_buffer() issues data from origin
   buffer (i.e. non-IMMED operation). */
static int issue_from_origin_buffer(MPIDI_RMA_Op_t * rma_op, MPIDI_VC_t * vc,
                                    void *ext_hdr_ptr, MPI_Aint ext_hdr_sz,
                                    intptr_t stream_offset, intptr_t stream_size,
                                    MPIR_Request ** req_ptr)
{
    MPI_Datatype target_datatype;
    MPIR_Datatype*target_dtp = NULL, *origin_dtp = NULL;
    int is_origin_contig;
    struct iovec iov[MPL_IOV_LIMIT];
    int iovcnt = 0;
    MPIR_Request *req = NULL;
    MPI_Aint dt_true_lb;
    int pkt_flags;
    int is_empty_origin = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ISSUE_FROM_ORIGIN_BUFFER);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_ISSUE_FROM_ORIGIN_BUFFER);

    /* Judge if origin buffer is empty (this can only happens for
     * GACC and FOP when op is MPI_NO_OP). */
    if ((rma_op->pkt).type == MPIDI_CH3_PKT_GET_ACCUM || (rma_op->pkt).type == MPIDI_CH3_PKT_FOP) {
        MPI_Op op;
        MPIDI_CH3_PKT_RMA_GET_OP(rma_op->pkt, op, mpi_errno);
        if (op == MPI_NO_OP)
            is_empty_origin = TRUE;
    }

    /* Judge if target datatype is derived datatype. */
    MPIDI_CH3_PKT_RMA_GET_TARGET_DATATYPE(rma_op->pkt, target_datatype, mpi_errno);
    if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
        MPIR_Datatype_get_ptr(target_datatype, target_dtp);
    }

    if (is_empty_origin == FALSE) {
        /* Judge if origin datatype is derived datatype. */
        if (!MPIR_DATATYPE_IS_PREDEFINED(rma_op->origin_datatype)) {
            MPIR_Datatype_get_ptr(rma_op->origin_datatype, origin_dtp);
        }

        /* check if origin data is contiguous and get true lb */
        MPIR_Datatype_is_contig(rma_op->origin_datatype, &is_origin_contig);
        MPIR_Datatype_get_true_lb(rma_op->origin_datatype, &dt_true_lb);
    }
    else {
        /* origin buffer is empty, mark origin data as contig and true_lb as 0. */
        is_origin_contig = 1;
        dt_true_lb = 0;
    }

    iov[iovcnt].iov_base = (void *) & (rma_op->pkt);
    iov[iovcnt].iov_len = sizeof(rma_op->pkt);
    iovcnt++;

    MPIDI_CH3_PKT_RMA_GET_FLAGS(rma_op->pkt, pkt_flags, mpi_errno);
    if (!(pkt_flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) && target_dtp == NULL && is_origin_contig) {
        /* Fast path --- use iStartMsgv() to issue the data, which does not need a request
         * to be passed in:
         * (1) non-streamed op (do not need to send extended packet header);
         * (2) target datatype is predefined (do not need to send derived datatype info);
         * (3) origin datatype is contiguous (do not need to pack the data and send);
         */

        if (is_empty_origin == FALSE) {
            iov[iovcnt].iov_base =
                (void *) ((char *) rma_op->origin_addr + dt_true_lb + stream_offset);
            iov[iovcnt].iov_len = stream_size;
            iovcnt++;
        }

        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, &req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

        if (origin_dtp != NULL) {
            if (req == NULL) {
                MPIR_Datatype_ptr_release(origin_dtp);
            }
            else {
                /* this will cause the datatype to be freed when the request
                 * is freed. */
                req->dev.datatype_ptr = origin_dtp;
            }
        }

        goto fn_exit;
    }

    /* Normal path: use iSendv() and sendNoncontig_fn() to issue the data, which
     * always need a request to be passed in. */

    /* create a new request */
    req = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
    MPIR_ERR_CHKANDJUMP(req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    MPIR_Object_set_ref(req, 2);

    /* set extended packet header, it is freed when the request is freed.  */
    if (ext_hdr_sz > 0) {
        req->dev.ext_hdr_sz = ext_hdr_sz;
        req->dev.ext_hdr_ptr = ext_hdr_ptr;
        req->dev.flattened_type = NULL;

        iov[iovcnt].iov_base = (void *) req->dev.ext_hdr_ptr;
        iov[iovcnt].iov_len = ext_hdr_sz;
        iovcnt++;
    }

    if (origin_dtp != NULL) {
        req->dev.datatype_ptr = origin_dtp;
        /* this will cause the datatype to be freed when the request
         * is freed. */
    }

    if (is_origin_contig) {
        /* origin data is contiguous */
        if (is_empty_origin == FALSE) {
            iov[iovcnt].iov_base =
                (void *) ((char *) rma_op->origin_addr + dt_true_lb + stream_offset);
            iov[iovcnt].iov_len = stream_size;
            iovcnt++;
        }

        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iSendv(vc, req, iov, iovcnt);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }
    else {
        /* origin data is non-contiguous */
        req->dev.user_buf = rma_op->origin_addr;
        req->dev.user_count = rma_op->origin_count;
        req->dev.datatype = rma_op->origin_datatype;
        req->dev.msg_offset = stream_offset;
        req->dev.msgsize = stream_offset + stream_size;

        req->dev.OnFinal = 0;
        req->dev.OnDataAvail = 0;

        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = vc->sendNoncontig_fn(vc, req, iov[0].iov_base, iov[0].iov_len,
                                         &iov[1], iovcnt - 1);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

  fn_exit:
    /* release the target datatype */
    if (target_dtp)
        MPIR_Datatype_ptr_release(target_dtp);
    (*req_ptr) = req;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_ISSUE_FROM_ORIGIN_BUFFER);
    return mpi_errno;
  fn_fail:
    if (req) {
        if (req->dev.datatype_ptr)
            MPIR_Datatype_ptr_release(req->dev.datatype_ptr);
        MPL_free(req->dev.ext_hdr_ptr);
        MPIR_Request_free(req);
    }

    (*req_ptr) = NULL;
    goto fn_exit;
}


/* issue_put_op() issues PUT packet header and data. */
static int issue_put_op(MPIDI_RMA_Op_t * rma_op, MPIR_Win * win_ptr,
                        MPIDI_RMA_Target_t * target_ptr, int pkt_flags)
{
    MPIDI_VC_t *vc = NULL;
    MPIR_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_CH3_Pkt_put_t *put_pkt = &rma_op->pkt.put;
    MPIR_Request *curr_req = NULL;
    MPI_Datatype target_datatype;
    MPIR_Datatype*target_dtp_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ISSUE_PUT_OP);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_ISSUE_PUT_OP);

    put_pkt->pkt_flags |= pkt_flags;

    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    if (rma_op->pkt.type == MPIDI_CH3_PKT_PUT_IMMED) {
        /* All origin data is in packet header, issue the header. */
        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, put_pkt, sizeof(*put_pkt), &curr_req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }
    else {
        MPI_Aint origin_type_size;
        void *ext_hdr_ptr = NULL;
        MPI_Aint ext_hdr_sz = 0;
        MPIR_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);

        /* If derived datatype on target, add extended packet header. */
        MPIDI_CH3_PKT_RMA_GET_TARGET_DATATYPE(rma_op->pkt, target_datatype, mpi_errno);
        if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
            MPIR_Datatype_get_ptr(target_datatype, target_dtp_ptr);
            MPIR_Typerep_flatten_size(target_dtp_ptr, &put_pkt->info.flattened_type_size);

            ext_hdr_ptr = MPL_malloc(put_pkt->info.flattened_type_size, MPL_MEM_RMA);
            if (ext_hdr_ptr == NULL) {
                MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem",
                                     "**nomem %d", put_pkt->info.flattened_type_size);
            }
            MPL_VG_MEM_INIT(ext_hdr_ptr, put_pkt->info.flattened_type_size);

            MPIR_Typerep_flatten(target_dtp_ptr, ext_hdr_ptr);
            ext_hdr_sz = put_pkt->info.flattened_type_size;
        }

        mpi_errno = issue_from_origin_buffer(rma_op, vc, ext_hdr_ptr, ext_hdr_sz,
                                             0, rma_op->origin_count * origin_type_size, &curr_req);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (curr_req != NULL) {
        rma_op->reqs_size = 1;

        rma_op->single_req = curr_req;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_ISSUE_PUT_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    rma_op->single_req = NULL;
    rma_op->reqs_size = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#define ALL_STREAM_UNITS_ISSUED (-1)

/* issue_acc_op() send ACC packet header and data. */
static int issue_acc_op(MPIDI_RMA_Op_t * rma_op, MPIR_Win * win_ptr,
                        MPIDI_RMA_Target_t * target_ptr, int pkt_flags)
{
    MPIDI_VC_t *vc = NULL;
    MPIR_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_CH3_Pkt_accum_t *accum_pkt = &rma_op->pkt.accum;
    int i, j;
    MPI_Aint stream_elem_count, stream_unit_count;
    MPI_Aint predefined_dtp_size, predefined_dtp_extent, predefined_dtp_count;
    MPI_Aint total_len, rest_len;
    MPI_Aint origin_dtp_size;
    MPIR_Datatype*origin_dtp_ptr = NULL;
    MPIR_Datatype*target_dtp_ptr = NULL;
    void *ext_hdr_ptr = NULL;
    MPI_Aint ext_hdr_sz = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ISSUE_ACC_OP);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_ISSUE_ACC_OP);

    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    if (rma_op->pkt.type == MPIDI_CH3_PKT_ACCUMULATE_IMMED) {
        MPIR_Request *curr_req = NULL;

        accum_pkt->pkt_flags |= pkt_flags;

        /* All origin data is in packet header, issue the header. */
        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, accum_pkt, sizeof(*accum_pkt), &curr_req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

        if (curr_req != NULL) {
            MPIR_Assert(rma_op->reqs_size == 0 && rma_op->single_req == NULL);

            rma_op->reqs_size = 1;
            rma_op->single_req = curr_req;
        }
        goto fn_exit;
    }

    /* Get total length of origin data */
    MPIR_Datatype_get_size_macro(rma_op->origin_datatype, origin_dtp_size);
    total_len = origin_dtp_size * rma_op->origin_count;

    /* Get size and count for predefined datatype elements */
    if (MPIR_DATATYPE_IS_PREDEFINED(rma_op->origin_datatype)) {
        predefined_dtp_size = origin_dtp_size;
        predefined_dtp_count = rma_op->origin_count;
        MPIR_Datatype_get_extent_macro(rma_op->origin_datatype, predefined_dtp_extent);
    }
    else {
        MPIR_Datatype_get_ptr(rma_op->origin_datatype, origin_dtp_ptr);
        MPIR_Assert(origin_dtp_ptr != NULL && origin_dtp_ptr->basic_type != MPI_DATATYPE_NULL);
        MPIR_Datatype_get_size_macro(origin_dtp_ptr->basic_type, predefined_dtp_size);
        predefined_dtp_count = total_len / predefined_dtp_size;
        MPIR_Datatype_get_extent_macro(origin_dtp_ptr->basic_type, predefined_dtp_extent);
    }
    MPIR_Assert(predefined_dtp_count > 0 && predefined_dtp_size > 0 && predefined_dtp_extent > 0);

    /* Calculate number of predefined elements in each stream unit, and
     * total number of stream units. */
    stream_elem_count = MPIDI_CH3U_Acc_stream_size / predefined_dtp_extent;
    stream_unit_count = (predefined_dtp_count - 1) / stream_elem_count + 1;
    MPIR_Assert(stream_elem_count > 0 && stream_unit_count > 0);

    /* If there are more than one stream unit, mark the current packet
     * as stream packet */
    if (stream_unit_count > 1)
        pkt_flags |= MPIDI_CH3_PKT_FLAG_RMA_STREAM;

    /* Get target datatype */
    if (!MPIR_DATATYPE_IS_PREDEFINED(accum_pkt->datatype))
        MPIR_Datatype_get_ptr(accum_pkt->datatype, target_dtp_ptr);

    rest_len = total_len;
    MPIR_Assert(rma_op->issued_stream_count >= 0);
    for (j = 0; j < stream_unit_count; j++) {
        intptr_t stream_offset, stream_size;
        MPIR_Request *curr_req = NULL;

        if (j < rma_op->issued_stream_count)
            continue;

        accum_pkt->pkt_flags |= pkt_flags;

        if (j != 0) {
            accum_pkt->pkt_flags &= ~MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED;
            accum_pkt->pkt_flags &= ~MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE;
        }
        if (j != stream_unit_count - 1) {
            accum_pkt->pkt_flags &= ~MPIDI_CH3_PKT_FLAG_RMA_UNLOCK;
            accum_pkt->pkt_flags &= ~MPIDI_CH3_PKT_FLAG_RMA_FLUSH;
            accum_pkt->pkt_flags &= ~MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER;
        }

        stream_offset = j * stream_elem_count * predefined_dtp_size;
        stream_size = MPL_MIN(stream_elem_count * predefined_dtp_size, rest_len);
        rest_len -= stream_size;

        /* Set extended packet header if needed. */
        init_stream_dtype_ext_pkt(pkt_flags, target_dtp_ptr, stream_offset, &ext_hdr_ptr, &ext_hdr_sz,
                           &accum_pkt->info.flattened_type_size);

        mpi_errno = issue_from_origin_buffer(rma_op, vc, ext_hdr_ptr, ext_hdr_sz,
                                             stream_offset, stream_size, &curr_req);
        MPIR_ERR_CHECK(mpi_errno);

        if (curr_req != NULL) {
            if (rma_op->reqs_size == 0) {
                MPIR_Assert(rma_op->single_req == NULL && rma_op->multi_reqs == NULL);
                rma_op->reqs_size = stream_unit_count;

                if (stream_unit_count > 1) {
                    rma_op->multi_reqs =
                        (MPIR_Request **) MPL_malloc(sizeof(MPIR_Request *) * rma_op->reqs_size, MPL_MEM_RMA);
                    for (i = 0; i < rma_op->reqs_size; i++)
                        rma_op->multi_reqs[i] = NULL;
                }
            }

            if (rma_op->reqs_size == 1)
                rma_op->single_req = curr_req;
            else
                rma_op->multi_reqs[j] = curr_req;
        }

        rma_op->issued_stream_count++;

        if (accum_pkt->pkt_flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
            accum_pkt->pkt_flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE) {
            /* if piggybacked with LOCK flag, we
             * only issue the first streaming unit */
            MPIR_Assert(j == 0);
            break;
        }
    }   /* end of for loop */

    if (rma_op->issued_stream_count == stream_unit_count) {
        /* Mark that all stream units have been issued */
        rma_op->issued_stream_count = ALL_STREAM_UNITS_ISSUED;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_ISSUE_ACC_OP);
    return mpi_errno;
  fn_fail:
    if (rma_op->reqs_size == 1) {
        rma_op->single_req = NULL;
    }
    else if (rma_op->reqs_size > 1) {
        MPL_free(rma_op->multi_reqs);
        rma_op->multi_reqs = NULL;
    }
    rma_op->reqs_size = 0;
    goto fn_exit;
}


/* issue_get_acc_op() send GACC packet header and data. */
static int issue_get_acc_op(MPIDI_RMA_Op_t * rma_op, MPIR_Win * win_ptr,
                            MPIDI_RMA_Target_t * target_ptr, int pkt_flags)
{
    MPIDI_VC_t *vc = NULL;
    MPIR_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_CH3_Pkt_get_accum_t *get_accum_pkt = &rma_op->pkt.get_accum;
    int i, j;
    MPI_Aint stream_elem_count, stream_unit_count;
    MPI_Aint predefined_dtp_size, predefined_dtp_count, predefined_dtp_extent;
    MPI_Aint total_len, rest_len;
    MPI_Aint target_dtp_size;
    MPIR_Datatype*target_dtp_ptr = NULL;
    void *ext_hdr_ptr = NULL;
    MPI_Aint ext_hdr_sz = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ISSUE_GET_ACC_OP);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_ISSUE_GET_ACC_OP);

    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    if (rma_op->pkt.type == MPIDI_CH3_PKT_GET_ACCUM_IMMED) {
        MPIR_Request *resp_req = NULL;
        MPIR_Request *curr_req = NULL;

        get_accum_pkt->pkt_flags |= pkt_flags;

        rma_op->reqs_size = 1;

        /* Create a request for the GACC response.  Store the response buf, count, and
         * datatype in it, and pass the request's handle in the GACC packet. When the
         * response comes from the target, it will contain the request handle. */
        resp_req = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
        MPIR_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

        MPIR_Object_set_ref(resp_req, 2);

        resp_req->dev.user_buf = rma_op->result_addr;
        resp_req->dev.user_count = rma_op->result_count;
        resp_req->dev.datatype = rma_op->result_datatype;
        resp_req->dev.target_win_handle = MPI_WIN_NULL;
        resp_req->dev.source_win_handle = win_ptr->handle;

        /* Note: Get_accumulate uses the same packet type as accumulate */
        get_accum_pkt->request_handle = resp_req->handle;

        /* All origin data is in packet header, issue the header. */
        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, get_accum_pkt, sizeof(*get_accum_pkt), &curr_req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

        if (curr_req != NULL) {
            MPIR_Request_free(curr_req);
        }

        rma_op->single_req = resp_req;

        goto fn_exit;
    }

    /* Get total length of target data */
    MPIR_Datatype_get_size_macro(get_accum_pkt->datatype, target_dtp_size);
    total_len = target_dtp_size * get_accum_pkt->count;

    /* Get size and count for predefined datatype elements */
    if (MPIR_DATATYPE_IS_PREDEFINED(get_accum_pkt->datatype)) {
        predefined_dtp_size = target_dtp_size;
        predefined_dtp_count = get_accum_pkt->count;
        MPIR_Datatype_get_extent_macro(get_accum_pkt->datatype, predefined_dtp_extent);
    }
    else {
        MPIR_Datatype_get_ptr(get_accum_pkt->datatype, target_dtp_ptr);
        MPIR_Assert(target_dtp_ptr != NULL && target_dtp_ptr->basic_type != MPI_DATATYPE_NULL);
        MPIR_Datatype_get_size_macro(target_dtp_ptr->basic_type, predefined_dtp_size);
        predefined_dtp_count = total_len / predefined_dtp_size;
        MPIR_Datatype_get_extent_macro(target_dtp_ptr->basic_type, predefined_dtp_extent);
    }
    MPIR_Assert(predefined_dtp_count > 0 && predefined_dtp_size > 0 && predefined_dtp_extent > 0);

    /* Calculate number of predefined elements in each stream unit, and
     * total number of stream units. */
    stream_elem_count = MPIDI_CH3U_Acc_stream_size / predefined_dtp_extent;
    stream_unit_count = (predefined_dtp_count - 1) / stream_elem_count + 1;
    MPIR_Assert(stream_elem_count > 0 && stream_unit_count > 0);

    /* If there are more than one stream unit, mark the current packet
     * as stream packet */
    if (stream_unit_count > 1)
        pkt_flags |= MPIDI_CH3_PKT_FLAG_RMA_STREAM;

    rest_len = total_len;

    rma_op->reqs_size = stream_unit_count;

    if (rma_op->reqs_size > 1) {
        rma_op->multi_reqs =
            (MPIR_Request **) MPL_malloc(sizeof(MPIR_Request *) * rma_op->reqs_size, MPL_MEM_RMA);
        for (i = 0; i < rma_op->reqs_size; i++)
            rma_op->multi_reqs[i] = NULL;
    }

    MPIR_Assert(rma_op->issued_stream_count >= 0);

    for (j = 0; j < stream_unit_count; j++) {
        intptr_t stream_offset, stream_size;
        MPIR_Request *resp_req = NULL;
        MPIR_Request *curr_req = NULL;

        if (j < rma_op->issued_stream_count)
            continue;

        get_accum_pkt->pkt_flags |= pkt_flags;

        if (j != 0) {
            get_accum_pkt->pkt_flags &= ~MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED;
            get_accum_pkt->pkt_flags &= ~MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE;
        }
        if (j != stream_unit_count - 1) {
            get_accum_pkt->pkt_flags &= ~MPIDI_CH3_PKT_FLAG_RMA_UNLOCK;
            get_accum_pkt->pkt_flags &= ~MPIDI_CH3_PKT_FLAG_RMA_FLUSH;
            get_accum_pkt->pkt_flags &= ~MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER;
        }

        /* Create a request for the GACC response.  Store the response buf, count, and
         * datatype in it, and pass the request's handle in the GACC packet. When the
         * response comes from the target, it will contain the request handle. */
        resp_req = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
        MPIR_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

        MPIR_Object_set_ref(resp_req, 2);

        resp_req->dev.user_buf = rma_op->result_addr;
        resp_req->dev.user_count = rma_op->result_count;
        resp_req->dev.datatype = rma_op->result_datatype;
        resp_req->dev.target_win_handle = MPI_WIN_NULL;
        resp_req->dev.source_win_handle = win_ptr->handle;
        resp_req->dev.pkt_flags = pkt_flags;

        if (!MPIR_DATATYPE_IS_PREDEFINED(resp_req->dev.datatype)) {
            MPIR_Datatype*result_dtp = NULL;
            MPIR_Datatype_get_ptr(resp_req->dev.datatype, result_dtp);
            resp_req->dev.datatype_ptr = result_dtp;
            /* this will cause the datatype to be freed when the
             * request is freed. */
        }

        /* Note: Get_accumulate uses the same packet type as accumulate */
        get_accum_pkt->request_handle = resp_req->handle;

        stream_offset = j * stream_elem_count * predefined_dtp_size;
        stream_size = MPL_MIN(stream_elem_count * predefined_dtp_size, rest_len);
        rest_len -= stream_size;

        /* Set extended packet header if needed. */
        init_stream_dtype_ext_pkt(pkt_flags, target_dtp_ptr, stream_offset, &ext_hdr_ptr, &ext_hdr_sz,
                           &get_accum_pkt->info.flattened_type_size);

        /* Note: here we need to allocate an extended packet header in response request,
         * in order to store the stream_offset locally and use it in PktHandler_Get_AccumResp.
         * This extended packet header only contains stream_offset and does not contain any
         * other information. */
        {
            int dummy;
            init_stream_dtype_ext_pkt(pkt_flags, NULL /* target_dtp_ptr */ , stream_offset,
                                      &(resp_req->dev.ext_hdr_ptr), &(resp_req->dev.ext_hdr_sz),
                                      &dummy);
        }

        mpi_errno = issue_from_origin_buffer(rma_op, vc, ext_hdr_ptr, ext_hdr_sz,
                                             stream_offset, stream_size, &curr_req);
        MPIR_ERR_CHECK(mpi_errno);

        if (curr_req != NULL) {
            MPIR_Request_free(curr_req);
        }

        if (rma_op->reqs_size == 1)
            rma_op->single_req = resp_req;
        else
            rma_op->multi_reqs[j] = resp_req;

        rma_op->issued_stream_count++;

        if (get_accum_pkt->pkt_flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
            get_accum_pkt->pkt_flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE) {
            /* if piggybacked with LOCK flag, we
             * only issue the first streaming unit */
            MPIR_Assert(j == 0);
            break;
        }
    }   /* end of for loop */

    if (rma_op->issued_stream_count == stream_unit_count) {
        /* Mark that all stream units have been issued */
        rma_op->issued_stream_count = ALL_STREAM_UNITS_ISSUED;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_ISSUE_GET_ACC_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (rma_op->reqs_size == 1) {
        /* error case: drop both our reference to the request and the
         * progress engine's reference to it, since the progress
         * engine didn't get a chance to see it yet. */
        MPIR_Request_free(rma_op->single_req);
        MPIR_Request_free(rma_op->single_req);
        rma_op->single_req = NULL;
    }
    else if (rma_op->reqs_size > 1) {
        for (i = 0; i < rma_op->reqs_size; i++) {
            if (rma_op->multi_reqs[i] != NULL) {
                /* error case: drop both our reference to the request
                 * and the progress engine's reference to it, since
                 * the progress engine didn't get a chance to see it
                 * yet. */
                MPIR_Request_free(rma_op->multi_reqs[i]);
                MPIR_Request_free(rma_op->multi_reqs[i]);
            }
        }
        MPL_free(rma_op->multi_reqs);
        rma_op->multi_reqs = NULL;
    }
    rma_op->reqs_size = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


static int issue_get_op(MPIDI_RMA_Op_t * rma_op, MPIR_Win * win_ptr,
                        MPIDI_RMA_Target_t * target_ptr, int pkt_flags)
{
    MPIDI_CH3_Pkt_get_t *get_pkt = &rma_op->pkt.get;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    MPIR_Comm *comm_ptr;
    MPIR_Datatype*dtp;
    MPI_Datatype target_datatype;
    MPIR_Request *req = NULL;
    MPIR_Request *curr_req = NULL;
    struct iovec iov[MPL_IOV_LIMIT];
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ISSUE_GET_OP);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_ISSUE_GET_OP);

    rma_op->reqs_size = 1;

    /* create a request, store the origin buf, cnt, datatype in it,
     * and pass a handle to it in the get packet. When the get
     * response comes from the target, it will contain the request
     * handle. */
    curr_req = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
    if (curr_req == NULL) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    }

    MPIR_Object_set_ref(curr_req, 2);

    curr_req->dev.user_buf = rma_op->origin_addr;
    curr_req->dev.user_count = rma_op->origin_count;
    curr_req->dev.datatype = rma_op->origin_datatype;
    curr_req->dev.target_win_handle = MPI_WIN_NULL;
    curr_req->dev.source_win_handle = win_ptr->handle;
    if (!MPIR_DATATYPE_IS_PREDEFINED(curr_req->dev.datatype)) {
        MPIR_Datatype_get_ptr(curr_req->dev.datatype, dtp);
        curr_req->dev.datatype_ptr = dtp;
        /* this will cause the datatype to be freed when the
         * request is freed. */
    }

    get_pkt->request_handle = curr_req->handle;

    get_pkt->pkt_flags |= pkt_flags;

    comm_ptr = win_ptr->comm_ptr;
    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    MPIDI_CH3_PKT_RMA_GET_TARGET_DATATYPE(rma_op->pkt, target_datatype, mpi_errno);
    if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
        /* basic datatype on target. simply send the get_pkt. */
        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, get_pkt, sizeof(*get_pkt), &req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    }
    else {
        /* derived datatype on target. */
        void *ext_hdr_ptr = NULL;
        MPI_Aint ext_hdr_sz = 0;

        MPIR_Datatype_get_ptr(target_datatype, dtp);
        MPIR_Typerep_flatten_size(dtp, &get_pkt->info.flattened_type_size);

        ext_hdr_ptr = MPL_malloc(get_pkt->info.flattened_type_size, MPL_MEM_RMA);
        if (ext_hdr_ptr == NULL) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem",
                                 "**nomem %d", get_pkt->info.flattened_type_size);
        }
        MPL_VG_MEM_INIT(ext_hdr_ptr, get_pkt->info.flattened_type_size);

        MPIR_Typerep_flatten(dtp, ext_hdr_ptr);
        ext_hdr_sz = get_pkt->info.flattened_type_size;

        iov[0].iov_base = (void *) get_pkt;
        iov[0].iov_len = sizeof(*get_pkt);
        iov[1].iov_base = (void *) ext_hdr_ptr;
        iov[1].iov_len = ext_hdr_sz;

        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, 2, &req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);

        /* release the target datatype */
        MPIR_Datatype_ptr_release(dtp);

        /* If send is finished, we free extended header immediately.
         * Otherwise, store its pointer in request thus it can be freed when request is freed.*/
        if (req != NULL) {
            req->dev.ext_hdr_ptr = ext_hdr_ptr;
        }
        else {
            MPL_free(ext_hdr_ptr);
        }
    }

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

    /* release the request returned by iStartMsg or iStartMsgv */
    if (req != NULL) {
        MPIR_Request_free(req);
    }

    rma_op->single_req = curr_req;

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_ISSUE_GET_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    rma_op->single_req = NULL;
    rma_op->reqs_size = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


static int issue_cas_op(MPIDI_RMA_Op_t * rma_op,
                        MPIR_Win * win_ptr, MPIDI_RMA_Target_t * target_ptr,
                        int pkt_flags)
{
    MPIDI_VC_t *vc = NULL;
    MPIR_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_CH3_Pkt_cas_t *cas_pkt = &rma_op->pkt.cas;
    MPIR_Request *rmw_req = NULL;
    MPIR_Request *curr_req = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ISSUE_CAS_OP);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_ISSUE_CAS_OP);

    rma_op->reqs_size = 1;

    /* Create a request for the RMW response.  Store the origin buf, count, and
     * datatype in it, and pass the request's handle RMW packet. When the
     * response comes from the target, it will contain the request handle. */
    curr_req = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
    MPIR_ERR_CHKANDJUMP(curr_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    /* Set refs on the request to 2: one for the response message, and one for
     * the partial completion handler */
    MPIR_Object_set_ref(curr_req, 2);

    curr_req->dev.user_buf = rma_op->result_addr;
    curr_req->dev.datatype = rma_op->result_datatype;

    curr_req->dev.target_win_handle = MPI_WIN_NULL;
    curr_req->dev.source_win_handle = win_ptr->handle;

    cas_pkt->request_handle = curr_req->handle;
    cas_pkt->pkt_flags |= pkt_flags;

    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);
    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, cas_pkt, sizeof(*cas_pkt), &rmw_req);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

    if (rmw_req != NULL) {
        MPIR_Request_free(rmw_req);
    }

    rma_op->single_req = curr_req;

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_ISSUE_CAS_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    rma_op->single_req = NULL;
    rma_op->reqs_size = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


static int issue_fop_op(MPIDI_RMA_Op_t * rma_op,
                        MPIR_Win * win_ptr, MPIDI_RMA_Target_t * target_ptr,
                        int pkt_flags)
{
    MPIDI_VC_t *vc = NULL;
    MPIR_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_CH3_Pkt_fop_t *fop_pkt = &rma_op->pkt.fop;
    MPIR_Request *resp_req = NULL;
    MPIR_Request *curr_req = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ISSUE_FOP_OP);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_ISSUE_FOP_OP);

    rma_op->reqs_size = 1;

    /* Create a request for the GACC response.  Store the response buf, count, and
     * datatype in it, and pass the request's handle in the GACC packet. When the
     * response comes from the target, it will contain the request handle. */
    resp_req = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
    MPIR_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    MPIR_Object_set_ref(resp_req, 2);

    resp_req->dev.user_buf = rma_op->result_addr;
    resp_req->dev.datatype = rma_op->result_datatype;
    resp_req->dev.target_win_handle = MPI_WIN_NULL;
    resp_req->dev.source_win_handle = win_ptr->handle;

    fop_pkt->request_handle = resp_req->handle;

    fop_pkt->pkt_flags |= pkt_flags;

    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    if (rma_op->pkt.type == MPIDI_CH3_PKT_FOP_IMMED) {
        /* All origin data is in packet header, issue the header. */
        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, fop_pkt, sizeof(*fop_pkt), &curr_req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }
    else {
        MPI_Aint origin_dtp_size;
        MPIR_Datatype_get_size_macro(rma_op->origin_datatype, origin_dtp_size);
        mpi_errno = issue_from_origin_buffer(rma_op, vc, NULL, 0,       /*ext_hdr_ptr, ext_hdr_sz */
                                             0, 1 * origin_dtp_size, &curr_req);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (curr_req != NULL) {
        MPIR_Request_free(curr_req);
    }

    rma_op->single_req = resp_req;

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_ISSUE_FOP_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    rma_op->single_req = NULL;
    rma_op->reqs_size = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


/* issue_rma_op() is called by ch3u_rma_progress.c, it triggers
   proper issuing functions according to packet type. */
static inline int issue_rma_op(MPIDI_RMA_Op_t * op_ptr, MPIR_Win * win_ptr,
                               MPIDI_RMA_Target_t * target_ptr, int pkt_flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ISSUE_RMA_OP);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_ISSUE_RMA_OP);

    switch (op_ptr->pkt.type) {
    case (MPIDI_CH3_PKT_PUT):
    case (MPIDI_CH3_PKT_PUT_IMMED):
        mpi_errno = issue_put_op(op_ptr, win_ptr, target_ptr, pkt_flags);
        break;
    case (MPIDI_CH3_PKT_ACCUMULATE):
    case (MPIDI_CH3_PKT_ACCUMULATE_IMMED):
        mpi_errno = issue_acc_op(op_ptr, win_ptr, target_ptr, pkt_flags);
        break;
    case (MPIDI_CH3_PKT_GET_ACCUM):
    case (MPIDI_CH3_PKT_GET_ACCUM_IMMED):
        mpi_errno = issue_get_acc_op(op_ptr, win_ptr, target_ptr, pkt_flags);
        break;
    case (MPIDI_CH3_PKT_GET):
        mpi_errno = issue_get_op(op_ptr, win_ptr, target_ptr, pkt_flags);
        break;
    case (MPIDI_CH3_PKT_CAS_IMMED):
        mpi_errno = issue_cas_op(op_ptr, win_ptr, target_ptr, pkt_flags);
        break;
    case (MPIDI_CH3_PKT_FOP):
    case (MPIDI_CH3_PKT_FOP_IMMED):
        mpi_errno = issue_fop_op(op_ptr, win_ptr, target_ptr, pkt_flags);
        break;
    default:
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**winInvalidOp");
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_ISSUE_RMA_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#endif /* MPID_RMA_ISSUE_H_INCLUDED */
