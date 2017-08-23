/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPID_RMA_ISSUE_H_INCLUDED)
#define MPID_RMA_ISSUE_H_INCLUDED

#include "mpl_utlist.h"
#include "mpid_rma_types.h"

/* =========================================================== */
/*                    auxiliary functions                      */
/* =========================================================== */

/* immed_copy() copys data from origin buffer to
   IMMED packet header. */
#undef FUNCNAME
#define FUNCNAME immed_copy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
  fn_fail:
    goto fn_exit;
}

/* =========================================================== */
/*                  extended packet functions                  */
/* =========================================================== */

/* Copy derived datatype information issued within RMA operation. */
#undef FUNCNAME
#define FUNCNAME fill_in_derived_dtp_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void fill_in_derived_dtp_info(MPIDI_RMA_dtype_info * dtype_info, void *dataloop,
                                            MPIR_Datatype* dtp)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_FILL_IN_DERIVED_DTP_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_FILL_IN_DERIVED_DTP_INFO);

    /* Derived datatype on target, fill derived datatype info. */
    dtype_info->is_contig = dtp->is_contig;
    dtype_info->max_contig_blocks = dtp->max_contig_blocks;
    dtype_info->size = dtp->size;
    dtype_info->extent = dtp->extent;
    dtype_info->dataloop_size = dtp->dataloop_size;
    dtype_info->dataloop_depth = dtp->dataloop_depth;
    dtype_info->basic_type = dtp->basic_type;
    dtype_info->dataloop = dtp->dataloop;
    dtype_info->ub = dtp->ub;
    dtype_info->lb = dtp->lb;
    dtype_info->true_ub = dtp->true_ub;
    dtype_info->true_lb = dtp->true_lb;
    dtype_info->has_sticky_ub = dtp->has_sticky_ub;
    dtype_info->has_sticky_lb = dtp->has_sticky_lb;

    MPIR_Assert(dataloop != NULL);
    MPIR_Memcpy(dataloop, dtp->dataloop, dtp->dataloop_size);
    /* The dataloop can have undefined padding sections, so we need to let
     * valgrind know that it is OK to pass this data to writev later on. */
    MPL_VG_MAKE_MEM_DEFINED(dataloop, dtp->dataloop_size);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_FILL_IN_DERIVED_DTP_INFO);
}

/* Set extended header for ACC operation and return its real size. */
#undef FUNCNAME
#define FUNCNAME init_accum_ext_pkt
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int init_accum_ext_pkt(MPIDI_CH3_Pkt_flags_t flags,
                              MPIR_Datatype* target_dtp, intptr_t stream_offset,
                              void **ext_hdr_ptr, MPI_Aint * ext_hdr_sz)
{
    MPI_Aint _ext_hdr_sz = 0, _total_sz = 0;
    void *dataloop_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_INIT_ACCUM_EXT_PKT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_INIT_ACCUM_EXT_PKT);

    if ((flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) && target_dtp != NULL) {
        MPIDI_CH3_Ext_pkt_accum_stream_derived_t *_ext_hdr_ptr = NULL;

        /* dataloop is behind of extended header on origin.
         * TODO: support extended header array */
        _ext_hdr_sz = sizeof(MPIDI_CH3_Ext_pkt_accum_stream_derived_t);
        _total_sz = _ext_hdr_sz + target_dtp->dataloop_size;

        _ext_hdr_ptr = (MPIDI_CH3_Ext_pkt_accum_stream_derived_t *) MPL_malloc(_total_sz);
        MPL_VG_MEM_INIT(_ext_hdr_ptr, _total_sz);
        if (_ext_hdr_ptr == NULL) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem",
                                 "**nomem %s", "MPIDI_CH3_Ext_pkt_accum_stream_derived_t");
        }

        _ext_hdr_ptr->stream_offset = stream_offset;

        dataloop_ptr = (void *) ((char *) _ext_hdr_ptr + _ext_hdr_sz);
        fill_in_derived_dtp_info(&_ext_hdr_ptr->dtype_info, dataloop_ptr, target_dtp);

        (*ext_hdr_ptr) = _ext_hdr_ptr;
    }
    else if (flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) {
        MPIDI_CH3_Ext_pkt_accum_stream_t *_ext_hdr_ptr = NULL;

        _total_sz = sizeof(MPIDI_CH3_Ext_pkt_accum_stream_t);

        _ext_hdr_ptr = (MPIDI_CH3_Ext_pkt_accum_stream_t *) MPL_malloc(_total_sz);
        MPL_VG_MEM_INIT(_ext_hdr_ptr, _total_sz);
        if (_ext_hdr_ptr == NULL) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem",
                                 "**nomem %s", "MPIDI_CH3_Ext_pkt_accum_stream_t");
        }

        _ext_hdr_ptr->stream_offset = stream_offset;
        (*ext_hdr_ptr) = _ext_hdr_ptr;
    }
    else if (target_dtp != NULL) {
        MPIDI_CH3_Ext_pkt_accum_derived_t *_ext_hdr_ptr = NULL;

        /* dataloop is behind of extended header on origin.
         * TODO: support extended header array */
        _ext_hdr_sz = sizeof(MPIDI_CH3_Ext_pkt_accum_derived_t);
        _total_sz = _ext_hdr_sz + target_dtp->dataloop_size;

        _ext_hdr_ptr = (MPIDI_CH3_Ext_pkt_accum_derived_t *) MPL_malloc(_total_sz);
        MPL_VG_MEM_INIT(_ext_hdr_ptr, _total_sz);
        if (_ext_hdr_ptr == NULL) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem",
                                 "**nomem %s", "MPIDI_CH3_Ext_pkt_accum_derived_t");
        }

        dataloop_ptr = (void *) ((char *) _ext_hdr_ptr + _ext_hdr_sz);
        fill_in_derived_dtp_info(&_ext_hdr_ptr->dtype_info, dataloop_ptr, target_dtp);

        (*ext_hdr_ptr) = _ext_hdr_ptr;
    }

    (*ext_hdr_sz) = _total_sz;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_INIT_ACCUM_EXT_PKT);
    return mpi_errno;
  fn_fail:
    if ((*ext_hdr_ptr))
        MPL_free((*ext_hdr_ptr));
    (*ext_hdr_ptr) = NULL;
    (*ext_hdr_sz) = 0;
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME init_get_accum_ext_pkt
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int init_get_accum_ext_pkt(MPIDI_CH3_Pkt_flags_t flags,
                                  MPIR_Datatype* target_dtp, intptr_t stream_offset,
                                  void **ext_hdr_ptr, MPI_Aint * ext_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_INIT_GET_ACCUM_EXT_PKT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_INIT_GET_ACCUM_EXT_PKT);

    /* Check if get_accum still reuses accum' extended packet header. */
    MPIR_Assert(sizeof(MPIDI_CH3_Ext_pkt_accum_stream_derived_t) ==
                sizeof(MPIDI_CH3_Ext_pkt_get_accum_stream_derived_t));
    MPIR_Assert(sizeof(MPIDI_CH3_Ext_pkt_accum_derived_t) ==
                sizeof(MPIDI_CH3_Ext_pkt_get_accum_derived_t));
    MPIR_Assert(sizeof(MPIDI_CH3_Ext_pkt_accum_stream_t) ==
                sizeof(MPIDI_CH3_Ext_pkt_get_accum_stream_t));

    mpi_errno = init_accum_ext_pkt(flags, target_dtp, stream_offset, ext_hdr_ptr, ext_hdr_sz);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_INIT_GET_ACCUM_EXT_PKT);
    return mpi_errno;
}

/* =========================================================== */
/*                      issuinng functions                     */
/* =========================================================== */

/* issue_from_origin_buffer() issues data from origin
   buffer (i.e. non-IMMED operation). */
#undef FUNCNAME
#define FUNCNAME issue_from_origin_buffer
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int issue_from_origin_buffer(MPIDI_RMA_Op_t * rma_op, MPIDI_VC_t * vc,
                                    void *ext_hdr_ptr, MPI_Aint ext_hdr_sz,
                                    intptr_t stream_offset, intptr_t stream_size,
                                    MPIR_Request ** req_ptr)
{
    MPI_Datatype target_datatype;
    MPIR_Datatype*target_dtp = NULL, *origin_dtp = NULL;
    int is_origin_contig;
    MPL_IOV iov[MPL_IOV_LIMIT];
    int iovcnt = 0;
    MPIR_Request *req = NULL;
    MPI_Aint dt_true_lb;
    MPIDI_CH3_Pkt_flags_t flags;
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

        /* Set dataloop size in pkt header */
        MPIDI_CH3_PKT_RMA_SET_DATALOOP_SIZE(rma_op->pkt, target_dtp->dataloop_size, mpi_errno);
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

    iov[iovcnt].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) & (rma_op->pkt);
    iov[iovcnt].MPL_IOV_LEN = sizeof(rma_op->pkt);
    iovcnt++;

    MPIDI_CH3_PKT_RMA_GET_FLAGS(rma_op->pkt, flags, mpi_errno);
    if (!(flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) && target_dtp == NULL && is_origin_contig) {
        /* Fast path --- use iStartMsgv() to issue the data, which does not need a request
         * to be passed in:
         * (1) non-streamed op (do not need to send extended packet header);
         * (2) target datatype is predefined (do not need to send derived datatype info);
         * (3) origin datatype is contiguous (do not need to pack the data and send);
         */

        if (is_empty_origin == FALSE) {
            iov[iovcnt].MPL_IOV_BUF =
                (MPL_IOV_BUF_CAST) ((char *) rma_op->origin_addr + dt_true_lb + stream_offset);
            iov[iovcnt].MPL_IOV_LEN = stream_size;
            iovcnt++;
        }

        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, &req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

        if (origin_dtp != NULL) {
            if (req == NULL) {
                MPIR_Datatype_release(origin_dtp);
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
    req = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
    MPIR_ERR_CHKANDJUMP(req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    MPIR_Object_set_ref(req, 2);
    req->kind = MPIR_REQUEST_KIND__SEND;

    /* set extended packet header, it is freed when the request is freed.  */
    if (ext_hdr_sz > 0) {
        req->dev.ext_hdr_sz = ext_hdr_sz;
        req->dev.ext_hdr_ptr = ext_hdr_ptr;
    }

    if (origin_dtp != NULL) {
        req->dev.datatype_ptr = origin_dtp;
        /* this will cause the datatype to be freed when the request
         * is freed. */
    }

    if (is_origin_contig) {
        /* origin data is contiguous */

        if (is_empty_origin == FALSE) {
            iov[iovcnt].MPL_IOV_BUF =
                (MPL_IOV_BUF_CAST) ((char *) rma_op->origin_addr + dt_true_lb + stream_offset);
            iov[iovcnt].MPL_IOV_LEN = stream_size;
            iovcnt++;
        }

        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iSendv(vc, req, iov, iovcnt);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }
    else {
        /* origin data is non-contiguous */
        req->dev.segment_ptr = MPIR_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1(req->dev.segment_ptr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_Segment_alloc");

        MPIR_Segment_init(rma_op->origin_addr, rma_op->origin_count,
                          rma_op->origin_datatype, req->dev.segment_ptr, 0);
        req->dev.segment_first = stream_offset;
        req->dev.segment_size = stream_offset + stream_size;

        req->dev.OnFinal = 0;
        req->dev.OnDataAvail = 0;

        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = vc->sendNoncontig_fn(vc, req, iov[0].MPL_IOV_BUF, iov[0].MPL_IOV_LEN);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

  fn_exit:
    /* release the target datatype */
    if (target_dtp)
        MPIR_Datatype_release(target_dtp);
    (*req_ptr) = req;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_ISSUE_FROM_ORIGIN_BUFFER);
    return mpi_errno;
  fn_fail:
    if (req) {
        if (req->dev.datatype_ptr)
            MPIR_Datatype_release(req->dev.datatype_ptr);
        if (req->dev.ext_hdr_ptr)
            MPL_free(req->dev.ext_hdr_ptr);
        MPIR_Request_free(req);
    }

    (*req_ptr) = NULL;
    goto fn_exit;
}


/* issue_put_op() issues PUT packet header and data. */
#undef FUNCNAME
#define FUNCNAME issue_put_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int issue_put_op(MPIDI_RMA_Op_t * rma_op, MPIR_Win * win_ptr,
                        MPIDI_RMA_Target_t * target_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_VC_t *vc = NULL;
    MPIR_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_CH3_Pkt_put_t *put_pkt = &rma_op->pkt.put;
    MPIR_Request *curr_req = NULL;
    MPI_Datatype target_datatype;
    MPIR_Datatype*target_dtp_ptr = NULL;
    MPIDI_CH3_Ext_pkt_put_derived_t *ext_hdr_ptr = NULL;
    MPI_Aint ext_hdr_sz = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ISSUE_PUT_OP);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_ISSUE_PUT_OP);

    put_pkt->flags |= flags;

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
        MPIR_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);

        /* If derived datatype on target, add extended packet header. */
        MPIDI_CH3_PKT_RMA_GET_TARGET_DATATYPE(rma_op->pkt, target_datatype, mpi_errno);
        if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
            MPIR_Datatype_get_ptr(target_datatype, target_dtp_ptr);

            void *dataloop_ptr = NULL;

            /* dataloop is behind of extended header on origin.
             * TODO: support extended header array */
            ext_hdr_sz = sizeof(MPIDI_CH3_Ext_pkt_put_derived_t) + target_dtp_ptr->dataloop_size;
            ext_hdr_ptr = MPL_malloc(ext_hdr_sz);
            MPL_VG_MEM_INIT(ext_hdr_ptr, ext_hdr_sz);
            if (!ext_hdr_ptr) {
                MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem",
                                     "**nomem %s", "MPIDI_CH3_Ext_pkt_put_derived_t");
            }

            dataloop_ptr = (void *) ((char *) ext_hdr_ptr +
                                     sizeof(MPIDI_CH3_Ext_pkt_put_derived_t));
            fill_in_derived_dtp_info(&ext_hdr_ptr->dtype_info, dataloop_ptr, target_dtp_ptr);
        }

        mpi_errno = issue_from_origin_buffer(rma_op, vc, ext_hdr_ptr, ext_hdr_sz,
                                             0, rma_op->origin_count * origin_type_size, &curr_req);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
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
#undef FUNCNAME
#define FUNCNAME issue_acc_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int issue_acc_op(MPIDI_RMA_Op_t * rma_op, MPIR_Win * win_ptr,
                        MPIDI_RMA_Target_t * target_ptr, MPIDI_CH3_Pkt_flags_t flags)
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

        accum_pkt->flags |= flags;

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
        flags |= MPIDI_CH3_PKT_FLAG_RMA_STREAM;

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

        accum_pkt->flags |= flags;

        if (j != 0) {
            accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED;
            accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE;
        }
        if (j != stream_unit_count - 1) {
            accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_UNLOCK;
            accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_FLUSH;
            accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER;
        }

        stream_offset = j * stream_elem_count * predefined_dtp_size;
        stream_size = MPL_MIN(stream_elem_count * predefined_dtp_size, rest_len);
        rest_len -= stream_size;

        /* Set extended packet header if needed. */
        init_accum_ext_pkt(flags, target_dtp_ptr, stream_offset, &ext_hdr_ptr, &ext_hdr_sz);

        mpi_errno = issue_from_origin_buffer(rma_op, vc, ext_hdr_ptr, ext_hdr_sz,
                                             stream_offset, stream_size, &curr_req);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        if (curr_req != NULL) {
            if (rma_op->reqs_size == 0) {
                MPIR_Assert(rma_op->single_req == NULL && rma_op->multi_reqs == NULL);
                rma_op->reqs_size = stream_unit_count;

                if (stream_unit_count > 1) {
                    rma_op->multi_reqs =
                        (MPIR_Request **) MPL_malloc(sizeof(MPIR_Request *) * rma_op->reqs_size);
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

        if (accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
            accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE) {
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
        if (rma_op->multi_reqs != NULL) {
            MPL_free(rma_op->multi_reqs);
            rma_op->multi_reqs = NULL;
        }
    }
    rma_op->reqs_size = 0;
    goto fn_exit;
}


/* issue_get_acc_op() send GACC packet header and data. */
#undef FUNCNAME
#define FUNCNAME issue_get_acc_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int issue_get_acc_op(MPIDI_RMA_Op_t * rma_op, MPIR_Win * win_ptr,
                            MPIDI_RMA_Target_t * target_ptr, MPIDI_CH3_Pkt_flags_t flags)
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

        get_accum_pkt->flags |= flags;

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
        flags |= MPIDI_CH3_PKT_FLAG_RMA_STREAM;

    rest_len = total_len;

    rma_op->reqs_size = stream_unit_count;

    if (rma_op->reqs_size > 1) {
        rma_op->multi_reqs =
            (MPIR_Request **) MPL_malloc(sizeof(MPIR_Request *) * rma_op->reqs_size);
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

        get_accum_pkt->flags |= flags;

        if (j != 0) {
            get_accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED;
            get_accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE;
        }
        if (j != stream_unit_count - 1) {
            get_accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_UNLOCK;
            get_accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_FLUSH;
            get_accum_pkt->flags &= ~MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER;
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
        resp_req->dev.flags = flags;

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
        init_get_accum_ext_pkt(flags, target_dtp_ptr, stream_offset, &ext_hdr_ptr, &ext_hdr_sz);

        /* Note: here we need to allocate an extended packet header in response request,
         * in order to store the stream_offset locally and use it in PktHandler_Get_AccumResp.
         * This extended packet header only contains stream_offset and does not contain any
         * other information. */
        init_get_accum_ext_pkt(flags, NULL /* target_dtp_ptr */ , stream_offset,
                               &(resp_req->dev.ext_hdr_ptr), &(resp_req->dev.ext_hdr_sz));

        mpi_errno = issue_from_origin_buffer(rma_op, vc, ext_hdr_ptr, ext_hdr_sz,
                                             stream_offset, stream_size, &curr_req);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        if (curr_req != NULL) {
            MPIR_Request_free(curr_req);
        }

        if (rma_op->reqs_size == 1)
            rma_op->single_req = resp_req;
        else
            rma_op->multi_reqs[j] = resp_req;

        rma_op->issued_stream_count++;

        if (get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
            get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE) {
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


#undef FUNCNAME
#define FUNCNAME issue_get_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int issue_get_op(MPIDI_RMA_Op_t * rma_op, MPIR_Win * win_ptr,
                        MPIDI_RMA_Target_t * target_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_CH3_Pkt_get_t *get_pkt = &rma_op->pkt.get;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    MPIR_Comm *comm_ptr;
    MPIR_Datatype*dtp;
    MPI_Datatype target_datatype;
    MPIR_Request *req = NULL;
    MPIR_Request *curr_req = NULL;
    MPIDI_CH3_Ext_pkt_get_derived_t *ext_hdr_ptr = NULL;
    MPI_Aint ext_hdr_sz = 0;
    MPL_IOV iov[MPL_IOV_LIMIT];
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

    get_pkt->flags |= flags;

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
        MPIR_Datatype_get_ptr(target_datatype, dtp);
        void *dataloop_ptr = NULL;

        /* set extended packet header.
         * dataloop is behind of extended header on origin.
         * TODO: support extended header array */
        ext_hdr_sz = sizeof(MPIDI_CH3_Ext_pkt_get_derived_t) + dtp->dataloop_size;
        ext_hdr_ptr = MPL_malloc(ext_hdr_sz);
        MPL_VG_MEM_INIT(ext_hdr_ptr, ext_hdr_sz);
        if (!ext_hdr_ptr) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem",
                                 "**nomem %s", "MPIDI_CH3_Ext_pkt_get_derived_t");
        }

        dataloop_ptr = (void *) ((char *) ext_hdr_ptr + sizeof(MPIDI_CH3_Ext_pkt_get_derived_t));
        fill_in_derived_dtp_info(&ext_hdr_ptr->dtype_info, dataloop_ptr, dtp);

        /* Set dataloop size in pkt header */
        MPIDI_CH3_PKT_RMA_SET_DATALOOP_SIZE(rma_op->pkt, dtp->dataloop_size, mpi_errno);

        iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) get_pkt;
        iov[0].MPL_IOV_LEN = sizeof(*get_pkt);
        iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) ext_hdr_ptr;
        iov[1].MPL_IOV_LEN = ext_hdr_sz;

        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, 2, &req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);

        /* release the target datatype */
        MPIR_Datatype_release(dtp);

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


#undef FUNCNAME
#define FUNCNAME issue_cas_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int issue_cas_op(MPIDI_RMA_Op_t * rma_op,
                        MPIR_Win * win_ptr, MPIDI_RMA_Target_t * target_ptr,
                        MPIDI_CH3_Pkt_flags_t flags)
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
    cas_pkt->flags |= flags;

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


#undef FUNCNAME
#define FUNCNAME issue_fop_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int issue_fop_op(MPIDI_RMA_Op_t * rma_op,
                        MPIR_Win * win_ptr, MPIDI_RMA_Target_t * target_ptr,
                        MPIDI_CH3_Pkt_flags_t flags)
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

    fop_pkt->flags |= flags;

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
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
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
#undef FUNCNAME
#define FUNCNAME issue_rma_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int issue_rma_op(MPIDI_RMA_Op_t * op_ptr, MPIR_Win * win_ptr,
                               MPIDI_RMA_Target_t * target_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ISSUE_RMA_OP);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_ISSUE_RMA_OP);

    switch (op_ptr->pkt.type) {
    case (MPIDI_CH3_PKT_PUT):
    case (MPIDI_CH3_PKT_PUT_IMMED):
        mpi_errno = issue_put_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_ACCUMULATE):
    case (MPIDI_CH3_PKT_ACCUMULATE_IMMED):
        mpi_errno = issue_acc_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_GET_ACCUM):
    case (MPIDI_CH3_PKT_GET_ACCUM_IMMED):
        mpi_errno = issue_get_acc_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_GET):
        mpi_errno = issue_get_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_CAS_IMMED):
        mpi_errno = issue_cas_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_FOP):
    case (MPIDI_CH3_PKT_FOP_IMMED):
        mpi_errno = issue_fop_op(op_ptr, win_ptr, target_ptr, flags);
        break;
    default:
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**winInvalidOp");
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_ISSUE_RMA_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#endif /* MPID_RMA_ISSUE_H_INCLUDED */
